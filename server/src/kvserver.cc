#include "kvserver.h"
#include <memory>

KVServer::KVServer(std::string config_path) : config_path_(config_path){
    // 初始化日志和配置文件
    lightrpc::Config::SetGlobalConfig(config_path_.c_str());
    lightrpc::Logger::InitGlobalLogger();
    LightKVDB::getInstance()->init();
    kvService_ = std::shared_ptr<KVService>(new KVService());
}

void KVServer::serve() {
    // 绑定地址
    std::unique_ptr<lightrpc::TcpServer> server_ = std::make_unique<lightrpc::TcpServer>(lightrpc::Config::GetGlobalConfig()->m_rpc_stubs_["default"].addr_, 
    lightrpc::Config::GetGlobalConfig()->m_rpc_servers_["default"].addr_, 
    lightrpc::Config::GetGlobalConfig()->m_rpc_stubs_["default"].protocal_, 
    lightrpc::Config::GetGlobalConfig()->m_rpc_stubs_["default"].timeout_);
    // 注册服务
    lightrpc::RpcDispatcher::GetRpcDispatcher()->RegisterService(kvService_);
    // 构建服务端
    server_->Start();
}