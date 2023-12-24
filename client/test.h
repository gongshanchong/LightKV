#ifndef TEST_H
#define TEST_H
#include "kvclient.h"

inline void testExpire(KVClient* c) {
    int res = c->setKV("yunfei", "23", LightKV_STRING);
    if (res == LightKV_SET_SUCCESS) {
        fprintf(stdout, "set key success!\n");
    } else {
        fprintf(stderr, "set key failed!\n");
        exit(1);
    }
    std::vector<std::string> ans;
    c->getK("yunfei", ans);
    std::cout << ans[0] << std::endl;
    res = c->setExpires("yunfei", 1000);
    if (res == LightKV_SET_EXPIRE_SUCCESS) {
        fprintf(stdout, "set expire success!\n");
    } else {
        fprintf(stderr, "set expire failed!\n");
        exit(1);
    }
    sleep(3);
    c->getK("yunfei", ans);
    std::cout << (ans.empty() ? "expired" : "expire doesn't work") << std::endl;
}

inline void testFixedTimeDelKeyExpired(KVClient* c) {
    int res = c->setKV("yunfei", "23", LightKV_STRING);
    if (res == LightKV_SET_SUCCESS) {
        fprintf(stdout, "set key success!\n");
    } else {
        fprintf(stderr, "set key failed!\n");
        exit(1);
    }
    std::vector<std::string> ans;
    c->getK("yunfei", ans);
    std::cout << ans[0] << std::endl;
    res = c->setExpires("yunfei", 1000);
    if (res == LightKV_SET_EXPIRE_SUCCESS) {
        fprintf(stdout, "set expire success!\n");
    } else {
        fprintf(stderr, "set expire failed!\n");
        exit(1);
    }
    // dont use get to start the lazy delete mode
    // debug server to judge whether the key is trying to delete
}

inline void testSetLists(KVClient* c) {
    for (int i = 0; i < 100; ++i) {
        int res = c->setKV("yunfei", "23", LightKV_LIST);
        if (res == LightKV_SET_SUCCESS) {
            fprintf(stdout, "set key success!\n");
        } else {
            fprintf(stderr, "set key failed!\n");
            exit(1);
        }
    }
    std::vector<std::string> ans;
    c->getK("yunfei", ans);
    for (int i = 0; i < ans.size(); ++i) {
        std::cout << ans[i] << " ";
    } std::cout << std::endl;
}

inline void testSetStream(KVClient* c) {
    std::vector<std::string> keyVec = {"topic1", "topic2", "topic3"};
    std::vector<std::string> valVec = {"1", "2", "3"};
    std::vector<uint32_t> ecVec = {LightKV_STRING, LightKV_STRING, LightKV_STRING};
    c->setKVStream(keyVec, valVec, ecVec);
}

#endif