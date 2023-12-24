#ifndef ENCODING_H
#define ENCODING_H

enum {
    LightKV_STRING = 1,
    LightKV_LIST,
    LightKV_SET_SUCCESS,
    LightKV_SET_FAIL,
    LightKV_GET_SUCCESS,
    LightKV_GET_FAIL,
    LightKV_DEL_FAIL,
    LightKV_DEL_SUCCESS,
    LightKV_SET_EXPIRE_SUCCESS,
    LightKV_SET_EXPIRE_FAIL,
    LightKV_KEY_EMPTY,
    LightKV_GET_KEYNAME_FAIL,
    LightKV_GET_KEYNAME_SUCCESS
};

enum {
    KV_SET = 100,
    KV_DEL,
    KV_GET,
    KV_EXPIRE,
    KV_KEY_FIND
};

#endif