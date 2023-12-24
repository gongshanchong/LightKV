#ifndef KVSERVER_H
#define KVSERVER_H

#include <map>
#include <string>
#include <functional>
#include "lightrpc/net/tcp/tcp_server.h"
#include "../../proto/kvserver/kvserver.pb.h"
#include "../service/kvservice.h"
#include "db.h"

class KVServer {
private:
    // 配置文件地址
    std::string config_path_;
    // 构建服务
    std::shared_ptr<KVService> kvService_;
    // 与客户端通信的服务端
    std::unique_ptr<lightrpc::TcpServer> server_;
public:
    // 初始化配置文件地址
    KVServer(std::string config_path);
    // 构建服务端通信
    void serve();
};

#endif