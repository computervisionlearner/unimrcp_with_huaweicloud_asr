/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2020-2020. All rights reserved.
 */
#ifndef RASR_REQUEST_H_
#define RASR_REQUEST_H_


#include "Utils.h"

namespace speech {
namespace huawei_asr {
class RasrRequest {
public:
    RasrRequest(std::string audioFormat, std::string audioProperty);

    void SetPunc(std::string punc);

    void SetDigitNorm(std::string digitNorm);

    void SetVadHead(int vadHead);

    void SetVadTail(int vadTail);

    void SetMaxSeconds(int maxSeconds);

    void SetIntermediateResult(std::string intermediateResult);

    void SetVocabularyId(std::string vocabularyId);

    void SetNeedWordInfo(std::string needWordInfo);

    std::string ConstructParams();

private:
    std::string audioFormat_;
    std::string audioProperty_;
    std::string punc_ = "no";
    std::string digitNorm_ = "no";
    
    int vadHead_ = 10000;
    int vadTail_ = 500;
    int maxSeconds_ = 30;
    std::string intermediateResult_ = "no";
    std::string vocabularyId_ = "";
    std::string needWordInfo_ = "no";
};
}
}
#endif
