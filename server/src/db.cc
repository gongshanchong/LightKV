#include "db.h"
#include <cstdio>
std::shared_mutex smutex_;
// Although sizeof can be obtained at compile time
static constexpr uint32_t fixedEntryHeadLen = 3 * sizeof(uint32_t);
static constexpr uint32_t sizeofUint32 = sizeof(uint32_t);
static constexpr uint32_t sizeofChar = sizeof(char);

void LightKVDB::init() {
    // db结构初始化
    hashSize_ = HASH_SIZE_INIT;
    hash1_ = std::shared_ptr<HashTable>(new HashTable(hashSize_));
    hash2_ = std::shared_ptr<HashTable>(new HashTable(hashSize_));
    expires_ = std::unique_ptr<HashTable>(new HashTable(hashSize_));
    io_ = std::unique_ptr<KVio>(new KVio(RDB_FILE_NAME));
    rdbFileReadInitDB();
    rdbe_ = new rdbEntry();
    // 定时器初始化
    timerRdb_ = std::shared_ptr<KVTimer>(new KVTimer());
    timerRehash_ = std::shared_ptr<KVTimer>(new KVTimer());
    timerFixedTimeDelKey_ = std::shared_ptr<KVTimer>(new KVTimer());
    timerMemoryDetect_ = std::shared_ptr<KVTimer>(new KVTimer());
    // 每隔一段时间进行相关操作
    timerRdb_->start(RDB_INTERVAL, std::bind(&LightKVDB::rdbSave, this));
    timerRehash_->start(REHASH_DETECT_INTERVAL, std::bind(&LightKVDB::rehash, this));
    timerFixedTimeDelKey_->start(FIXED_TIME_DELETE_EXPIRED_KEY, std::bind(&LightKVDB::fixedTimeDeleteExpiredKey, this));
    timerMemoryDetect_->start(MEMORY_DETECT_INTERVAL, std::bind(&LightKVDB::memoryDetector, this));
    rehashFlag_ = false;
}

LightKVDB::~LightKVDB() {
    timerRdb_->stop();
    timerRehash_->stop();
    timerFixedTimeDelKey_->stop();
    timerMemoryDetect_->stop();
    io_->flushCache();
    delete rdbe_;
}

// 向db插入kv键值对
void LightKVDB::insert(std::string key, std::string val, uint32_t encoding) {
    if (!rehashFlag_) {
        // 锁粒度用来描述单个锁所保护的数据量。细粒度保护着少量的数据，粗粒度保护着大量的数据。
        std::unique_lock<std::shared_mutex> lk(smutex_);
        hash1_->insert(key, val, encoding);
        expires_->update(key);
        return;
    }
    if (hash1_->exist(key)) {
        std::unique_lock<std::shared_mutex> lk(smutex_);
        hash1_->insert(key, val, encoding);
        expires_->update(key);
        return;
    }
    // 渐进式rehash
    std::unique_lock<std::shared_mutex> lk(smutex_);
    hash2_->insert(key, val, encoding);
    expires_->update(key);
    progressiveRehash(hash2_);
}

// 处理时间过期的kv键值对，定期删除
void LightKVDB::fixedTimeDeleteExpiredKey() {
    std::string keyRandom;
    while (true) {
        int expiredKeyNum = 0;
        for (int i = 0; i < ACTIVE_EXPIRE_CYCLE_LOOKUPS_PER_LOOP; ++i) {
            if (expires_->keyNum() <= 0) goto breakLoop;
            keyRandom = expires_->randomKeyFind();
            if (expired(keyRandom)) {
                std::unique_lock<std::shared_mutex> lk(smutex_);
                hash1_->del(keyRandom);
                expires_->del(keyRandom);
                lk.unlock();
                expiredKeyNum++;
            }
        }
        if (static_cast<double>(expiredKeyNum) / ACTIVE_EXPIRE_CYCLE_LOOKUPS_PER_LOOP < 0.25) {
            goto breakLoop;
        }
    }
    breakLoop:
    return;
}

// 判断该key是否过期
bool LightKVDB::expired(std::string key) {
    std::vector<std::string> e;
    expires_->get(key, e);
    if (!e.empty()) {
        uint64_t expires = stol(e[0]); // use stoi may throw out of range error
        auto now = std::chrono::system_clock::now(); 
        auto timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count();
        if (timestamp >= expires) {
            return true;
        }
    }
    return false;
}

void LightKVDB::get(std::string key, std::vector<std::string>& res) {
    // there are 2 cases for empty res:
    // 1. exceed the time limit
    // 2. no data
    
    // 惰性删除
    if (expired(key)) {
        res = std::vector<std::string>();
        std::unique_lock<std::shared_mutex> lkS(smutex_);
        hash1_->del(key); 
        expires_->del(key);
        lkS.unlock();
        return;
    }
    if (!rehashFlag_) {
        hash1_->get(key, res);
        if (expires_->exist(key)) {
            std::unique_lock<std::shared_mutex> lkS(smutex_);
            expires_->update(key);
        }
        return;
    }
    if (hash1_->exist(key)) {
        hash1_->get(key, res);
        if (expires_->exist(key)) {
            std::unique_lock<std::shared_mutex> lkS(smutex_);
            expires_->update(key);
        }
        return;
    }
    hash2_->get(key, res);
    if (expires_->exist(key)) {
        std::unique_lock<std::shared_mutex> lkS(smutex_);
        expires_->update(key);
    }
    // 渐进式rehash
    std::unique_lock<std::shared_mutex> lk(smutex_);
    progressiveRehash(hash2_);
    lk.unlock();
}

// 从db中删除key
int LightKVDB::del(std::string key) {
    std::unique_lock<std::shared_mutex> lk(smutex_);
    int flag;
    if (rehashFlag_) {
        int f1 = hash1_->del(key);
        int f2 = hash2_->del(key);
        expires_->del(key);
        flag = (f1 == LightKV_DEL_SUCCESS || f2 == LightKV_DEL_SUCCESS);
    } else {
        int f1 = hash1_->del(key);
        expires_->del(key);
        flag = (f1 == LightKV_DEL_SUCCESS);
    }
    return flag ? LightKV_DEL_SUCCESS : LightKV_DEL_FAIL;
}

// 进行正则匹配获取当前哈希表中的key
void LightKVDB::getKeyName(std::string keyRex, std::vector<std::string>& res) {
    std::shared_lock<std::shared_mutex> lk(smutex_);
    hash1_->findKeyName(keyRex, res);
    hash2_->findKeyName(keyRex, res);
}

// 设置过期时间
int LightKVDB::setExpire(std::string key, uint64_t expires) {
    bool exist1 = hash1_->exist(key);
    bool exist2 = hash2_->exist(key);
    if (!exist1 && !exist2) return LightKV_SET_EXPIRE_FAIL;
    std::unique_lock<std::shared_mutex> lk(smutex_);
    expires_->insert(key, to_string(expires), LightKV_STRING);
    return LightKV_SET_EXPIRE_SUCCESS;
}

// rdb持久化
void LightKVDB::rdbSave() {
    if (rehashFlag_) return;
    std::unique_lock<std::shared_mutex> lk(smutex_);
    printf("rdb save triggered!");
    io_->clearFile();
    for (int i = 0; i < hash1_->hash_.size(); ++i) {
        auto d = hash1_->hash_[i];
        if (d.empty()) {
            continue;
        }
        for (auto it = d.begin(); it != d.end(); ++it) {
            std::string key = it->get()->key;
            if (expired(key)) continue;
            uint32_t encoding = it->get()->encoding;
            void* data = it->get()->data.get();
            saveKVWithEncoding(key, encoding, data);
        }
    }
    io_->flushCache(); // C cache->Page cache
}

// 将数据转换为rdb数据项
void LightKVDB::saveKVWithEncoding(std::string key, uint32_t encoding, void* data) {
    switch (encoding) {
        case LightKV_STRING : {
            makeStringObject2RDBEntry(rdbe_, key, encoding, data);
            break;
        }
        case LightKV_LIST : {
            makeListObject2RDBEntry(rdbe_, key, encoding, data);
            break;
        }
    }
    rdbEntryWrite(rdbe_);
}

void LightKVDB::makeStringObject2RDBEntry(rdbEntry* rdbe, std::string key, uint32_t encoding, void* data) {
    // 计算长度
    uint32_t keyLen = (uint32_t)key.size();
    kvString* kvStr = (kvString*)data;
    uint32_t totalLen = fixedEntryHeadLen + keyLen + sizeofUint32 + kvStr->len;
    // rdb数据项的项头
    rdbe->totalLen = totalLen;
    rdbe->encoding = encoding;
    rdbe->keyLen = keyLen;
    // 设置rdb数据项中的key
    char* buf = new char[keyLen]; 
    memcpy(buf, key.data(), keyLen);
    rdbe->key = std::shared_ptr<char[]>(buf);
    // 设置rdb数据项中的data
    buf = new char[sizeofUint32 + strlen(kvStr->data.get())];
    memcpy(buf, kvStr, sizeofUint32);
    memcpy(buf + sizeofUint32, kvStr->data.get(), kvStr->len);
    rdbe->data = std::shared_ptr<char[]>(buf);
}

void LightKVDB::makeListObject2RDBEntry(rdbEntry* rdbe, std::string key, uint32_t encoding, void* data) {
    // 计算长度
    uint32_t keyLen = (uint32_t)key.size();
    auto lst = (std::list<kvString>*)data;
    uint32_t totalLen = fixedEntryHeadLen + keyLen;
    int bytes = 0;
    for (auto it = lst->begin(); it != lst->end(); ++it) {
        bytes += sizeofUint32 + it->len;
    }
    char* buf = new char[bytes];
    char* cur = buf;
    for (auto it = lst->begin(); it != lst->end(); ++it) {
        memcpy(cur, (char*)(&(*it)), sizeofUint32);
        cur += sizeofUint32;
        memcpy(cur, it->data.get(), it->len);
        cur += it->len; 
        totalLen += sizeofUint32 + it->len;
    }
    // rdb数据项的项头
    rdbe->totalLen = totalLen;
    rdbe->encoding = encoding;
    rdbe->keyLen = keyLen;
    // 设置rdb数据项中的data
    rdbe->data = std::shared_ptr<char[]>(buf);
    // 设置rdb数据项中的key
    buf = new char[keyLen]; 
    memcpy(buf, key.data(), keyLen);
    rdbe->key = std::shared_ptr<char[]>(buf);
}

// 将rdb数据项写入文件
void LightKVDB::rdbEntryWrite(rdbEntry* rdbe) {
    // write totalLen, encoding and keyLen
    int rest = io_->kvioFileWrite(rdbe, fixedEntryHeadLen); 
    CHECK_REST_AND_LOG(rest);
    size_t keyLen = rdbe->keyLen;
    size_t dataLen = rdbe->totalLen - fixedEntryHeadLen - keyLen;
    rest = io_->kvioFileWrite(rdbe->key.get(), keyLen);
    CHECK_REST_AND_LOG(rest);
    rest = io_->kvioFileWrite(rdbe->data.get(), dataLen);
    CHECK_REST_AND_LOG(rest);
}

// 从文件加载rdb数据项
void LightKVDB::rdbLoadEntry() {
    // 获取rdb数据项的项头
    char fixedBuf[fixedEntryHeadLen];
    int rest = io_->kvioFileRead(fixedBuf, fixedEntryHeadLen);
    CHECK_REST_AND_LOG(rest);
    int totalLen, encoding, keyLen;
    memcpy(&totalLen, fixedBuf, sizeofUint32);
    memcpy(&encoding, fixedBuf + sizeofUint32, sizeofUint32);
    memcpy(&keyLen, fixedBuf + 2 * sizeofUint32, sizeofUint32);
    // 获取rdb数据项的key
    char keyBuf[keyLen];
    rest = io_->kvioFileRead(keyBuf, keyLen);
    CHECK_REST_AND_LOG(rest);
    std::string key(keyBuf, keyBuf + keyLen);
    // 获取rdb数据项的data
    int waitProcessLen = totalLen - fixedEntryHeadLen - keyLen;
    uint32_t len = 0;
    switch (encoding) {
        case LightKV_STRING : {
            rest = io_->kvioFileRead(&len, sizeofUint32);
            CHECK_REST_AND_LOG(rest);
            std::string valStr(len, 0);
            rest = io_->kvioFileRead(valStr.data(), len);
            CHECK_REST_AND_LOG(rest);
            hash1_->insert(key, valStr, encoding);
            break;
        }
        case LightKV_LIST : {
            while (waitProcessLen > 0) {
                rest = io_->kvioFileRead(&len, sizeofUint32);
                CHECK_REST_AND_LOG(rest);
                std::string valStr(len, 0);
                rest = io_->kvioFileRead(valStr.data(), len);
                CHECK_REST_AND_LOG(rest);
                hash1_->insert(key, valStr, encoding);
                waitProcessLen -= sizeofUint32 + len;
            }
            break;
        }
    }
}

// 从文件加载rdb数据项进行数据库的初始化
void LightKVDB::rdbFileReadInitDB() {
    if (io_->empty()) {
        printf("No rdb file to load.");
        return;
    }
    while (!io_->empty()) {
        rdbLoadEntry();
    }
    printf("Rdb file loaded.");
}

// 判断是否需要rehash并设置标志
void LightKVDB::rehash() {
    if (rehashFlag_) return;
    std::shared_lock<std::shared_mutex> lk(smutex_);
    if (!hash1_->needRehash()) return;
    lk.unlock();
    rehashFlag_ = true;
    hash2_->resize(hash1_->size() * 2);
}

// 进行rehash
void LightKVDB::progressiveRehash(std::shared_ptr<HashTable> h2) {
    if (!rehashFlag_) return;
    bool completed = hash1_->progressiveRehash(h2, REHASH_MAX_EMPTY_VISITS);
    if (completed) {
        rehashFlag_ = false;
        std::swap(hash1_, hash2_);
        hash2_->clear();
    }
}

// 获取虚拟内存驻留集合大小，是驻留在物理内存的一部分。它没有交换到硬盘。它包括代码，数据和栈。
uint32_t LightKVDB::getVmrss() {
    int pid = getpid();
    // 在Linux中，用户进程在/proc/{pid}/status文件中记录了该进程的内存使用实时情况
    std::string file_name = "/proc/" + to_string(pid) + "/status";
    ifstream file(file_name);
    std::string line;
    uint32_t vmrss = 0;
    while (getline(file, line)) {
        if (line.find("VmRSS") != string::npos)
        {
            vmrss = stoi(line.substr(line.find(" ")));
            break;
        } 
    }
    return vmrss;
}

// 内存检测
void LightKVDB::memoryDetector() {
    uint32_t vmrss = getVmrss();
    if (vmrss > MAX_MEMORY_KB) {
        memoryObsolescence();
    }
}

// lru算法，在设置了过期时间的数据中进行淘汰
void LightKVDB::memoryObsolescence() {
    std::unique_lock<std::shared_mutex> lk(smutex_);
    int keyNum = expires_->keyNum();
    for (int i = 0; i < keyNum; ++i) {
        std::string waitingToDelKey = expires_->lru_.lastKey();
        hash1_->del(waitingToDelKey);
        hash2_->del(waitingToDelKey);
        expires_->del(waitingToDelKey);
        if (i % 10 && getVmrss() <= MAX_MEMORY_KB) {
            break;
        }
    }
}