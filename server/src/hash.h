#ifndef HASH_H
#define HASH_H

#include <list>
#include <memory>
#include <algorithm>
#include <vector>
#include <set>
#include <regex>
#include "../../type/encoding.h"
#include "lru.h"

// 参考redis的string类型的实现
typedef struct kvString {
    uint32_t len;
    std::shared_ptr<char[]> data;
} kvString;

// 哈希表节点
typedef struct Entry {
    uint32_t encoding;
    std::string key;
    std::shared_ptr<char[]> data;
    ~Entry() {
    }
} Entry;

class HashTable {
public:
    // 哈希表，连接法解决冲突
    std::vector<std::list<std::shared_ptr<Entry>>> hash_;
    // LRU算法进行淘汰
    LRUCache lru_;
private:
    std::set<std::string> keySet_;
    // 哈希表长度
    int size_;
    int rehashIndex_;
    // 哈希函数
    int hash(std::string key) {
        unsigned int seed = 131; // 31 131 1313 13131 131313 etc..
        unsigned int hash = 0;
        char* str = const_cast<char*>(key.data());
        while (*str) {
            hash = hash * seed + (*str++);
        }
        return (hash & 0x7FFFFFFF) % hash_.size();
    }
    
    // 构建哈希表节点
    inline void insertWithEncoding(Entry* entry, std::string key, std::string val, uint32_t encoding) {
        switch (encoding) {
            // string类型
            case LightKV_STRING : {
                entry->encoding = encoding;
                entry->key = key;
                char* valBuf = new char[val.size()];
                memcpy(valBuf, val.data(), val.size());
                entry->data = std::shared_ptr<char[]>(
                                (char*)new kvString{(uint32_t)val.size(), std::shared_ptr<char[]>(valBuf)});
                break;
            }
            // list类型
            case LightKV_LIST : {
                auto node = new std::list<kvString>;
                char* valBuf = new char[val.size()];
                memcpy(valBuf, val.data(), val.size());
                node->push_back(kvString{(uint32_t)val.size(), std::shared_ptr<char[]>(valBuf)});
                entry->encoding = encoding;
                entry->key = key;
                entry->data = std::shared_ptr<char[]>((char*)node);
                break;
            }
        }
    }
public:
    HashTable() = delete;
    // 初始化
    HashTable(int size) {
        hash_.resize(size);
        rehashIndex_ = 0;
        size_ = size;
    }

    // 向哈希表插入值
    void insert(std::string key, std::string val, uint32_t encoding) {
        int slot = hash(key);
        for (auto i = hash_[slot].begin(); i != hash_[slot].end(); ++i) {
            // key already exists, update!
            if (i->get()->key == key) {
                if (i->get()->encoding == LightKV_LIST) {
                    std::list<kvString>* p = (std::list<kvString>*)(i->get()->data.get());
                    char* valBuf = new char[val.size()];
                    memcpy(valBuf, val.data(), val.size());
                    p->push_back(kvString{(uint32_t)val.size(), std::shared_ptr<char[]>(valBuf)});
                    lru_.update(key);
                } else if (i->get()->encoding == LightKV_STRING) {
                    insertWithEncoding(i->get(), key, val, encoding);
                    lru_.update(key);
                }
                return;
            }
        }
        // new entry
        Entry* entry = new Entry;
        insertWithEncoding(entry, key, val, encoding);
        hash_[slot].push_front(std::shared_ptr<Entry>(entry)); 
        keySet_.insert(key);
        lru_.insert(key);
    }

    // 向哈希表插入哈希节点
    void insertEntry(std::shared_ptr<Entry> entry, std::string key) {
        int slot = hash(key);
        hash_[slot].push_front(entry);
        keySet_.insert(key);
        lru_.insert(key);
    }

    // 从哈希表中获取key的元素
    void get(std::string key, std::vector<std::string>& res) {
        int slot = hash(key);
        for (auto i = hash_[slot].begin(); i != hash_[slot].end(); ++i) {
            if (i->get()->key == key) {
                if (i->get()->encoding == LightKV_STRING) {
                    kvString* kvStr = (kvString*)(i->get()->data.get());
                    std::string s(kvStr->data.get(), kvStr->data.get() + kvStr->len);
                    res.push_back(s);
                    lru_.update(key);
                }
                if (i->get()->encoding == LightKV_LIST) {
                    std::list<kvString>* p = (std::list<kvString>*)(i->get()->data.get());
                    for (auto it = p->begin(); it != p->end(); ++it) {
                        std::string s(it->data.get(), it->data.get() + it->len);
                        res.push_back(s);
                    }
                    lru_.update(key);
                }
            }
        }   
    }

    // 判断key是否存在
    bool exist(std::string key) {
        int slot = hash(key);
        for (auto i = hash_[slot].begin(); i != hash_[slot].end(); ++i) {
            if (i->get()->key == key) {
                return true;
            }
        }
        return false;
    }

    // 删除key项
    int del(std::string key) {
        int slot = hash(key);
        for (auto it = hash_[slot].begin(); it != hash_[slot].end(); ++it) {
            if (it->get()->key == key) {
                hash_[slot].remove(*it);
                keySet_.erase(key);
                lru_.remove(key);
                return LightKV_DEL_SUCCESS;
            }
        }
        return LightKV_DEL_FAIL;
    }

    // 依据负载因子判断是否需要rehash
    bool needRehash() {
        if ((double)(keySet_.size()) / size_ > 1.5) {
            return true;
        }
        return false;
    }

    // 重置哈希表长度
    void resize(int size) {
        hash_.resize(size);
        size_ = size;
    }

    // 获取哈希表长度
    size_t size() {
        return size_;
    }

    // 返回key的数量
    size_t keyNum() {
        return keySet_.size();
    }

    // 清空
    void clear() {
        rehashIndex_ = 0;
        hash_.clear();
        keySet_.clear();
    }

    // 进行rehash
    bool progressiveRehash(std::shared_ptr<HashTable> h2, int emptyVisits) {
        auto& d2 = h2.get()->hash_;
        auto& d1 = hash_;
        for (int i = rehashIndex_; i < d1.size() && emptyVisits > 0; ++i) {
            if (d1[i].empty()) {
                emptyVisits--;
                rehashIndex_ = i + 1;
                continue;
            }
            // 向新的哈希表添加哈希节点
            for (auto it = d1[i].begin(); it != d1[i].end(); ++it) {
                std::string key = (*it).get()->key;
                h2->insertEntry(*it, key);
                lru_.remove(key);
            }
            d1[i].clear();
            rehashIndex_ = i + 1;
        }
        if (rehashIndex_ == d1.size()) {
            return true;
        }
        return false;
    }

    // 进行正则匹配获取当前哈希表中的key
    void findKeyName(std::string keyRex, std::vector<std::string>& res) {
        std::regex pattern(keyRex);
        for (int i = 0; i < hash_.size(); ++i) {
            for (auto it = hash_[i].begin(); it != hash_[i].end(); ++it) {
                if (std::regex_search(it->get()->key, pattern)) {
                    res.push_back(it->get()->key);
                }
            }
        }
    }

    // 随机获取一个key
    std::string randomKeyFind() {
        auto it(keySet_.begin());
        // 用于将迭代器前进（或者后退）指定长度的距离
        std::advance(it, rand() % keySet_.size());
        return *it;
    }

    // 对lru链表中的key进行位置更新
    void update(std::string& key) {
        if (!exist(key)) return;
        lru_.update(key);
    }

};

#endif