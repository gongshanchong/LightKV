#ifndef GET_KV_H
#define GET_KV_H

#include "lightrpc/net/rpc/rpc_interface.h"
#include "lightrpc/common/exception.h"
#include "../../proto/kvserver/kvserver.pb.h"
#include "../src/db.h"
#include <cstdio>
#include <string>

class GetKVImpl : public lightrpc::RpcInterface{
public:
    GetKVImpl(const google::protobuf::Message* req, google::protobuf::Message* rsp, google::protobuf::RpcController* controller, ::google::protobuf::Closure* done):
    lightrpc::RpcInterface(req, rsp, controller, done){}

    void Run(){
        try {
            // 初始化请求和响应
            auto req = dynamic_cast<const kv::ReqK*>(m_req_base_);
            auto res = dynamic_cast<kv::GetKResponse*>(m_rsp_base_);
            // 获取请求
            std::string key = req->key();
            // 执行请求并构建响应
            std::vector<std::string> v;
            LightKVDB::getInstance()->get(key, v);
            if (v.empty()) {
                res->set_flag(false);
            }
            res->set_flag(true);
            for (auto& str : v) {
                res->add_val(str);
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
