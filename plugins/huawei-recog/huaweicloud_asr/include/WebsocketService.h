/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2020-2020. All rights reserved.
 */
#ifndef HUAWEICLOUD_CPP_SDK_SIS_WEBSOCKETSERVICE_H
#define HUAWEICLOUD_CPP_SDK_SIS_WEBSOCKETSERVICE_H

#include "RasrListener.h"

#include <websocketpp/config/asio_client.hpp>
#include <websocketpp/client.hpp>
#include <websocketpp/common/thread.hpp>
#include <websocketpp/common/memory.hpp>

namespace speech {
namespace huawei_asr {

using websocketpp::lib::bind;
typedef websocketpp::client<websocketpp::config::asio_tls_client> client;
typedef websocketpp::lib::shared_ptr<websocketpp::lib::asio::ssl::context> context_ptr;
typedef websocketpp::config::asio_client::message_type::ptr message_ptr;

class WebsocketService {
public:
    typedef websocketpp::lib::shared_ptr<WebsocketService> ptr;
    void OnOpen(client *c, websocketpp::connection_hdl hdl);
    void OnFail(client *c, websocketpp::connection_hdl hdl);
    void OnClose(client *c, websocketpp::connection_hdl hdl);
    void OnMessage(websocketpp::connection_hdl, client::message_ptr msg);
    WebsocketStatus GetStatus() {
        return status_;
    }
    void SetStatus(WebsocketStatus newStatus) {
        status_ = newStatus;
    }
    void SetCallBack(RasrListener* callBack) {
        rasrListener_ = callBack;
    }
private:
    WebsocketStatus status_ = WB_PRE_START;
    RasrListener* rasrListener_;
    void ProcessMessage(std::string message);
    void ProcessConnect();
    void ProcessStart(std::string text);
    void ProcessEnd(std::string text);
    void ProcessError(std::string text);
    void ProcessClose();
    void ProcessResp(std::string text);
    void ProcessEvent(std::string text);

};

}
}

#endif //HUAWEICLOUD_CPP_SDK_SIS_WEBSOCKETSERVICE_H
