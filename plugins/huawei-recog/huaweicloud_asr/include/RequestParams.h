/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2020-2020. All rights reserved.
 */
#ifndef SRC_REQUESTPARAMS_H_
#define SRC_REQUESTPARAMS_H_

#include "Utils.h"
#include "Header.h"

namespace speech {
namespace huawei_asr {
class RequestParams {
public:
    RequestParams();
    RequestParams(std::string method, std::string host, std::string uri,
        std::string queryParams);
    RequestParams(std::string method, std::string host, std::string uri,
        std::string queryParams, std::string payload);
    virtual ~RequestParams();

    const std::string getMethod();
    const std::string getHost();
    const std::string getUri();
    const std::string getQueryParams();
    const std::string getPayload();
    const std::set<Header>* getHeaders();

    void addHeader(Header& header);
    void addHeader(std::string key, std::string value);
    std::string initHeaders();
private:
    /* HTTP Request Parameters */
    std::string mMethod;
    std::string mHost;
    std::string mUri;
    std::string mQueryParams;
    std::string mPayload;

    std::set<Header> mHeaders;

};
}
}

#endif /* SRC_REQUESTPARAMS_H_ */
