#ifndef GET_KEYNAME_H
#define GET_KEYNAME_H

#include "lightrpc/net/rpc/rpc_interface.h"
#include "lightrpc/common/exception.h"
#include "../../proto/kvserver/kvserver.pb.h"
#include "../src/db.h"
#include <cstdio>
#include <string>

class GetKeyNameImpl : public lightrpc::RpcInterface{
public:
    GetKeyNameImpl(const google::protobuf::Message* req, google::protobuf::Message* rsp, google::protobuf::RpcController* controller, ::google::protobuf::Closure* done):
    lightrpc::RpcInterface(req, rsp, controller, done){}

    void Run(){
        try {
            // 初始化请求和响应
            auto req = dynamic_cast<const kv::ReqKeyName*>(m_req_base_);
            auto res = dynamic_cast<kv::GetKeyNameResponse*>(m_rsp_base_);
            // 获取请求
            std::string keyRex = req->keyrex();
            std::vector<std::string> ans;
            // 执行请求并构建响应
            LightKVDB::getInstance()->getKeyName(keyRex, ans);
            for (auto& key : ans) {
                res->add_val(key);
            }
        }catch (lightrpc::LightrpcException& e) {
            LOG_APPERROR("LightrpcException exception[%s], deal handle", e.what());
            e.handle();
            SetError(e.errorCode(), e.errorInfo());
        } catch (std::exception& e) {
            LOG_APPERROR("std::exception[%s], deal handle", e.what());
            SetError(-1, "unkonwn std::exception");
        } catch (...) {
            LOG_APPERROR("Unkonwn exception, deal handle");
            SetError(-1, "unkonwn exception");
        }
    }

    void SetError(int code, const std::string& err_info){
        m_controller_->SetError(code, err_info);
    }
};
#endif
