#include "../../proto/test_rpc/order.pb.h"
#include "../interface/make_order.h"

class OrderService : public Order {
 public:
  void makeOrder(google::protobuf::RpcController* controller,
                      const ::makeOrderRequest* request,
                      ::makeOrderResponse* response,
                      ::google::protobuf::Closure* done) {
    std::shared_ptr<MakeOrderImpl> impl = std::make_shared<MakeOrderImpl>(request, response, controller, done);
    impl->Run();
  }
};
