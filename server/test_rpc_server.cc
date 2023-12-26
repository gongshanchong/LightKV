#include <assert.h>
#include <cstdio>
#include <sys/socket.h>
#include <fcntl.h>
#include <string.h>
#include <pthread.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <string>
#include <memory>
#include <unistd.h>
#include <google/protobuf/service.h>
#include "../lightrpc/net/tcp/tcp_server.h"

#include "service/order_service.h"

int main(int argc, char* argv[]) {
  if (argc != 2) {
    printf("Start test_rpc_server error, argc not 2 \n");
    printf("Start like this: \n");
    printf("./test_rpc_server ../conf/lightrpc_server.xml \n");
    return 0;
  }

  // 初始化日志和配置文件
  lightrpc::Config::SetGlobalConfig(argv[1]);
  lightrpc::Logger::InitGlobalLogger();

  lightrpc::TcpServer tcp_server(lightrpc::Config::GetGlobalConfig()->m_rpc_stubs_["browser"].addr_, 
  lightrpc::Config::GetGlobalConfig()->m_rpc_servers_["default"].addr_, 
  lightrpc::Config::GetGlobalConfig()->m_rpc_stubs_["browser"].protocal_, 
  lightrpc::Config::GetGlobalConfig()->m_rpc_stubs_["browser"].timeout_);
  // 依据配置文件中的服务的协议进行相关的操作
  std::shared_ptr<OrderService> service = std::make_shared<OrderService>();
  lightrpc::RpcDispatcher::GetRpcDispatcher()->RegisterService(service);

  tcp_server.Start();

  return 0;
}