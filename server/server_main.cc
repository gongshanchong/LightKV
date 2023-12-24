#include <iostream>
#include <memory>
#include <signal.h>
#include "src/kvserver.h"

int main(int argc, char* argv[]) {
    if (argc != 2) {
        printf("Start server_main error, argc not 2 \n");
        printf("Start like this: \n");
        printf("./kvserver_main ../conf/lightrpc_server.xml \n");
        return 0;
    }

    std::shared_ptr<KVServer> kvServer = std::make_shared<KVServer>(argv[1]);
    kvServer->serve();

    return 0;
}

