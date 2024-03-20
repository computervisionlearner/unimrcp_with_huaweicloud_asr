/*
 * Copyright 2008-2015 Arsen Chaloyan
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/*
 * Mandatory rules concerning plugin implementation.
 * 1. Each plugin MUST implement a plugin/engine creator function
 *    with the exact signature and name (the main entry point)
 *        MRCP_PLUGIN_DECLARE(mrcp_engine_t*) mrcp_plugin_create(apr_pool_t *pool)
 * 2. Each plugin MUST declare its version number
 *        MRCP_PLUGIN_VERSION_DECLARE
 * 3. One and only one response MUST be sent back to the received request.
 * 4. Methods (callbacks) of the MRCP engine channel MUST not block.
 *   (asynchronous response can be sent from the context of other thread)
 * 5. Methods (callbacks) of the MPF engine stream MUST not block.
 */

extern "C" {
#include "mrcp_recog_engine.h"
#include "mpf_activity_detector.h"
#include "apt_consumer_task.h"
#include "apt_log.h"
}  // extern C

#include "Utils.h"
#include "RasrListener.h"
#include "RasrClient.h"
#include "gflags/gflags.h"
#define RECOG_ENGINE_TASK_NAME "Huawei Recog Engine"
typedef struct huawei_recog_engine_t huawei_recog_engine_t;
typedef struct huawei_recog_channel_t huawei_recog_channel_t;
typedef struct huawei_recog_msg_t huawei_recog_msg_t;
DEFINE_string(ak, "", "access key");
DEFINE_string(sk, "", "secrect key");
// region, for example cn-east-3, cn-north-4
DEFINE_string(region, "cn-east-3", "project region, such as cn-east-3");
// projectId, refer to https://support.huaweicloud.com/api-sis/sis_03_0008.html
DEFINE_string(projectId, "", "project id");
// endpoint, relevant to region, sis-ext.${region}.myhuaweicloud.com
DEFINE_string(endpoint, "", "service endpoint");
DEFINE_string(audioFormat, "pcm8k16bit", "such pcm16k16bit alaw16k16bit etc.");
DEFINE_string(property, "chinese_8k_general", "");
DEFINE_string(asr_mode, "continue", "continue, short , sentence");
DEFINE_int32(readTimeOut, 20000, "read time out, default 20s");
DEFINE_int32(connectTimeOut, 20000, "connecting time out, default 20s");
DEFINE_int32(vadHead, 10000, "read time out, default 20s");
DEFINE_int32(vadTail, 800, "connecting time out, default 20s");
DEFINE_int32(maxSeconds, 60, "connecting time out, default 20s");
/** Declare this macro to set plugin version */
MRCP_PLUGIN_VERSION_DECLARE

/**
 * Declare this macro to use log routine of the server, plugin is loaded from.
 * Enable/add the corresponding entry in logger.xml to set a cutsom log source priority.
 *    <source name="RECOG-PLUGIN" priority="DEBUG" masking="NONE"/>
 */
MRCP_PLUGIN_LOG_SOURCE_IMPLEMENT(RECOG_PLUGIN, "RECOG-PLUGIN")

/** Use custom log source mark */
#define RECOG_LOG_MARK APT_LOG_MARK_DECLARE(RECOG_PLUGIN)

enum class Status :int {
    initial = 0,
    opened = 1,
    started = 2,
    voice_start = 3,
    recognizing = 4,
    voice_end = 5,
    exceeded_silence = 6,
    ended = 7,
    closed = 8
};

class CallBack : public RasrListener {
public:
    CallBack() {
        result.clear();
        status = Status::initial;
    }
    void OnOpen()
    {   
        status = Status::opened;
        apt_log(RECOG_LOG_MARK, APT_PRIO_INFO, "now rasr Connect success");
    }
    void OnStart(std::string text)
    {
        status = Status::started;
        apt_log(RECOG_LOG_MARK, APT_PRIO_INFO, "now rasr receive start response: %s", text.c_str());
    }
    void OnResp(std::string text)
    {
        apt_log(RECOG_LOG_MARK, APT_PRIO_INFO, "user [%s] rasr receive %s", userMeta_.c_str(), text.c_str());
        result.assign(text);
    }
    void OnEnd(std::string text)
    {
        status = Status::ended;
        apt_log(RECOG_LOG_MARK, APT_PRIO_INFO, "now rasr receive end response: %s", text.c_str());
    }
    void OnClose()
    {
        status = Status::closed;
        apt_log(RECOG_LOG_MARK, APT_PRIO_INFO, "now rasr receive Close");
    }
    void OnError(std::string text)
    {
        apt_log(RECOG_LOG_MARK, APT_PRIO_INFO, "now rasr receive error: %s", text.c_str());
    }
    void OnEvent(std::string text)
    {
        // EXCEED_SILENCE
        if (text.find("VOICE_START") != std::string::npos) {
            status = Status::voice_start;
        } else if (text.find("EXCEEDED_SILENCE") != std::string::npos) {
            status = Status::exceeded_silence;
        } else if (text.find("VOICE_END") != std::string::npos) {
            status = Status::voice_end;
        }
        apt_log(RECOG_LOG_MARK, APT_PRIO_INFO, "now rasr receive event: %s", text.c_str());
    }
    void SetUserInfo(const std::string &str)
    {
        userMeta_ = str;
    }
    const std::string Result() {
        return result;
    }
    Status GetStatus() {
        return status;
    }
    void SetStatus(Status in) {
        status = in;
    }
private:
    std::string userMeta_;
    std::string result;
    Status status;
};

/** Declaration of recognizer engine methods */
static apt_bool_t huawei_recog_engine_destroy(mrcp_engine_t *engine);
static apt_bool_t huawei_recog_engine_open(mrcp_engine_t *engine);
static apt_bool_t huawei_recog_engine_close(mrcp_engine_t *engine);
static mrcp_engine_channel_t *huawei_recog_engine_channel_create(mrcp_engine_t *engine, apr_pool_t *pool);

static const struct mrcp_engine_method_vtable_t engine_vtable = {huawei_recog_engine_destroy,
    huawei_recog_engine_open,
    huawei_recog_engine_close,
    huawei_recog_engine_channel_create};

/** Declaration of recognizer channel methods */
static apt_bool_t huawei_recog_channel_destroy(mrcp_engine_channel_t *channel);
static apt_bool_t huawei_recog_channel_open(mrcp_engine_channel_t *channel);
static apt_bool_t huawei_recog_channel_close(mrcp_engine_channel_t *channel);
static apt_bool_t huawei_recog_channel_request_process(mrcp_engine_channel_t *channel, mrcp_message_t *request);

static const struct mrcp_engine_channel_method_vtable_t channel_vtable = {huawei_recog_channel_destroy,
    huawei_recog_channel_open,
    huawei_recog_channel_close,
    huawei_recog_channel_request_process};

/** Declaration of recognizer audio stream methods */
static apt_bool_t huawei_recog_stream_destroy(mpf_audio_stream_t *stream);
static apt_bool_t huawei_recog_stream_open(mpf_audio_stream_t *stream, mpf_codec_t *codec);
static apt_bool_t huawei_recog_stream_close(mpf_audio_stream_t *stream);
static apt_bool_t huawei_recog_stream_write(mpf_audio_stream_t *stream, const mpf_frame_t *frame);

static const mpf_audio_stream_vtable_t audio_stream_vtable = {huawei_recog_stream_destroy,
    NULL,
    NULL,
    NULL,
    huawei_recog_stream_open,
    huawei_recog_stream_close,
    huawei_recog_stream_write,
    NULL};

/** Declaration of huawei recognizer engine */
struct huawei_recog_engine_t {
    apt_consumer_task_t *task;
};

/** Declaration of huawei recognizer channel */
struct huawei_recog_channel_t {
    /** Back pointer to engine */
    huawei_recog_engine_t *huawei_engine;
    /** Engine channel base */
    mrcp_engine_channel_t *channel;

    /** Active (in-progress) recognition request */
    mrcp_message_t *recog_request;
    /** Pending stop response */
    mrcp_message_t *stop_response;
    /** Indicates whether input timers are started */
    apt_bool_t timers_started;
    /** Voice activity detector */
    mpf_activity_detector_t *detector;
    /** File to write utterance to */
    FILE *audio_out;

    /** huawei rasr client */
    speech::huawei_asr::RasrClient* rasr_client;  // 华为SDK的属性
    CallBack* call_back;
    ~huawei_recog_channel_t() {
        if (rasr_client) {
            delete rasr_client;
        }
        if (call_back) {
            delete call_back;
        }
    }
};

typedef enum {
    HUAWEI_RECOG_MSG_OPEN_CHANNEL,
    HUAWEI_RECOG_MSG_CLOSE_CHANNEL,
    HUAWEI_RECOG_MSG_REQUEST_PROCESS
} huawei_recog_msg_type_e;

/** Declaration of demo recognizer task message */
struct huawei_recog_msg_t {
    huawei_recog_msg_type_e type;
    mrcp_engine_channel_t *channel;
    mrcp_message_t *request;
};

static apt_bool_t huawei_recog_msg_signal(
    huawei_recog_msg_type_e type, mrcp_engine_channel_t *channel, mrcp_message_t *request);
static apt_bool_t huawei_recog_msg_process(apt_task_t *task, apt_task_msg_t *msg);

/** Create huawei recognizer engine */
MRCP_PLUGIN_DECLARE(mrcp_engine_t *) mrcp_plugin_create(apr_pool_t *pool)
{
    huawei_recog_engine_t *huawei_engine = (huawei_recog_engine_t *)apr_palloc(pool, sizeof(huawei_recog_engine_t));
    apt_task_t *task;
    apt_task_vtable_t *vtable;
    apt_task_msg_pool_t *msg_pool;

    msg_pool = apt_task_msg_pool_create_dynamic(sizeof(huawei_recog_msg_t), pool);
    huawei_engine->task = apt_consumer_task_create(huawei_engine, msg_pool, pool);
    if (!huawei_engine->task) {
        return NULL;
    }
    task = apt_consumer_task_base_get(huawei_engine->task);
    apt_task_name_set(task, RECOG_ENGINE_TASK_NAME);
    vtable = apt_task_vtable_get(task);
    if (vtable) {
        vtable->process_msg = huawei_recog_msg_process;
    }

    /* create engine base */
    return mrcp_engine_create(MRCP_RECOGNIZER_RESOURCE, /* MRCP resource identifier */
        huawei_engine,                                  /* object to associate */
        &engine_vtable,                                 /* virtual methods table of engine */
        pool);                                          /* pool to allocate memory from */
}

/** Destroy recognizer engine */
static apt_bool_t huawei_recog_engine_destroy(mrcp_engine_t *engine)
{
    huawei_recog_engine_t *huawei_engine = (huawei_recog_engine_t *)engine->obj;
    if (huawei_engine->task) {
        apt_task_t *task = apt_consumer_task_base_get(huawei_engine->task);
        apt_task_destroy(task);
        huawei_engine->task = NULL;
    }
    return TRUE;
}

/** Open recognizer engine */
static apt_bool_t huawei_recog_engine_open(mrcp_engine_t *engine)
{
    huawei_recog_engine_t *huawei_engine = (huawei_recog_engine_t *)engine->obj;
    if (huawei_engine->task) {
        apt_task_t *task = apt_consumer_task_base_get(huawei_engine->task);
        apt_task_start(task);
    }

    return mrcp_engine_open_respond(engine, TRUE);
}

/** Close recognizer engine */
static apt_bool_t huawei_recog_engine_close(mrcp_engine_t *engine)
{
    huawei_recog_engine_t *huawei_engine = (huawei_recog_engine_t *)engine->obj;
    if (huawei_engine->task) {
        apt_task_t *task = apt_consumer_task_base_get(huawei_engine->task);
        apt_task_terminate(task, TRUE);
    }

    return mrcp_engine_close_respond(engine);
}

static mrcp_engine_channel_t *huawei_recog_engine_channel_create(mrcp_engine_t *engine, apr_pool_t *pool)
{
    mpf_stream_capabilities_t *capabilities;
    mpf_termination_t *termination;

    /* create huawei recog channel */
    huawei_recog_channel_t *recog_channel = (huawei_recog_channel_t *)apr_palloc(pool, sizeof(huawei_recog_channel_t));
    recog_channel->huawei_engine = (huawei_recog_engine_t *)engine->obj;
    recog_channel->recog_request = nullptr;
    recog_channel->stop_response = nullptr;
    recog_channel->detector = mpf_activity_detector_create(pool);
    recog_channel->audio_out = nullptr;
    recog_channel->rasr_client = nullptr;
    recog_channel->call_back = nullptr;
    
    capabilities = mpf_sink_stream_capabilities_create(pool);
    mpf_codec_capabilities_add(&capabilities->codecs, MPF_SAMPLE_RATE_8000 | MPF_SAMPLE_RATE_16000, "LPCM");

    /* create media termination */
    termination = mrcp_engine_audio_termination_create(recog_channel, /* object to associate */
        &audio_stream_vtable,                                         /* virtual methods table of audio stream */
        capabilities,                                                 /* stream capabilities */
        pool);                                                        /* pool to allocate memory from */

    /* create engine channel base */
    recog_channel->channel = mrcp_engine_channel_create(engine, /* engine */
        &channel_vtable,                                        /* virtual methods table of engine channel */
        recog_channel,                                          /* object to associate */
        termination,                                            /* associated media termination */
        pool);                                                  /* pool to allocate memory from */
    const apt_dir_layout_t *dir_layout = engine->dir_layout;
    // 获取asr配置文件 以下两行均可以正常获取
    // const char* asr_conf_path = apt_dir_layout_path_compose(dir_layout, APT_LAYOUT_CONF_DIR, "asr_info.conf", pool);
    const char* asr_conf_path = apt_confdir_filepath_get(dir_layout, "asr_info.conf", pool);
    apt_log(RECOG_LOG_MARK, APT_PRIO_INFO, "set asr info from %s", asr_conf_path);
    google::SetCommandLineOption("flagfile", asr_conf_path);
    return recog_channel->channel;
}

/** Destroy engine channel */
static apt_bool_t huawei_recog_channel_destroy(mrcp_engine_channel_t *channel)
{
    /* nothing to destrtoy */
    return TRUE;
}

/** Open engine channel (asynchronous response MUST be sent)*/
static apt_bool_t huawei_recog_channel_open(mrcp_engine_channel_t *channel)
{
    if (channel->attribs) {
        /* process attributes */
        const apr_array_header_t *header = apr_table_elts(channel->attribs);
        apr_table_entry_t *entry = (apr_table_entry_t *)header->elts;
        int i;
        for (i = 0; i < header->nelts; i++) {
            apt_log(RECOG_LOG_MARK, APT_PRIO_INFO, "Attrib name [%s] value [%s]", entry[i].key, entry[i].val);
        }
    }

    return huawei_recog_msg_signal(HUAWEI_RECOG_MSG_OPEN_CHANNEL, channel, NULL);
}

/** Close engine channel (asynchronous response MUST be sent)*/
static apt_bool_t huawei_recog_channel_close(mrcp_engine_channel_t *channel)
{
    return huawei_recog_msg_signal(HUAWEI_RECOG_MSG_CLOSE_CHANNEL, channel, NULL);
}

/** Process MRCP channel request (asynchronous response MUST be sent)*/
static apt_bool_t huawei_recog_channel_request_process(mrcp_engine_channel_t *channel, mrcp_message_t *request)
{
    return huawei_recog_msg_signal(HUAWEI_RECOG_MSG_REQUEST_PROCESS, channel, request);
}

/** Process RECOGNIZE request */
static apt_bool_t huawei_recog_channel_recognize(
    mrcp_engine_channel_t *channel, mrcp_message_t *request, mrcp_message_t *response)
{
    /* process RECOGNIZE request */
    mrcp_recog_header_t *recog_header;
    huawei_recog_channel_t *recog_channel = (huawei_recog_channel_t *)channel->method_obj;
    const mpf_codec_descriptor_t *descriptor = mrcp_engine_sink_stream_codec_get(channel);

    if (!descriptor) {
        apt_log(RECOG_LOG_MARK,
            APT_PRIO_WARNING,
            "Failed to Get Codec Descriptor " APT_SIDRES_FMT,
            MRCP_MESSAGE_SIDRES(request));
        response->start_line.status_code = MRCP_STATUS_CODE_METHOD_FAILED;
        return FALSE;
    }

    recog_channel->timers_started = TRUE;

    /* get recognizer header */ 
    // 通过请求header设置vad断句参数  unimrcp提供的断句不够精准，推荐用华为云asr自带的断句能力
    recog_header = (mrcp_recog_header_t *)mrcp_resource_header_get(request);
    if (recog_header) {
        if (mrcp_resource_header_property_check(request, RECOGNIZER_HEADER_START_INPUT_TIMERS) == TRUE) {
            recog_channel->timers_started = recog_header->start_input_timers;
        }
        if (mrcp_resource_header_property_check(request, RECOGNIZER_HEADER_NO_INPUT_TIMEOUT) == TRUE) {
            mpf_activity_detector_noinput_timeout_set(recog_channel->detector, recog_header->no_input_timeout);
        }
        if (mrcp_resource_header_property_check(request, RECOGNIZER_HEADER_SPEECH_COMPLETE_TIMEOUT) == TRUE) {
            mpf_activity_detector_silence_timeout_set(recog_channel->detector, recog_header->speech_complete_timeout);
        }
    }

    if (!recog_channel->audio_out) {
        const apt_dir_layout_t *dir_layout = channel->engine->dir_layout;
        char *file_name = apr_psprintf(
            channel->pool, "utter-%dkHz-%s.pcm", descriptor->sampling_rate / 1000, request->channel_id.session_id.buf);
        char *file_path = apt_vardir_filepath_get(dir_layout, file_name, channel->pool);
        if (file_path) {
            apt_log(RECOG_LOG_MARK, APT_PRIO_INFO, "Open Utterance Output File [%s] for Writing", file_path);
            recog_channel->audio_out = fopen(file_path, "wb");
            if (!recog_channel->audio_out) {
                apt_log(RECOG_LOG_MARK,
                    APT_PRIO_WARNING,
                    "Failed to Open Utterance Output File [%s] for Writing",
                    file_path);
            }
        }
    }

    // 初始化ASR引擎
    speech::huawei_asr::AuthInfo authInfo(FLAGS_ak, FLAGS_sk, FLAGS_region, FLAGS_projectId, FLAGS_endpoint);
    // config Connect parameter
    speech::huawei_asr::HttpConfig httpConfig;
    httpConfig.SetReadTimeout(FLAGS_readTimeOut);
    httpConfig.SetConnectTimeout(FLAGS_connectTimeOut);

    // config callback, callback function are optional, if not set, it will use function in RasrListener
    speech::huawei_asr::WebsocketService::ptr websocketServicePtr =
        websocketpp::lib::make_shared<speech::huawei_asr::WebsocketService>();
    CallBack* callBack = new CallBack();
    callBack->SetUserInfo("User Info");
    websocketServicePtr->SetCallBack(callBack); // Connect success callback
    // step1 create client
    speech::huawei_asr::RasrClient* rasrClient = new speech::huawei_asr::RasrClient(authInfo, websocketServicePtr, httpConfig); 

    recog_channel->rasr_client = rasrClient;
    recog_channel->call_back = callBack;

    // 判断指针是否创建成功
    if (rasrClient == NULL) {
        printf("create rasr client failed\n");
        return FALSE;
    }

    // step2 connect, just select one mode, the following is continue stream connect.
    if (FLAGS_asr_mode == "continue") {
        rasrClient->ContinueStreamConnect();
    } else if (FLAGS_asr_mode == "short") {
        rasrClient->ShortStreamConnect();
    } else {
        rasrClient->SentenceStreamConnect();
    }

    // step3 construct request params
    speech::huawei_asr::RasrRequest rasr_request(FLAGS_audioFormat, FLAGS_property);
    // set whether to add punctuation, yes or no, default no, optional operation.
    rasr_request.SetPunc("no");
    // set whether to transcribe number into arabic numerals, yes or no, default yes,optional operation.
    rasr_request.SetDigitNorm("yes");
    // set vad head, max silent head, [0, 60000], default 10000, optional operation.
    rasr_request.SetVadHead(FLAGS_vadHead);
    // set vad tail, max silent tail, [0, 3000], default 500, optional operation.
    rasr_request.SetVadTail(FLAGS_vadTail);
    // set max seconds of one sentence, [1, 60], default 30, optional operation.
    rasr_request.SetMaxSeconds(FLAGS_maxSeconds);
    // set whether to return intermediate result, yes or no, default no. optional operation.
    rasr_request.SetIntermediateResult("no");
    // set whether to return word_info, yes or no, default no. optional operation.
    rasr_request.SetNeedWordInfo("no");
    // set vocabulary_id, it should be filled only if it exists or it will report error
    // request.SetVocabularyId("");

    // step4 send start
    rasrClient->SendStart(rasr_request);

    response->start_line.request_state = MRCP_REQUEST_STATE_INPROGRESS;
    /* send asynchronous response */
    mrcp_engine_channel_message_send(channel, response);
    recog_channel->recog_request = request;
    return TRUE;
}

/** Process STOP request */
static apt_bool_t huawei_recog_channel_stop(
    mrcp_engine_channel_t *channel, mrcp_message_t *request, mrcp_message_t *response)
{
    /* process STOP request */
    huawei_recog_channel_t *recog_channel = (huawei_recog_channel_t *)channel->method_obj;
    /* store STOP request, make sure there is no more activity and only then send the response */
    recog_channel->stop_response = response;
    return TRUE;
}

/** Process START-INPUT-TIMERS request */
static apt_bool_t huawei_recog_channel_timers_start(
    mrcp_engine_channel_t *channel, mrcp_message_t *request, mrcp_message_t *response)
{
    huawei_recog_channel_t *recog_channel = (huawei_recog_channel_t *)channel->method_obj;
    recog_channel->timers_started = TRUE;
    return mrcp_engine_channel_message_send(channel, response);
}

/** Dispatch MRCP request */
static apt_bool_t huawei_recog_channel_request_dispatch(mrcp_engine_channel_t *channel, mrcp_message_t *request)
{
    apt_bool_t processed = FALSE;
    mrcp_message_t *response = mrcp_response_create(request, request->pool);
    switch (request->start_line.method_id) {
        case RECOGNIZER_SET_PARAMS:
            break;
        case RECOGNIZER_GET_PARAMS:
            break;
        case RECOGNIZER_DEFINE_GRAMMAR:
            break;
        case RECOGNIZER_RECOGNIZE:
            processed = huawei_recog_channel_recognize(channel, request, response);
            break;
        case RECOGNIZER_GET_RESULT:
            break;
        case RECOGNIZER_START_INPUT_TIMERS:
            processed = huawei_recog_channel_timers_start(channel, request, response);
            break;
        case RECOGNIZER_STOP:
            processed = huawei_recog_channel_stop(channel, request, response);
            break;
        default:
            break;
    }
    if (processed == FALSE) {
        /* send asynchronous response for not handled request */
        mrcp_engine_channel_message_send(channel, response);
    }
    return TRUE;
}

/** Callback is called from MPF engine context to destroy any additional data associated with audio stream */
static apt_bool_t huawei_recog_stream_destroy(mpf_audio_stream_t *stream)
{
    return TRUE;
}

/** Callback is called from MPF engine context to perform any action before open */
static apt_bool_t huawei_recog_stream_open(mpf_audio_stream_t *stream, mpf_codec_t *codec)
{
    return TRUE;
}

/** Callback is called from MPF engine context to perform any action after close */
static apt_bool_t huawei_recog_stream_close(mpf_audio_stream_t *stream)
{
    return TRUE;
}

/* Raise huawei START-OF-INPUT event */
static apt_bool_t huawei_recog_start_of_input(huawei_recog_channel_t *recog_channel)
{
    /* create START-OF-INPUT event */
    mrcp_message_t *message =
        mrcp_event_create(recog_channel->recog_request, RECOGNIZER_START_OF_INPUT, recog_channel->recog_request->pool);
    if (!message) {
        return FALSE;
    }

    /* set request state */
    message->start_line.request_state = MRCP_REQUEST_STATE_INPROGRESS;
    /* send asynch event */
    return mrcp_engine_channel_message_send(recog_channel->channel, message);
}

/* Load demo recognition result */
static apt_bool_t huawei_recog_result_load(huawei_recog_channel_t *recog_channel, mrcp_message_t *message)
{
    mrcp_engine_channel_t *channel = recog_channel->channel;
    const apt_dir_layout_t *dir_layout = channel->engine->dir_layout;

    /* read the demo result from file  这里是从asr真实结果中读取*/
    mrcp_generic_header_t *generic_header;

    apt_string_assign_n(&message->body, recog_channel->call_back->Result().c_str(), recog_channel->call_back->Result().size(), message->pool);

    /* get/allocate generic header */
    generic_header = mrcp_generic_header_prepare(message);
    if (generic_header) {
        /* set content types */
        apt_string_assign(&generic_header->content_type, "application/x-nlsml", message->pool);
        mrcp_generic_header_property_add(message, GENERIC_HEADER_CONTENT_TYPE);
    }
    return TRUE;
}

/* Raise huawei RECOGNITION-COMPLETE event */
static apt_bool_t huawei_recog_recognition_complete(
    huawei_recog_channel_t *recog_channel, mrcp_recog_completion_cause_e cause)
{
    mrcp_recog_header_t *recog_header;
    /* create RECOGNITION-COMPLETE event */
    mrcp_message_t *message = mrcp_event_create(
        recog_channel->recog_request, RECOGNIZER_RECOGNITION_COMPLETE, recog_channel->recog_request->pool);
    if (!message) {
        return FALSE;
    }

    /* get/allocate recognizer header */
    recog_header = (mrcp_recog_header_t *)mrcp_resource_header_prepare(message);
    if (recog_header) {
        /* set completion cause */
        recog_header->completion_cause = cause;
        mrcp_resource_header_property_add(message, RECOGNIZER_HEADER_COMPLETION_CAUSE);
    }
    /* set request state */
    message->start_line.request_state = MRCP_REQUEST_STATE_COMPLETE;

    if (cause == RECOGNIZER_COMPLETION_CAUSE_SUCCESS) {
        huawei_recog_result_load(recog_channel, message);
    }

    recog_channel->recog_request = NULL;
    /* send asynch event */
    return mrcp_engine_channel_message_send(recog_channel->channel, message);
}

/** Callback is called from MPF engine context to write/send new frame */
static apt_bool_t huawei_recog_stream_write(mpf_audio_stream_t *stream, const mpf_frame_t *frame)
{
    huawei_recog_channel_t *recog_channel = (huawei_recog_channel_t *)stream->obj;
    if (recog_channel->stop_response) {
        /* send asynchronous response to STOP request */
        mrcp_engine_channel_message_send(recog_channel->channel, recog_channel->stop_response);
        recog_channel->stop_response = NULL;
        recog_channel->recog_request = NULL;
        return TRUE;
    }
    bool finish = false;

    // 接收的数据帧转给SDK
    if (recog_channel->recog_request) {
        // 第622行至660行 是unimrcp提供的断句规则，这里禁用中间两个case（因为效果差），直接用华为云的断句规则
        // mpf_detector_event_e det_event = mpf_activity_detector_process(recog_channel->detector, frame);
        // switch (det_event) {
        //     case MPF_DETECTOR_EVENT_ACTIVITY:
        //         apt_log(RECOG_LOG_MARK,
        //             APT_PRIO_INFO,
        //             "Detected Voice Activity " APT_SIDRES_FMT,
        //             MRCP_MESSAGE_SIDRES(recog_channel->recog_request));
        //         huawei_recog_start_of_input(recog_channel);
        //         break;
        //     case MPF_DETECTOR_EVENT_INACTIVITY:
        //         apt_log(RECOG_LOG_MARK,
        //             APT_PRIO_INFO,
        //             "Detected Voice Inactivity " APT_SIDRES_FMT,
        //             MRCP_MESSAGE_SIDRES(recog_channel->recog_request));
        //         if (recog_channel->rasr_client) {
        //             recog_channel->rasr_client->SendEnd();
        //             recog_channel->rasr_client->Close();
        //         }
        //         finish = true;
        //         huawei_recog_recognition_complete(recog_channel, RECOGNIZER_COMPLETION_CAUSE_SUCCESS);
        //         apt_log(RECOG_LOG_MARK, APT_PRIO_INFO, "get RECOGNIZER_COMPLETION_CAUSE_SUCCESS stop recog");
        //         break;
        //     case MPF_DETECTOR_EVENT_NOINPUT:
        //         apt_log(RECOG_LOG_MARK,
        //             APT_PRIO_INFO,
        //             "Detected Noinput " APT_SIDRES_FMT,
        //             MRCP_MESSAGE_SIDRES(recog_channel->recog_request));
        //         if (recog_channel->timers_started == TRUE) {
        //             if (recog_channel->rasr_client) {
        //                 recog_channel->rasr_client->SendEnd();
        //                 recog_channel->rasr_client->Close();
        //             }
        //             huawei_recog_recognition_complete(recog_channel,RECOGNIZER_COMPLETION_CAUSE_NO_INPUT_TIMEOUT);
        //         }
        //         finish = true;
        //         break;
        //     default:
        //         break;
        // }
        Status cur_status = recog_channel->call_back->GetStatus();
        switch (cur_status) {
            case Status::voice_start:
                apt_log(RECOG_LOG_MARK,
                    APT_PRIO_INFO,
                    "Detected Voice Activity " APT_SIDRES_FMT,
                    MRCP_MESSAGE_SIDRES(recog_channel->recog_request));
                huawei_recog_start_of_input(recog_channel);
                recog_channel->call_back->SetStatus(Status::recognizing);
                break;
            case Status::voice_end:
                apt_log(RECOG_LOG_MARK,
                    APT_PRIO_INFO,
                    "Detected Voice Inactivity " APT_SIDRES_FMT,
                    MRCP_MESSAGE_SIDRES(recog_channel->recog_request));
                if (recog_channel->rasr_client) {
                    recog_channel->rasr_client->SendEnd();
                    recog_channel->rasr_client->Close();
                }
                finish = true;
                huawei_recog_recognition_complete(recog_channel, RECOGNIZER_COMPLETION_CAUSE_SUCCESS);
                apt_log(RECOG_LOG_MARK, APT_PRIO_INFO, "get RECOGNIZER_COMPLETION_CAUSE_SUCCESS stop recog");
                break;
            case Status::exceeded_silence:
                apt_log(RECOG_LOG_MARK,
                    APT_PRIO_INFO,
                    "Detected Noinput " APT_SIDRES_FMT,
                    MRCP_MESSAGE_SIDRES(recog_channel->recog_request));
                if (recog_channel->timers_started == TRUE) {
                    if (recog_channel->rasr_client) {
                        recog_channel->rasr_client->SendEnd();
                        recog_channel->rasr_client->Close();
                    }
                    huawei_recog_recognition_complete(recog_channel,RECOGNIZER_COMPLETION_CAUSE_NO_INPUT_TIMEOUT);
                }
                finish = true;
                break;
            default:
                break;
        }        

        if (recog_channel->recog_request) {
            if ((frame->type & MEDIA_FRAME_TYPE_EVENT) == MEDIA_FRAME_TYPE_EVENT) {
                if (frame->marker == MPF_MARKER_START_OF_EVENT) {
                    apt_log(RECOG_LOG_MARK,
                        APT_PRIO_INFO,
                        "Detected Start of Event " APT_SIDRES_FMT " id:%d",
                        MRCP_MESSAGE_SIDRES(recog_channel->recog_request),
                        frame->event_frame.event_id);
                } else if (frame->marker == MPF_MARKER_END_OF_EVENT) {
                    apt_log(RECOG_LOG_MARK,
                        APT_PRIO_INFO,
                        "Detected End of Event " APT_SIDRES_FMT " id:%d duration:%d ts",
                        MRCP_MESSAGE_SIDRES(recog_channel->recog_request),
                        frame->event_frame.event_id,
                        frame->event_frame.duration);
                }
            }
        }

        if (recog_channel->audio_out) {
            fwrite(frame->codec_frame.buffer, 1, frame->codec_frame.size, recog_channel->audio_out);
        }

        if (recog_channel->rasr_client && !finish) {
            recog_channel->rasr_client->SendBinary((unsigned char*)frame->codec_frame.buffer, frame->codec_frame.size);
            apt_log(RECOG_LOG_MARK, APT_PRIO_INFO, "Sending [%d] bytes", frame->codec_frame.size);
        }
    }
    return TRUE;
}

static apt_bool_t huawei_recog_msg_signal(
    huawei_recog_msg_type_e type, mrcp_engine_channel_t *channel, mrcp_message_t *request)
{
    apt_bool_t status = FALSE;
    huawei_recog_channel_t *huawei_channel = (huawei_recog_channel_t *)channel->method_obj;
    huawei_recog_engine_t *huawei_engine = huawei_channel->huawei_engine;
    apt_task_t *task = apt_consumer_task_base_get(huawei_engine->task);
    apt_task_msg_t *msg = apt_task_msg_get(task);
    if (msg) {
        huawei_recog_msg_t *huawei_msg;
        msg->type = TASK_MSG_USER;
        huawei_msg = (huawei_recog_msg_t *)msg->data;

        huawei_msg->type = type;
        huawei_msg->channel = channel;
        huawei_msg->request = request;
        status = apt_task_msg_signal(task, msg);
    }
    return status;
}

static apt_bool_t huawei_recog_msg_process(apt_task_t *task, apt_task_msg_t *msg)
{
    huawei_recog_msg_t *huawei_msg = (huawei_recog_msg_t *)msg->data;
    switch (huawei_msg->type) {
        case HUAWEI_RECOG_MSG_OPEN_CHANNEL:
            /* open channel and send asynch response */
            mrcp_engine_channel_open_respond(huawei_msg->channel, TRUE);
            apt_log(RECOG_LOG_MARK, APT_PRIO_INFO, "HUAWEI_RECOG_MSG_OPEN_CHANNEL");
            break;
        case HUAWEI_RECOG_MSG_CLOSE_CHANNEL: {
            /* close channel, make sure there is no activity and send asynch response */
            huawei_recog_channel_t *recog_channel = (huawei_recog_channel_t *)huawei_msg->channel->method_obj;
            if (recog_channel->audio_out) {
                fclose(recog_channel->audio_out);
                recog_channel->audio_out = NULL;
            }
            apt_log(RECOG_LOG_MARK, APT_PRIO_INFO, "HUAWEI_RECOG_MSG_CLOSE_CHANNEL");
            mrcp_engine_channel_close_respond(huawei_msg->channel);
            break;
        }
        case HUAWEI_RECOG_MSG_REQUEST_PROCESS:
            apt_log(RECOG_LOG_MARK, APT_PRIO_INFO, "HUAWEI_RECOG_MSG_REQUEST_PROCESS");
            huawei_recog_channel_request_dispatch(huawei_msg->channel, huawei_msg->request);
            break;
        default:
            break;
    }
    return TRUE;
}