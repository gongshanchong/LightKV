#include "gflags/gflags.h"
#include "../client/kvclient.h"
#include "draw_table.h"

DEFINE_string(config_path, "../conf/lightrpc_client.xml", "config path");
DEFINE_string(key, "", "key");
DEFINE_string(value, "", "value");
DEFINE_string(encoding, "string", "encoding, see in encoding.h");
DEFINE_string(operate, "set", "operate, e.g. set, del..");
DEFINE_uint64(expires, 10000, "expire time");
DEFINE_string(keyRex, "", "regex for key name");


DECLARE_string(config_path);
DECLARE_string(key);
DECLARE_string(value);
DECLARE_string(encoding);
DECLARE_string(operate);
DECLARE_uint64(expires);
DECLARE_string(keyRex);

#include <unordered_map>
// 设置命令对应的操作
std::unordered_map<std::string, int> oper {
    {"set", KV_SET},
    {"del", KV_DEL},
    {"get", KV_GET},
    {"expire", KV_EXPIRE},
    {"findkey", KV_KEY_FIND}
};

// 设置存储
std::unordered_map<std::string, int> enMap {
    {"string", LightKV_STRING},
    {"list", LightKV_LIST},
};

// 设置返回状态
std::unordered_map<int, std::string> enStatus;
void initResponseEncoding() {
    enStatus[LightKV_SET_SUCCESS] = "OK";
    enStatus[LightKV_GET_SUCCESS] = "OK";
    enStatus[LightKV_DEL_SUCCESS] = "OK";
    enStatus[LightKV_SET_EXPIRE_SUCCESS] = "OK";
    enStatus[LightKV_GET_KEYNAME_SUCCESS] = "OK";
    enStatus[LightKV_SET_FAIL] = "Fail!";
    enStatus[LightKV_GET_FAIL] = "Fail!";
    enStatus[LightKV_DEL_FAIL] = "Fail!";
    enStatus[LightKV_SET_EXPIRE_FAIL] = "Fail!";
    enStatus[LightKV_GET_KEYNAME_FAIL] = "Fail!";
}

// 在命令行中显示状态
void drawStatus(int ans) {
    std::vector<std::vector<std::string>> data;
    data.push_back({enStatus[ans]});
    std::vector<int> max;
    std::vector<std::string> head = {"Status"};
    maxLenForEveryCol(data, max, head);
    drawDatas(max, data, head, 1, 1);
}

void drawMultiEleOneCol(std::vector<std::string>& ans, std::string headName) {
    std::vector<std::vector<std::string>> data;
    for (int i = 0; i < ans.size(); ++i) {
        data.push_back({ans[i]});
    }
    std::vector<int> max;
    std::vector<std::string> head = {headName};
    maxLenForEveryCol(data, max, head);
    drawDatas(max, data, head, 1, ans.size());
}

// 从命令行接收命令并进行操作，最后在命令行返回操作结果
int main(int argc, char* argv[]) {
    // 从命令行获取命令
    gflags::ParseCommandLineFlags(&argc, &argv, true);
    // 新建客户端与服务端建立链接
    std::shared_ptr<KVClient> kvclient = std::make_shared<KVClient>(FLAGS_config_path);
    // 从命令中获取参数
    std::string encoding = FLAGS_encoding;
    std::string operation  = FLAGS_operate;
    std::string key = FLAGS_key;
    std::string val = FLAGS_value;
    std::string keyRex = FLAGS_keyRex;
    uint64_t expires = FLAGS_expires;
    initResponseEncoding();
    // 依据参数进行相应的参数
    switch (oper[operation]) {
        case KV_SET : {
            int ans = kvclient->setKV(key, val, enMap[encoding]);
            drawStatus(ans);
            break;
        }
        case KV_DEL : {
            int ans = kvclient->delK(key);
            drawStatus(ans);
            break;
        }
        case KV_GET : {
            std::vector<std::string> ans;
            kvclient->getK(key, ans);
            drawMultiEleOneCol(ans, "Value");
            break;
        }
        case KV_EXPIRE : {
            int ans = kvclient->setExpires(key, expires);
            drawStatus(ans);
            break;
        }
        case KV_KEY_FIND : {
            std::vector<std::string> ans;
            kvclient->getKeyName(keyRex, ans);
            if (ans.empty()) {
                ans.push_back("empty!");
            }
            drawMultiEleOneCol(ans, "Key");
            break;
        }
    }

    return 0;
}