/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2020-2020. All rights reserved.
 */

#include "Utils.h"
#include "RasrListener.h"
#include "RasrClient.h"
#include "gflags/gflags.h"

// auth info
// refer to https://support.huaweicloud.com/api-sis/sis_03_0051.html
DEFINE_string(ak, "", "access key");
DEFINE_string(sk, "", "secrect key");

// region, for example cn-east-3, cn-north-4
DEFINE_string(region, "cn-east-3", "project region, such as cn-east-3");
// projectId, refer to https://support.huaweicloud.com/api-sis/sis_03_0008.html
DEFINE_string(projectId, "", "project id");
// endpoint, relevant to region, sis-ext.${region}.myhuaweicloud.com
DEFINE_string(endpoint, "", "service endpoint");

DEFINE_string(audioFormat, "pcm16k16bit", "such pcm16k16bit alaw16k16bit etc.");
DEFINE_string(property, "chinese_16k_general", "");
DEFINE_string(audioPath, "xx.wav", "audio path");

DEFINE_int32(chunkSize, 3000, "bytes per send");
DEFINE_int32(sampleRate, 16000, "sample rate of audio");
DEFINE_int32(readTimeOut, 20000, "read time out, default 20s");
DEFINE_int32(connectTimeOut, 20000, "connecting time out, default 20s");
DEFINE_int32(bytesPerSecond, 32000, "32000 bytes per second");

class CallBack : public RasrListener {
public:
    void OnOpen()
    {
        LOG(INFO) << "now rasr Connect success";
    }
    void OnStart(std::string text)
    {
        LOG(INFO) << "now rasr receive start response: " << text;
    }

    void OnResp(std::string text)
    {
        // text encoded by utf-8 contains chinese character, which will cause error code. So we should convert to ansi
        LOG(INFO) << "user [" << userMeta_ << "] rasr receive " << text;
    }
    void OnEnd(std::string text)
    {
        LOG(INFO) << "now rasr receive end response: " << text;
    }
    void OnClose()
    {
        LOG(INFO) << "now rasr receive Close";
    }
    void OnError(std::string text)
    {
        LOG(INFO) << "now rasr receive error: " << text;
    }
    void OnEvent(std::string text)
    {
        LOG(INFO) << "now rasr receive event: " << text;
    }
    void SetUserInfo(const std::string &str)
    {
        userMeta_ = str;
    }

private:
    std::string userMeta_;
};

void RasrTest(const std::string filePath)
{
    const int sleepTime = FLAGS_bytesPerSecond / FLAGS_chunkSize;

    speech::huawei_asr::AuthInfo authInfo(FLAGS_ak, FLAGS_sk, FLAGS_region, FLAGS_projectId, FLAGS_endpoint);
    // config Connect parameter
    speech::huawei_asr::HttpConfig httpConfig;
    httpConfig.SetReadTimeout(FLAGS_readTimeOut);
    httpConfig.SetConnectTimeout(FLAGS_connectTimeOut);

    // config callback, callback function are optional, if not set, it will use function in RasrListener
    speech::huawei_asr::WebsocketService::ptr websocketServicePtr =
        websocketpp::lib::make_shared<speech::huawei_asr::WebsocketService>();
    CallBack callBack;
    callBack.SetUserInfo("you can set a info to track session");
    websocketServicePtr->SetCallBack(&callBack); // Connect success callback
    // step1 create client
    std::shared_ptr<speech::huawei_asr::RasrClient> rasrClient =
        std::make_shared<speech::huawei_asr::RasrClient>(authInfo, websocketServicePtr, httpConfig);

    // step2 connect, just select one mode, the following is continue stream connect.
    rasrClient->ContinueStreamConnect();
    // short stream connect
    // rasrClient->ShortStreamConnect();
    // sentence stream connect
    // rasrClient->SentenceStreamConnect();

    // step3 construct request params
    speech::huawei_asr::RasrRequest request(FLAGS_audioFormat, FLAGS_property);
    // set whether to add punctuation, yes or no, default no, optional operation.
    request.SetPunc("no");
    // set whether to transcribe number into arabic numerals, yes or no, default yes,optional operation.
    request.SetDigitNorm("yes");
    // set vad head, max silent head, [0, 60000], default 10000, optional operation.
    request.SetVadHead(10000);
    // set vad tail, max silent tail, [0, 3000], default 500, optional operation.
    request.SetVadTail(500);
    // set max seconds of one sentence, [1, 60], default 30, optional operation.
    request.SetMaxSeconds(30);
    // set whether to return intermediate result, yes or no, default no. optional operation.
    request.SetIntermediateResult("no");
    // set whether to return word_info, yes or no, default no. optional operation.
    request.SetNeedWordInfo("no");
    // set vocabulary_id, it should be filled only if it exists or it will report error
    // request.SetVocabularyId("");

    // step4 send start
    rasrClient->SendStart(request);

    // step5 send audio
    std::string audioContent;
    int ret = speech::huawei_asr::ReadBinary(filePath, audioContent);
    if (ret != 0) {
        LOG(ERROR) << "RasrDemo running failed";
        rasrClient->Close();
        return;
    }
    unsigned char *buf = (unsigned char *)(audioContent.c_str());
    rasrClient->SendBinary(buf, audioContent.size(), FLAGS_chunkSize, sleepTime);

    // step5 send end
    rasrClient->SendEnd();

    // step6 close
    rasrClient->Close();
}

int main(int argc, char *argv[])
{
    FLAGS_alsologtostderr = true;
    FLAGS_log_dir = "./logs";
    gflags::ParseCommandLineFlags(&argc, &argv, true);
    google::InitGoogleLogging(argv[0]);
    RasrTest(FLAGS_audioPath);
    return 0;
}
