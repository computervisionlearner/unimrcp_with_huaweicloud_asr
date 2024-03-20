/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2020-2020. All rights reserved.
 */
#ifndef HUAWEICLOUD_CPP_SDK_SIS_CLION_AUTHUTIL_H
#define HUAWEICLOUD_CPP_SDK_SIS_CLION_AUTHUTIL_H

#include "Utils.h"
#include "Signer.h"
#include "AuthInfo.h"

namespace speech {

namespace huawei_asr {
std::map<std::string, std::string> SignHeaders(AuthInfo authInfo, std::string api,
    std::string queryParams, std::string body, std::string method);
}

}



#endif // HUAWEICLOUD_CPP_SDK_SIS_CLION_AUTHUTIL_H
