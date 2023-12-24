#ifndef DB_H
#define DB_H

#include <sys/resource.h>
#include <fstream>
#include <unistd.h>
#include "hash.h"
#include "io.h"
#include "../../timer/timer.h"
#include <mutex>
#include <shared_mutex>
#define HASH_SIZE_INIT 512
#define RDB_INTERVAL 5000 // 5000ms rdb save triggered
#define REHASH_DETECT_INTERVAL 5000
#define REHASH_MAX_EMPTY_VISITS 50
#define FIXED_TIME_DELETE_EXPIRED_KEY 2000
#define ACTIVE_EXPIRE_CYCLE_LOOKUPS_PER_LOOP 20
#define MEMORY_DETECT_INTERVAL 1000

#define MAX_MEMORY_KB 100000 // kb
#define RDB_FILE_NAME "lightkv.rdb"

#define CHECK_REST_AND_LOG(x)          \
    do {                               \
        if (x < 0) {                \
            printf("rdb read error!"); \
        }                              \
    } while (0)

extern std::shared_mutex smutex_;

// rdb数据项
typedef struct rdbEntry {
    uint32_t totalLen;
    uint32_t encoding;
    uint32_t keyLen;
    std::shared_ptr<char[]> key;
    std::shared_ptr<char[]> data;
} rdbEntry;
 

typedef class LightKVDB {
private:
    // 哈希表，db结构（双哈希表设计）
    std::shared_ptr<HashTable> hash1_;
    std::shared_ptr<HashTable> hash2_;
    uint64_t hashSize_;
    bool rehashFlag_;
    // Redis 实现的是一种近似 LRU 算法，目的是为了更好的节约内存，
    // Redis它的实现方式是在 Redis 的对象结构体中添加一个额外的字段，用于记录此数据的最后一次访问时间。
    // 过期字典，用于进行过期删除和内存淘汰（设置了过期时间的key中）
    std::unique_ptr<HashTable> expires_;
    // io
    std::unique_ptr<KVio> io_;
    // 定时器
    std::shared_ptr<KVTimer> timerRdb_;
    std::shared_ptr<KVTimer> timerRehash_;
    std::shared_ptr<KVTimer> timerFixedTimeDelKey_;
    std::shared_ptr<KVTimer> timerMemoryDetect_;
    rdbEntry* rdbe_;
    // 存储kv
    void saveKVWithEncoding(std::string key, uint32_t encoding, void* data);
    // rdb相关
    void makeStringObject2RDBEntry(rdbEntry* rdbe, std::string key, uint32_t encoding, void* data);
    void makeListObject2RDBEntry(rdbEntry* rdbe, std::string key, uint32_t encoding, void* data);
    void rdbEntryWrite(rdbEntry* rdbe);
    void rdbFileReadInitDB();
    void rdbLoadEntry();
    void rdbSave();
    void rehash();
    // rehash
    void progressiveRehash(std::shared_ptr<HashTable> hash2);
    // 过期策略和内存淘汰
    bool expired(std::string key);
    void fixedTimeDeleteExpiredKey();
    void memoryDetector();
    void memoryObsolescence();
    uint32_t getVmrss();
public:
    void init();
    // 增删改查
    void insert(std::string key, std::string val, uint32_t encoding);
    void get(std::string key, std::vector<std::string>& res);
    int del(std::string key);
    int setExpire(std::string key, uint64_t expires);
    void getKeyName(std::string keyRex, std::vector<std::string>& res);
    //单例模式
	static LightKVDB *getInstance(){
		static LightKVDB instance;
		return &instance;
	}
    LightKVDB()=default;
    ~LightKVDB();

} LightKVDB;

#endif