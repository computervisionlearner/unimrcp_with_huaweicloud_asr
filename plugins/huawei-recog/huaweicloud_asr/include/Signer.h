/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2020-2020. All rights reserved.
 */
#ifndef SRC_SIGNER_H_
#define SRC_SIGNER_H_

#include "Utils.h"
#include "Header.h"
#include "Hasher.h"
#include "RequestParams.h"

namespace speech {
namespace huawei_asr {
class Signer {
public:
    Signer();
    Signer(std::string appKey, std::string appSecret);
    ~Signer();

    /* Task 1: Get Canonicalized Request String*/
    const std::string getCanonicalRequest(std::string signedHeaders, std::string method, std::string uri,
        std::string query, const std::set<Header>* headers,
        std::string payload);

    const std::string getCanonicalURI(std::string& uri);
    const std::string getCanonicalQueryString(
        std::map<std::string, std::string>& queryParams);
    const std::string getCanonicalQueryString(
        std::map<std::string, std::vector<std::string>>& queryParams);
    const std::string getCanonicalQueryString(std::string& queryParams);
    const std::string getCanonicalHeaders(const std::set<Header>* headers);
    const std::string getSignedHeaders(const std::set<Header>* headers);
    const std::string getHexHash(std::string& payload);

    /* Task 2:　Get String to Sign */
    const std::string getStringToSign(std::string algorithm, std::string date,
        std::string canonicalRequest);

    /* Task 3: Calculate the Signature */
    const std::string getSignature(char * signingKey, const std::string& stringToSign);

    // One stroke create signature
    const std::string createSignature(RequestParams * request);
private:
    /* Credentials */
    std::string mAppKey;
    std::string mAppSecret;
    Hasher* hasher;
};

}
}

#endif /* SRC_SIGNER_H_ */
