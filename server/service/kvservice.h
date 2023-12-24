#include "../../proto/kvserver/kvserver.pb.h"
#include "../interface/set_kv.h"
#include "../interface/get_kv.h"
#include "../interface/del_kv.h"
#include "../interface/set_expire.h"
#include "../interface/get_keyname.h"
#include "../interface/set_stream.h"

class KVService final : public kv::KVServer {
public:
    void SetKV(google::protobuf::RpcController* controller, const ::kv::ReqKV* request, ::kv::SetKVResponse* response, ::google::protobuf::Closure* done){
        std::shared_ptr<SetKVImpl> impl = std::make_shared<SetKVImpl>(request, response, controller, done);
        impl->Run();
    }
    void GetKV(google::protobuf::RpcController*controller, const ::kv::ReqK* request, ::kv::GetKResponse* response, ::google::protobuf::Closure* done){
        std::shared_ptr<GetKVImpl> impl = std::make_shared<GetKVImpl>(request, response, controller, done);
        impl->Run();
    }
    void DelKV(google::protobuf::RpcController*controller, const ::kv::ReqK* request, ::kv::DelKVResponse* response, ::google::protobuf::Closure* done){
        std::shared_ptr<DelKVImpl> impl = std::make_shared<DelKVImpl>(request, response, controller, done);
        impl->Run();
    }
    void SetExpire(google::protobuf::RpcController*controller, const ::kv::ReqExpire* request, ::kv::SetExpireResponse* response, ::google::protobuf::Closure* done){
        std::shared_ptr<SetExpireImpl> impl = std::make_shared<SetExpireImpl>(request, response, controller, done);
        impl->Run();
    }
    void GetKeyName(google::protobuf::RpcController*controller, const ::kv::ReqKeyName* request, ::kv::GetKeyNameResponse* response, ::google::protobuf::Closure* done){
        std::shared_ptr<GetKeyNameImpl> impl = std::make_shared<GetKeyNameImpl>(request, response, controller, done);
        impl->Run();
    }
    void SetKVStream(google::protobuf::RpcController*controller, const ::kv::StreamtreamReqKV* request, ::kv::StreamSetKVResponse* response, ::google::protobuf::Closure* done){
        std::shared_ptr<SetStreamImpl> impl = std::make_shared<SetStreamImpl>(request, response, controller, done);
        impl->Run();
    }
};