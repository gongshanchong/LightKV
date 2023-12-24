cmake_minimum_required(VERSION 3.5.1)

project(LightKV C CXX)

set(CMAKE_BUILD_TYPE "Release")
set(CMAKE_CXX_STANDARD 17)

# 查找 protobuf
find_package(Protobuf REQUIRED)
if (PROTOBUF_FOUND)
    message("-- protobuf found")
else ()
    message(FATAL_ERROR "-- Cannot find Protobuf")
endif ()

# 设置proto静态库
set(proto_test_rpc_srcs "${CMAKE_CURRENT_BINARY_DIR}/../proto/test_rpc/order.pb.cc")
set(proto_test_rpc_hdrs "${CMAKE_CURRENT_BINARY_DIR}/../proto/test_rpc/order.pb.h")
set(proto_kvserver_srcs "${CMAKE_CURRENT_BINARY_DIR}/../proto/kvserver/kvserver.pb.cc")
set(proto_kvserver_hdrs "${CMAKE_CURRENT_BINARY_DIR}/../proto/kvserver/kvserver.pb.h")

include_directories("${CMAKE_CURRENT_BINARY_DIR}")

add_library(test_rpc_proto ${proto_test_rpc_srcs} ${proto_test_rpc_hdrs})
add_library(kvserver_proto ${proto_kvserver_srcs} ${proto_kvserver_hdrs})
target_link_libraries(test_rpc_proto protobuf)
target_link_libraries(kvserver_proto protobuf)

set(test_rpc_server_src "${CMAKE_CURRENT_BINARY_DIR}/../server/test_rpc_server.cc")
set(test_rpc_client_src "${CMAKE_CURRENT_BINARY_DIR}/../client/test_rpc_client.cc")
set(kvctl_src "${CMAKE_CURRENT_BINARY_DIR}/../client/kvclient.cc;${CMAKE_CURRENT_BINARY_DIR}/../kvctl/main.cc")
file(GLOB_RECURSE server_src "${CMAKE_CURRENT_BINARY_DIR}/../server/src/*.cc")
set(kvserver_src "${server_src};${CMAKE_CURRENT_BINARY_DIR}/../server/server_main.cc")
set(kvclient_src "${CMAKE_CURRENT_BINARY_DIR}/../client/kvclient.cc;${CMAKE_CURRENT_BINARY_DIR}/../client/client_main.cc")

add_executable(test_rpc_server ${test_rpc_server_src})
add_executable(test_rpc_client ${test_rpc_client_src})
add_executable(kvctl    ${kvctl_src})
add_executable(kvserver_main ${kvserver_src})
add_executable(kvclient_main ${kvclient_src})

target_link_libraries(test_rpc_server test_rpc_proto lightrpc tinyxml protobuf zookeeper_mt)
target_link_libraries(test_rpc_client test_rpc_proto lightrpc tinyxml protobuf zookeeper_mt)
target_link_libraries(kvctl kvserver_proto gflags lightrpc tinyxml protobuf zookeeper_mt)
target_link_libraries(kvserver_main kvserver_proto lightrpc tinyxml protobuf zookeeper_mt)
target_link_libraries(kvclient_main kvserver_proto lightrpc tinyxml protobuf zookeeper_mt)