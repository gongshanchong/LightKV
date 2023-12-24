#ifndef KV_Client_H
#define KV_Client_H

#include <string>
#include "../lightrpc/net/rpc/rpc_channel.h"
#include "../proto/kvserver/kvserver.pb.h"
#include "../type/encoding.h"

class KVClient {
public:
    // 初始化函数
    KVClient(std::string config_path);

    // 客户端对应的操作
    // 添加kv键值对
    int setKV(std::string key, std::string val, uint32_t encoding);
    // 删除键对应的内容
    int delK(std::string key);
    // 设置键过期时间
    int setExpires(std::string key, uint64_t millisecond);
    // 获取键对应的值
    int getK(std::string key, std::vector<std::string>& res);
    // 使用正则表达式获取当前存储中的键
    int getKeyName(std::string keyRegex, std::vector<std::string>& res);
    // 设置kv流
    int setKVStream(std::vector<std::string> keyVec, std::vector<std::string> valVec, std::vector<uint32_t> ecVec);
    
private:
    // 服务器桩
    std::unique_ptr<kv::KVServer_Stub> stub_;
    // 配置文件地址
    std::string config_path_;
};


#endif