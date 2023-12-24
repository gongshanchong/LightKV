#ifndef LRU_H
#define LRU_H
#include <string>
#include <unordered_map>
#include "../../exception/lightkv_exception.h"

// 节点结构
typedef struct Node {
    std::string key;
    Node* next;
    Node* prev;
} LRUNode;

// 双向链表
typedef class DList {
public:
    DList();
    ~DList();
    bool empty();
    // 头插法
    void insertFront(const std::string& key);
    void insertFront(Node* node);
    Node* tail();
    void popBack();
    // do not delete node, because of it will insert to the front of the dlist then
    void remove(Node* node);
    int size();
private:
    int count_;
    Node* head_;
    Node* tail_;

} DList;

// not to set max size of lru, because of already have memory policy
// the tail of lru cache is the node waiting to delete
typedef class LRUCache {
public:
    LRUCache();
    ~LRUCache();
    // 获取LRU链表中最后一个元素，最近最少使用的元素
    std::string lastKey();
    // 更新node的位置
    void update(const std::string& key);
    void remove(const std::string& key);
    void insert(const std::string& key);
private:
    // 记录所有key与节点的对应
    std::unordered_map<std::string, Node*> keyMap_;
    // LRU算法实现的链表，记录最近使用的节点，最近使用的在表头
    DList* dlist_;
    
} LRUCache;

#endif