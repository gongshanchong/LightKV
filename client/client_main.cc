#include <iostream>
#include <memory>
#include "kvclient.h"
#define DEBUG

#ifdef DEBUG
#include "test.h"
#endif

int main(int argc, char** argv) {
    std::shared_ptr<KVClient> kvclient = std::make_shared<KVClient>("../conf/lightrpc_client.xml");
    // 进行测试
    testExpire(kvclient.get());
    // testFixedTimeDelKeyExpired(kvclient);

    return 0;
}