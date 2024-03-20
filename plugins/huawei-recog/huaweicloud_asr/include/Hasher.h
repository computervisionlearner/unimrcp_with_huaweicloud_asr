/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2020-2020. All rights reserved.
 */
#ifndef SRC_HASHER_H_
#define SRC_HASHER_H_

#include "Utils.h"

namespace speech {
namespace huawei_asr {
class Hasher {
public:
    Hasher();
    const std::string hexEncode(unsigned char *md, size_t len);
    int hashSHA256(const std::string str, unsigned char *hash);
    unsigned char *hmac(void *key, unsigned int keyLength, std::string data);
};
}
}


#endif /* SRC_HASHER_H_ */
