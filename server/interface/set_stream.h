#ifndef SET_STREAM_H
#define SET_STREAM_H

#include "lightrpc/net/rpc/rpc_interface.h"
#include "lightrpc/common/exception.h"
#include "../../proto/kvserver/kvserver.pb.h"
#include "../src/db.h"
#include <cstdio>
#include <string>

class SetStreamImpl : public lightrpc::RpcInterface{
public:
    SetStreamImpl(const google::protobuf::Message* req, google::protobuf::Message* rsp, google::protobuf::RpcController* controller, ::google::protobuf::Closure* done):
    lightrpc::RpcInterface(req, rsp, controller, done){}

    void Run(){
        try {
            // 初始化请求和响应
            auto stream_req = dynamic_cast<const kv::StreamtreamReqKV*>(m_req_base_);
            auto stream_res = dynamic_cast<kv::StreamSetKVResponse*>(m_rsp_base_);

            // 获取请求
            for (int i = 0; i < stream_req->stream_req_size(); i++){
                // 取第i个req
                const kv::ReqKV& req = stream_req->stream_req(i);
                std::string key = req.key();
                std::string val = req.val();
                uint32_t encoding = req.encoding();
                // 执行请求并构建响应
                LightKVDB::getInstance()->insert(key, val, encoding);
                kv::SetKVResponse* res = stream_res->add_stream_res();
                res->set_flag(true);
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
