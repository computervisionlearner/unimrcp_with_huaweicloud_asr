/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2020-2020. All rights reserved.
 */
#ifndef HUAWEICLOUD_CPP_SDK_SIS_CLION_HTTPCONFIG_H
#define HUAWEICLOUD_CPP_SDK_SIS_CLION_HTTPCONFIG_H

namespace speech {
namespace huawei_asr {

class HttpConfig {
public:
    HttpConfig(){}
    void SetConnectTimeout(int timeout)
    {
        connectTimeout = timeout;
    }
    void SetReadTimeout(int timeout)
    {
        readTimeout = timeout;
    }
    void SetRetryNum(int num)
    {
        retryNum = num;
    }
    int GetConnectTimeout()
    {
        return connectTimeout;
    }
    int GetReadTimeout()
    {
        return readTimeout;
    }
    int GetRetryNum()
    {
        return readTimeout;
    }
private:
    int connectTimeout = 20000;
    int readTimeout = 20000;
    int retryNum = 1;
};

}
}

#endif // HUAWEICLOUD_CPP_SDK_SIS_CLION_HTTPCONFIG_H
