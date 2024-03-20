/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2020-2020. All rights reserved.
 */
#ifndef HUAWEICLOUD_CPP_SDK_SIS_WEBSOCKET_CLIENT_H
#define HUAWEICLOUD_CPP_SDK_SIS_WEBSOCKET_CLIENT_H

#include "Signer.h"
#include "WebsocketService.h"

namespace speech {
namespace huawei_asr {

using websocketpp::lib::bind;

class WebsocketClient {

public:
    WebsocketService::ptr getWebsocketServicePtr() {
        return websocketServicePtr;
    }

    void setWebsocketServicePtr(WebsocketService::ptr websocketService) {
        websocketServicePtr = websocketService;
    }

    WebsocketClient();
    ~WebsocketClient();

    bool Connect(std::string const &url, std::map<std::string, std::string> headers);

    bool Close(std::string reason = "");

    bool SendTxt(std::string message);

    bool SendBinary(unsigned char *buff, size_t size);

private:
    WebsocketService::ptr websocketServicePtr;
    websocketpp::connection_hdl hdl;
    client mClient;
    websocketpp::lib::shared_ptr<websocketpp::lib::thread> mThread;
};

}
}

#endif //HUAWEICLOUD_CPP_SDK_SIS_WEBSOCKET_CLIENT_H
