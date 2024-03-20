/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2020-2020. All rights reserved.
 */

#ifndef HUAWEICLOUD_CPP_SDK_SIS_CLION_AUTHINFO_H
#define HUAWEICLOUD_CPP_SDK_SIS_CLION_AUTHINFO_H
#include "Utils.h"

namespace speech {
namespace huawei_asr {

class AuthInfo {
public:
    AuthInfo(){}

    AuthInfo(std::string myAk, std::string mySk, std::string myRegion, std::string myProjectId)
    {
        ak = myAk;
        sk = mySk;
        region = myRegion;
        projectId = myProjectId;
    }
    AuthInfo(std::string myAk, std::string mySk, std::string myRegion, std::string myProjectId, std::string myEndpoint)
    {
        ak = myAk;
        sk = mySk;
        region = myRegion;
        projectId = myProjectId;
        endpoint = myEndpoint;
    }
    std::string GetAk()
    {
        return ak;
    }
    std::string GetSk()
    {
        return sk;
    }
    std::string GetRegion()
    {
        return region;
    }
    std::string GetProjectId()
    {
        return projectId;
    }
    std::string GetEndpoint()
    {
        if (!endpoint.empty()) {
            return endpoint;
        }
        std::string result = "sis-ext." + region + ".myhuaweicloud.com";
        return result;
    }
private:
    std::string ak;
    std::string sk;
    std::string region;
    std::string projectId;
    std::string endpoint;
};

}
}

#endif // HUAWEICLOUD_CPP_SDK_SIS_CLION_AUTHINFO_H
