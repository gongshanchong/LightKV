#ifndef MAKE_ORDER_H
#define MAKE_ORDER_H

#include "lightrpc/net/rpc/rpc_interface.h"
#include "lightrpc/common/exception.h"
#include "../../proto/test_rpc/order.pb.h"
#include <cstdio>
#include <string>

class MakeOrderImpl : public lightrpc::RpcInterface{
public:
    MakeOrderImpl(const google::protobuf::Message* req, google::protobuf::Message* rsp, google::protobuf::RpcController* controller, ::google::protobuf::Closure* done):
    lightrpc::RpcInterface(req, rsp, controller, done){}

    void Run(){
        try {
            // 业务逻辑代码
            LOG_APPDEBUG("start sleep 5s");
            sleep(5);
            LOG_APPDEBUG("end sleep 5s");
            auto resquest = dynamic_cast<const makeOrderRequest*>(m_req_base_);
            auto response = dynamic_cast<makeOrderResponse*>(m_rsp_base_);
            if (resquest->price() < 10) {
                response->set_ret_code(-1);
                response->set_res_info("short balance");
                return;
            }
            response->set_order_id("20230514");
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