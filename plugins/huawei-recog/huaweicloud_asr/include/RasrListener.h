/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2020-2020. All rights reserved.
 */
#ifndef HUAWEICLOUD_CPP_SDK_SIS_CLION_RASRLISTENER_H
#define HUAWEICLOUD_CPP_SDK_SIS_CLION_RASRLISTENER_H

#include "Utils.h"

class RasrListener {
public:
    void virtual OnConnect() {
        std::cout << "base rasr Connect success";;
    }

    void virtual OnStart(std::string text) {
        std::cout << "base rasr receive start response " << text;
    }

    void virtual OnResp(std::string text) {
        // text encoded by utf-8 contains chinese character, which will cause error code. So we should convert to ansi
        // cout << "rasr receive " << text << endl;
        std::cout << "base rasr receive " << text;
    }

    void virtual OnEnd(std::string text) {
        std::cout << "base rasr receive end response " << text;
    }

    void virtual OnClose() {
        std::cout << "base rasr receive Close";
    }

    void virtual OnError(std::string text) {
        std::cout << "base rasr receive error" << text;
    }

    void virtual OnEvent(std::string text) {
        std::cout << "base rasr receive event" << text;
    }


};


#endif //HUAWEICLOUD_CPP_SDK_SIS_CLION_RASRLISTENER_H
