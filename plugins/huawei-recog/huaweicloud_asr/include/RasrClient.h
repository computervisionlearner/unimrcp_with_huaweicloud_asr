/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2020-2020. All rights reserved.
 */
#ifndef HUAWEICLOUD_CPP_SDK_SIS_RASR_CLIENT_H
#define HUAWEICLOUD_CPP_SDK_SIS_RASR_CLIENT_H

#include "Utils.h"
#include "Signer.h"
#include "AuthInfo.h"
#include "WebsocketClient.h"
#include "RasrRequest.h"
#include "HttpConfig.h"


namespace speech {
namespace huawei_asr {

class RasrClient {
public:
    RasrClient(AuthInfo authinfo, WebsocketService::ptr servicePtr, HttpConfig config)
    {
        authInfo = authinfo;
        httpConfig = config;
        websocketServicePtr = servicePtr;
        ws.setWebsocketServicePtr(websocketServicePtr);
    }

    ~RasrClient(void) {};

    void ShortStreamConnect();

    void ContinueStreamConnect();

    void SentenceStreamConnect();

    void SendStart(RasrRequest request);

    void SendEnd();

    void SendBinary(unsigned char *buff, size_t size);

    void SendBinary(unsigned char *buff, size_t size, int byteLen, int sleepTime);

    void Close();
private:
    AuthInfo authInfo;
    WebsocketClient ws;
    HttpConfig httpConfig;
    WebsocketService::ptr websocketServicePtr;

    void Connect(std::string api);
    bool CheckStart();
    bool CheckBinary();
    bool CheckEnd();
    bool CheckConnect();
    void WaitStatus(std::set<WebsocketStatus> targetStatuses, int maxWaitTime);
};

}
}

#endif // HUAWEICLOUD_CPP_SDK_SIS_RASR_CLIENT_H
