#include "kvclient.h"
#include <cstddef>
#include <lightrpc/common/log.h>
#include <memory>

KVClient::KVClient(std::string config_path) : config_path_(config_path) {
    // 初始化日志和配置文件
    lightrpc::Config::SetGlobalConfig(config_path_.c_str());
    lightrpc::Logger::InitGlobalLogger();
    // 由地址获取通信通道
    std::unique_ptr<lightrpc::RpcChannel> channel = std::make_unique<lightrpc::RpcChannel>(lightrpc::Config::GetGlobalConfig()->m_rpc_servers_["zookeeper"].addr_);
    // 建立服务器桩便于服务的调用
    stub_ = std::make_unique<kv::KVServer_Stub>(channel.get());
}

int KVClient::setKV(std::string key, std::string val, uint32_t encoding) {
    // 健为空，则返回
    if (key.empty()) return LightKV_KEY_EMPTY;
    // 构建请求和响应
    NEWMESSAGE(kv::ReqKV, req);
    NEWMESSAGE(kv::SetKVResponse, res);
    req->set_key(key);
    req->set_val(val);
    req->set_encoding(encoding);
    // 通过lightrpc进行服务端的服务调用
    // 相关辅助参数设置
    NEWRPCCONTROLLER(controller);
    controller->SetMsgId(lightrpc::MsgIDUtil::GenMsgID());
    controller->SetTimeout(10000);
    controller->SetProtocol(lightrpc::ProtocalType::TINYPB);
    // 远程调用，可以通过controller来进行相关的控制（如连接超时时间、错误、调用完成。。。）
    stub_->SetKV(controller.get(), req.get(), res.get(), nullptr);
    // 执行业务逻辑
    if (controller->GetErrorCode() == 0){
        LOG_APPINFO("call rpc success, request[%s], response[%s]", req->ShortDebugString().c_str(), res->ShortDebugString().c_str());
        return LightKV_SET_SUCCESS;
    }else{
        LOG_APPERROR("call rpc failed, request[%s], error code[%d], error info[%s]", req->ShortDebugString().c_str(), controller->GetErrorCode(), controller->GetInfo().c_str());
        return LightKV_SET_FAIL;
    }
}

int KVClient::setKVStream(std::vector<std::string> keyVec, std::vector<std::string> valVec, std::vector<uint32_t> ecVec) {
    // 健为空，则返回
    if (keyVec.empty()) return LightKV_KEY_EMPTY;
    // 长度不对等，则返回
    if (keyVec.size() != valVec.size() || ecVec.size() != valVec.size()) return LightKV_SET_FAIL;
    // 通过lightrpc进行服务端的服务调用
    // 构建请求和响应
    NEWMESSAGE(kv::StreamtreamReqKV, stream_req);
    NEWMESSAGE(kv::StreamSetKVResponse, stream_res);
    // 写入请求
    for (int i = 0; i < keyVec.size(); ++i) {
        kv::ReqKV* req = stream_req->add_stream_req();
        req->set_key(keyVec[i]);
        req->set_val(valVec[i]);
        req->set_encoding(ecVec[i]);
    }
    // 通过lightrpc进行服务端的服务调用
    // 相关辅助参数设置
    NEWRPCCONTROLLER(controller);
    controller->SetMsgId(lightrpc::MsgIDUtil::GenMsgID());
    controller->SetTimeout(10000);
    controller->SetProtocol(lightrpc::ProtocalType::TINYPB);
    // 远程调用，可以通过controller来进行相关的控制（如连接超时时间、错误、调用完成。。。）
    stub_->SetKVStream(controller.get(), stream_req.get(), stream_res.get(), nullptr);
    // 执行业务逻辑
    if (controller->GetErrorCode() != 0){
        LOG_APPERROR("call rpc failed, request[%s], error code[%d], error info[%s]", stream_req->ShortDebugString().c_str(), controller->GetErrorCode(), controller->GetInfo().c_str());
        return LightKV_SET_FAIL;
    }
    // 获取响应
    for (int i = 0; i < stream_res->stream_res_size(); i++){
        // 取第i个res
        const kv::SetKVResponse& res = stream_res->stream_res(i);
        if (res.flag() == false) {
            LOG_APPERROR("call rpc failed, request[%s], error code[%d], error info[%s]", stream_req->ShortDebugString().c_str(), controller->GetErrorCode(), controller->GetInfo().c_str());
            return LightKV_SET_FAIL;
        }
    }
    LOG_APPINFO("call rpc success, request[%s], response[%s]", stream_req->ShortDebugString().c_str(), stream_res->ShortDebugString().c_str());
    return LightKV_SET_SUCCESS;
}

int KVClient::delK(std::string key) {
    // 健为空，则返回
    if (key.empty()) return LightKV_KEY_EMPTY;
    // 构建请求和响应
    NEWMESSAGE(kv::ReqK, req);
    NEWMESSAGE(kv::DelKVResponse, res);
    req->set_key(key);
    // 通过lightrpc进行服务端的服务调用
    // 相关辅助参数设置
    NEWRPCCONTROLLER(controller);
    controller->SetMsgId(lightrpc::MsgIDUtil::GenMsgID());
    controller->SetTimeout(10000);
    controller->SetProtocol(lightrpc::ProtocalType::TINYPB);
    // 远程调用，可以通过controller来进行相关的控制（如连接超时时间、错误、调用完成。。。）
    stub_->DelKV(controller.get(), req.get(), res.get(), nullptr);
    // 执行业务逻辑
    if (controller->GetErrorCode() == 0){
        LOG_APPINFO("call rpc success, request[%s], response[%s]", req->ShortDebugString().c_str(), res->ShortDebugString().c_str());
        return LightKV_DEL_SUCCESS;
    }else{
        LOG_APPERROR("call rpc failed, request[%s], error code[%d], error info[%s]", req->ShortDebugString().c_str(), controller->GetErrorCode(), controller->GetInfo().c_str());
        return LightKV_DEL_FAIL;
    }
}

int KVClient::getK(std::string key, std::vector<std::string>& ans) {
    // 健为空，则返回
    if (key.empty()) return LightKV_KEY_EMPTY;
    // 构建请求和响应
    NEWMESSAGE(kv::ReqK, req);
    NEWMESSAGE(kv::GetKResponse, res);
    req->set_key(key);
    // 通过lightrpc进行服务端的服务调用
    // 相关辅助参数设置
    NEWRPCCONTROLLER(controller);
    controller->SetMsgId(lightrpc::MsgIDUtil::GenMsgID());
    controller->SetTimeout(10000);
    controller->SetProtocol(lightrpc::ProtocalType::TINYPB);
    // 远程调用，可以通过controller来进行相关的控制（如连接超时时间、错误、调用完成。。。）
    stub_->GetKV(controller.get(), req.get(), res.get(), nullptr);
    // 执行业务逻辑
    if (controller->GetErrorCode() != 0){
        LOG_APPERROR("call rpc failed, request[%s], error code[%d], error info[%s]", req->ShortDebugString().c_str(), controller->GetErrorCode(), controller->GetInfo().c_str());
        ans = {};
        return LightKV_GET_FAIL;
    }
    LOG_APPINFO("call rpc success, request[%s], response[%s]", req->ShortDebugString().c_str(), res->ShortDebugString().c_str());
    // 获取从服务端获取的值
    auto p = res->val();
    for (auto it = p.begin(); it != p.end(); ++it) {
        ans.push_back(it->data());
    }
    return LightKV_GET_SUCCESS;
}

int KVClient::setExpires(std::string key, uint64_t millisecond) {
    // 健为空，则返回
    if (key.empty()) return LightKV_KEY_EMPTY;
    // 设置超时时间
    auto now = std::chrono::system_clock::now(); 
    auto timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count();
    uint64_t expires = timestamp + millisecond;
    // 构建请求和响应
    NEWMESSAGE(kv::ReqExpire, req);
    NEWMESSAGE(kv::SetExpireResponse, res);
    req->set_key(key);
    req->set_expires(expires);
    // 通过lightrpc进行服务端的服务调用
    // 相关辅助参数设置
    NEWRPCCONTROLLER(controller);
    controller->SetMsgId(lightrpc::MsgIDUtil::GenMsgID());
    controller->SetTimeout(10000);
    controller->SetProtocol(lightrpc::ProtocalType::TINYPB);
    // 远程调用，可以通过controller来进行相关的控制（如连接超时时间、错误、调用完成。。。）
    stub_->SetExpire(controller.get(), req.get(), res.get(), nullptr);
    // 执行业务逻辑
    if (controller->GetErrorCode() == 0){
        LOG_APPINFO("call rpc success, request[%s], response[%s]", req->ShortDebugString().c_str(), res->ShortDebugString().c_str());
        return LightKV_SET_EXPIRE_SUCCESS;
    }else{
        LOG_APPERROR("call rpc failed, request[%s], error code[%d], error info[%s]", req->ShortDebugString().c_str(), controller->GetErrorCode(), controller->GetInfo().c_str());
        return LightKV_SET_EXPIRE_FAIL;
    }
}

int KVClient::getKeyName(std::string key, std::vector<std::string>& ans) {
    // 健为空，则返回
    if (key.empty()) return LightKV_KEY_EMPTY;
    // 构建请求和响应
    NEWMESSAGE(kv::ReqKeyName, req);
    NEWMESSAGE(kv::GetKeyNameResponse, res);
    req->set_keyrex(key);
    // 通过lightrpc进行服务端的服务调用
    // 相关辅助参数设置
    NEWRPCCONTROLLER(controller);
    controller->SetMsgId(lightrpc::MsgIDUtil::GenMsgID());
    controller->SetTimeout(10000);
    controller->SetProtocol(lightrpc::ProtocalType::TINYPB);
    // 远程调用，可以通过controller来进行相关的控制（如连接超时时间、错误、调用完成。。。）
    stub_->GetKeyName(controller.get(), req.get(), res.get(), nullptr);
    // 执行业务逻辑
    if (controller->GetErrorCode() != 0){
        LOG_APPERROR("call rpc failed, request[%s], error code[%d], error info[%s]", req->ShortDebugString().c_str(), controller->GetErrorCode(), controller->GetInfo().c_str());
        ans = {};
        return LightKV_GET_KEYNAME_FAIL;
    }
    LOG_APPINFO("call rpc success, request[%s], response[%s]", req->ShortDebugString().c_str(), res->ShortDebugString().c_str());
    // 获取从服务端获取的键的内容
    auto p = res->val();
    for (auto it = p.begin(); it != p.end(); ++it) {
        ans.push_back(it->data());
    }
    return LightKV_GET_KEYNAME_SUCCESS;
}