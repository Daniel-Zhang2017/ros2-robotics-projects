#include "Processor.h"

std::string gSyncSid;
std::string gVoiceCloneResId;
std::string awake_word = "请问有什么指示。 ";
std::string thinking = "稍等，正在思索中。 ";
std::string g_current_awake_sid; // 当前唤醒反馈的SID
std::string RECORD_DEVICE_NAME;
int gPcmPlayerIndex = -1;
bool record_status = false;
bool initialized = false;
bool play_status;
bool g_tts_cancelling = false;
bool g_allow_new_tts = true; // 是否允许新的TTS
int player_mode = 1; //0:aiui内置播放器，1:外置播放器（默认）

//添加唤醒词列表(识别过滤)
const std::vector<std::string> wake_words = {
    "小微小微", 
    "小薇小薇", 
    "小V小V", 
    "小v小v",
    "你好小微",
    "你好小薇",
    "你好小V",
    "你好小v"
};

void waitForTtsComplete();

/*********************播放回调函数************************/
void onStarted()
{
    std::cout << "PcmPlayer, onStarted" << std::endl;
}

void onPaused()
{
    std::cout << "PcmPlayer, onPaused" << std::endl;
}

void onResumed()
{
    std::cout << "PcmPlayer, onResumed" << std::endl;
}

void onStopped()
{
    std::cout << "PcmPlayer, onStopped" << std::endl;
}

void onError(int error, const char* des)
{
    std::cout << "PcmPlayer, onError, error=" << error << ", des=" << des << std::endl;
}

void onProgress(int streamId, int progress, const char* audio, int len, bool isCompleted)
{
//    std::cout << "PcmPlayer, onProgress, streamId=" << streamId << ", progress=" << progress
//         << ", len=" << len << ", isCompleted=" << isCompleted << std::endl;
}

void startTTS(const std::string& text, const std::string& tag = "");
void cancelTTS(const std::string& sid = "");

int init_player()
{   
    player = std::make_unique<PCMPlayer>(16000, SND_PCM_FORMAT_S16_LE, 1);
    if (!initialized) {
        if (player->init_alsa()) {
            initialized = true;
            return 0;
        } else {
            return -1;
        }
    }
    return 0;
}

/**
 * 开始录音接口
 */
void alsaStart()
{
    try {
        if (!audioListeningThread) {
            audioListeningThread = std::make_unique<AudioListenThread>(g_pAgent);
        }
        bool result = audioListeningThread->run();
        if (result) {
            record_status = true;
        } else {
            record_status = audioListeningThread->isRunning();
        }
    } catch (const std::exception& e) {
        std::cerr << "alsaStart error: " << e.what() << std::endl;
        record_status = false;
    }
}

/**
 * 停止录音接口
 */
void alsaStop()
{
    try {
        if (audioListeningThread) {
            audioListeningThread->stopRun();
            record_status = false;
        }
    } catch (const std::exception& e) {
        std::cerr << "alsaStop error: " << e.what() << std::endl;
        record_status = false;
    }
}

/**
 * 检查识别结果是否包含唤醒词
 */
bool containsWakeWord(const std::string& text) {
    std::string lowerText = text;
    // 转换为小写进行比较
    std::transform(lowerText.begin(), lowerText.end(), lowerText.begin(), 
                  [](unsigned char c){ return std::tolower(c); });
    
    for (const auto& word : wake_words) {
        std::string lowerWord = word;
        std::transform(lowerWord.begin(), lowerWord.end(), lowerWord.begin(), 
                      [](unsigned char c){ return std::tolower(c); });
        
        if (lowerText.find(lowerWord) != std::string::npos) {
            return true;
        }
    }
    return false;
}

/**
 * 销毁AIUIAgent对象。
 */
void destroyAgent()
{
    if (g_pAgent) {
        g_pAgent->destroy();
        g_pAgent = nullptr;
    }
}


// 从StreamNlpTtsHelper::Listener派生流式合成监听器，用于监听大模型结果的合成
void DemoListener::TtsHelperListener::onText(const StreamNlpTtsHelper::OutTextSeg& textSeg)
{
    if (textSeg.isBegin() || textSeg.isEnd()) {
        if (aiui_pcm_player_get_state() != PCM_PLAYER_STATE_STARTED) {
            aiui_pcm_player_start();
        }
        if (textSeg.isBegin()) {
            aiui_pcm_player_clear();
        }
    }

    // 调用合成
    startTTS(textSeg.mText, textSeg.mTag);  
}

void DemoListener::TtsHelperListener::onFinish(const std::string& fullText)
{
    // 文本合成完成回调
    std::cout << "tts, fullText=" << fullText << std::endl;

}

void DemoListener::TtsHelperListener::onTtsData(const Json::Value& bizParamJson, const char* audio, int len)
{
    const Json::Value& data = (bizParamJson["data"])[0];
    const Json::Value& content = (data["content"])[0];
    int dts = content["dts"].asInt();
    int progress = content["text_percent"].asInt();

    // 将合成数据写入播放器
    aiui_pcm_player_write(0, audio, len, dts, progress);
}

DemoListener::DemoListener()
{  
    // 创建内置的pcm播放器，并初始化，设置回调，启动起来
    if (!player_mode){
        aiui_pcm_player_create();

        int count = aiui_pcm_player_get_output_device_count();
        for (int i = 0; i < count; i++) {
            std::cout << "pcm player index: " << i
                 << " device name: " << aiui_pcm_player_get_device_name(i) << std::endl;
        }
        std::cout << "user pcm player index: " << gPcmPlayerIndex << std::endl;

        int ret = aiui_pcm_player_init(2);
        if (ret == 0){
            initialized = true;
        }
        aiui_pcm_player_set_callbacks(
            onStarted, onPaused, onResumed, onStopped, onProgress, onError); 
        // aiui_pcm_player_start();   
    }
    else if (player_mode == 1){
        init_player();
    }

    std::shared_ptr<TtsHelperListener> listener = std::make_shared<TtsHelperListener>();
    m_pTtsHelper = std::make_shared<StreamNlpTtsHelper>(listener);
    m_pTtsHelper->setTextMinLimit(20);
}

DemoListener::~DemoListener()
{
    // 析构时销毁播放器，释放资源
    if (!player_mode) aiui_pcm_player_destroy();
}

/**
 * 重写onEvent方法，SDK通过回调该方法抛出各种事件，在这里针对事件做对应的处理。
 *
 * @param event 
 */
void DemoListener::onEvent(const IAIUIEvent& event)
{
    try {
        handleEvent(event);
    } catch (std::exception& e) {
        std::cout << e.what() << std::endl;
    }
}

void DemoListener::processIntentJson(Json::Value& params,
                       Json::Value& intentJson,
                       std::string& resultStr,
                       int eosRsltTime,
                       std::string& sid)
{
    int rc = intentJson["rc"].asInt();

    Json::Value answerJson = intentJson["answer"];
    std::string answer = answerJson["text"].asString();

    // 正常nlp结果（AIUI通用语义模型返回的语义结果）
    bool if_print = false;
    if (if_print){
        std::cout << "----------------------------------" << std::endl;
        std::cout << "params: " << params.asString() << std::endl;
        std::cout << "nlp: " << resultStr << std::endl;
        std::cout << "eos_result=" << eosRsltTime << "ms" << std::endl;
        std::cout << "结果解析：" << std::endl;
        std::cout << "sid=" << sid << std::endl;
        std::cout << "text（请求文本）: " << intentJson["text"].asString() << std::endl;
        std::cout << "rc=" << rc << ", answer（应答语）: " << answer << std::endl;
    }

    if (!answer.empty()) {
        startTTS(answer);
    }
}

void DemoListener::handleEvent(const IAIUIEvent& event)
{
    switch (event.getEventType()) {
        // SDK状态
        case AIUIConstant::EVENT_STATE: {
            switch (event.getArg1()) {
                case AIUIConstant::STATE_IDLE: {
                    // 空闲状态，即最初始的状态
                    std::cout << "EVENT_STATE: STATE_IDLE" << std::endl;
                } break;

                case AIUIConstant::STATE_READY: {
                    // 准备好状态（待唤醒），可以进行唤醒
                    std::cout << "EVENT_STATE: STATE_READY" << std::endl;
                } break;

                case AIUIConstant::STATE_WORKING: {
                    // 工作状态（即已唤醒状态），可以语音交互，也可以再次唤醒
                    std::cout << "EVENT_STATE: STATE_WORKING" << std::endl;
                } break;
            }
        } break;

        // 唤醒事件
        case AIUIConstant::EVENT_WAKEUP: {
            //std::cout << "EVENT_WAKEUP: " << event.getInfo() << std::endl;

            // 唤醒时停止播放
            if (!player_mode) aiui_pcm_player_stop();
        } break;

        // 休眠事件，即一段时间无有效交互或者外部主动要求，SDK会自动进入STATE_READY状态
        case AIUIConstant::EVENT_SLEEP: {
            // arg1用来区分休眠类型，是自动休眠还是外部要求，可参考AIUIConstant.h中EVENT_SLEEP的注释
            std::cout << "EVENT_SLEEP: arg1=" << event.getArg1() << std::endl;
        } break;

        // VAD事件，如语音活动检测
        case AIUIConstant::EVENT_VAD: {
            // arg1为活动类型
            switch (event.getArg1()) {
                case AIUIConstant::VAD_BOS_TIMEOUT: {
                    alsaStop();
                    std::cout << "EVENT_VAD: VAD_BOS_TIMEOUT" << std::endl;
                } break;

                // 检测到前端点，即开始说话
                case AIUIConstant::VAD_BOS: {
                    std::cout << "EVENT_VAD: BOS" << std::endl;
                } break;

                // 检测到后端点，即说话结束
                case AIUIConstant::VAD_EOS: {
                    alsaStop();
                    std::cout << "EVENT_VAD: EOS" << std::endl;
                } break;

                // 音量，arg2为音量级别（0-30）
                case AIUIConstant::VAD_VOL: {
                    //std::cout << "EVENT_VAD: vol=" << event.getArg2() << std::endl;
                } break;
            }
        } break;

        // 结果事件
        case AIUIConstant::EVENT_RESULT: {
            Json::Value bizParamJson;
            Json::Reader reader;

            if (!reader.parse(event.getInfo(), bizParamJson, false)) {
                std::cout << "parse error! info=" << event.getInfo() << std::endl;
                break;
            }

            Json::Value& data = (bizParamJson["data"])[0];
            Json::Value& params = data["params"];
            Json::Value& content = (data["content"])[0];

            std::string sub = params["sub"].asString();
            if (sub != "iat" && sub != "esr_pgs" && sub != "esr_iat" && sub != "nlp" && sub != "tts" 
                && sub != "cbm_intent_split" && sub != "cbm_semantic") {
                return;
            }
            Json::Value emptyJson;
            // sid即唯一标识一次会话的id
            std::string sid = event.getData()->getString("sid", "");
            if (sub == "iat") {
                if (sid != mCurIatSid) {
                    std::cout << "**********************************" << std::endl;
                    std::cout << "sid=" << sid << std::endl;

                    mCurIatSid = sid;

                    // 新的会话，清空之前识别缓存
                    mIatTextBuffer.clear();
                    mStreamNlpAnswerBuffer.clear();
                    m_pTtsHelper->clear();
                    mIntentCnt = 0;
                }
            } else if (sub == "tts") {
                if (sid != mCurTtsSid) {
                    std::cout << "**********************************" << std::endl;
                    std::cout << "sid=" << sid << std::endl;

                    mCurTtsSid = sid;
                    g_current_tts_sid = sid;
                }
            }

            Json::Value empty;
            std::string cnt_id = content.get("cnt_id", empty).asString();

            int dataLen = 0;

            // 注意：当buffer里存字符串时也不是以0结尾，当使用C语言时，转成字符串则需要自已在末尾加0
            const char* buffer = event.getData()->getBinary(cnt_id.c_str(), &dataLen);

            if (sub == "tts") {
                // 语音合成结果，返回url或者pcm音频
                //std::cout << "tts: " << content.toString() << std::endl;

                Json::Value&& isUrl = content.get("url", empty);
                if (isUrl.asString() == "1") {
                    // 云端返回的是url链接，可以用播放器播放
                    std::cout << "tts_url=" << string(buffer, dataLen) << std::endl;
                } else {
                    // 云端返回的是pcm音频，分成一块块流式返回
                    int progress = 0;
                    int dts = content["dts"].asInt();
                    bool isFirstBlock = (dts == AIUIConstant::DTS_BLOCK_FIRST || dts == AIUIConstant::DTS_ONE_BLOCK);
                    bool isLastBlock = (dts == AIUIConstant::DTS_BLOCK_LAST || dts == AIUIConstant::DTS_ONE_BLOCK);
                    // 处理取消逻辑
                    if (g_tts_cancelling) {
                        // 检查是否是取消的会话
                        if (sid == mCurTtsSid || mCurTtsSid.empty()) {
                            if (isLastBlock) {
                                //std::cout << "[TTS] 收到结束包，取消完成" << std::endl;
                                g_tts_cancelling = false;
                                mCurTtsSid.clear();
                                g_current_tts_sid.clear();
                                g_tts_is_playing = false;
                                g_allow_new_tts = true;
                            }
                            return;
                        }
                    }
                    if (isFirstBlock) {
                        g_tts_is_playing = true;
                        g_allow_new_tts = false;
                        play_status = true;
                    }
                    
                    if (isLastBlock) {
                        g_tts_is_playing = false;
                        g_allow_new_tts = true;
                        play_status = false;
                        mCurTtsSid.clear();
                        g_current_tts_sid.clear();
                    }
                    std::string tag = event.getData()->getString("tag", "");
                    if (tag.find("stream_nlp_tts") == 0) {
                        // 流式语义应答的合成
                        m_pTtsHelper->onOriginTtsData(tag, bizParamJson, buffer, dataLen);
                    } else {
                        if (dts == AIUIConstant::DTS_BLOCK_FIRST || dts == AIUIConstant::DTS_ONE_BLOCK) {
                            // 只有碰到开始块，才开启播放器
                            if (aiui_pcm_player_get_state() != PCM_PLAYER_STATE_STARTED && player_mode == 0) {
                                int ret = aiui_pcm_player_start();
                                std::cout << "pcm start ret:" << ret << std::endl;
                            }
                            play_status = true;
                        }
                        if (!player_mode) aiui_pcm_player_write(0, buffer, dataLen, dts, progress);

                        /////////////////////////////////////////////////////////////////
                        // 播放音频（需要确保audio_data是PCM原始数据）
                        if (buffer && initialized) {
                            if (player_mode) player->play_pcm(buffer, dataLen, dts);
                        }
                        /////////////////////////////////////////////////////////////////

                        if (dts == AIUIConstant::DTS_BLOCK_LAST || dts == AIUIConstant::DTS_ONE_BLOCK) {
                            play_status = false;
                        }

                        // 若要保存合成音频，请打开以下开关
#if 0
                        // 音频开始
                        if (dts == AIUIConstant::DTS_BLOCK_FIRST || dts == AIUIConstant::DTS_ONE_BLOCK) {
                            mFs.open("tts.pcm", ios::binary | ios::out);
                        }

                        if (mFs.is_open()) {
                            if (buffer){
                                mFs.write(buffer, dataLen);
                            }
                        } else {
                            cerr << "文件未打开，写入失败！" << std::endl;
                        }

                        // 音频结束
                        if (dts == AIUIConstant::DTS_BLOCK_LAST || dts == AIUIConstant::DTS_ONE_BLOCK) {
                            //mFs.close();
                            if (mFs.is_open())
                            {
                                mFs.close();
                            }
                        }
#endif
                    }
                }
            } else if (sub == "iat") {
                // 语音识别结果
                std::string resultStr = std::string(buffer, dataLen);     // 注意：这里不能用string resultStr = buffer，因为buffer不一定以0结尾
                Json::Value resultJson;
                if (reader.parse(resultStr, resultJson, false)) {
                    Json::Value textJson = resultJson["text"];
                    bool isWpgs = false;
                    if (textJson.isMember("pgs")) {
                        isWpgs = true;
                    }

                    if (isWpgs) {
                        mIatTextBuffer = IatResultUtil::parsePgsIatText(textJson);
                    } else {
                        // 结果拼接起来
                        mIatTextBuffer.append(IatResultUtil::parseIatResult(textJson));
                    }

                    // 是否是该次会话最后一个识别结果
                    bool isLast = textJson["ls"].asBool();
                    if (isLast) {
                        //std::cout << "params: " << params.asString() << std::endl;
                        std::cout << "识别结果(online): " << mIatTextBuffer << std::endl;
                        // 检查是否包含唤醒词
                        if (containsWakeWord(mIatTextBuffer)) {
                            mIatTextBuffer.clear();
                            std::cout << "检测到唤醒词，重新进入唤醒状态!" << std::endl;
                            
                            if (record_status) alsaStop();
                            alsaStart();
                        }
                        std_msgs::msg::String result_text;
                        result_text.data = mIatTextBuffer;
                        node->voice_words_pub->publish(result_text);


                        mIatTextBuffer.clear();
                    }
                }
            } 
            else if (sub == "esr_pgs") {
                // 语音识别结果(流式)
                std::string resultStr = std::string(buffer, dataLen);     // 注意：这里不能用string resultStr = buffer，因为buffer不一定以0结尾
                Json::Value resultJson;
                if (reader.parse(resultStr, resultJson, false)) {
                    Json::Value textJson = resultJson["text"];

                    std::cout << "esr_pgs: " << resultJson["text"] << std::endl;
                }
            } 
            else if (sub == "esr_iat") {
                // 语音识别结果
                std::string resultStr = std::string(buffer, dataLen);     // 注意：这里不能用string resultStr = buffer，因为buffer不一定以0结尾
                Json::Value resultJson;
                if (reader.parse(resultStr, resultJson, false)) {
                    Json::Value textJson = resultJson["text"];

                    mIatTextBuffer.append(IatResultUtil::parseEsrIatResult(textJson));
                    // 去除中文句号"。"
                    size_t pos;
                    while ((pos = mIatTextBuffer.find("。")) != std::string::npos) {
                        mIatTextBuffer.erase(pos, 3);
                    }
                    std::cout << "识别结果(offline): " << mIatTextBuffer << std::endl;
                    // 检查是否包含唤醒词
                    if (containsWakeWord(mIatTextBuffer)) {
                        mIatTextBuffer.clear();
                        std::cout << "检测到唤醒词，重新进入唤醒状态!" << std::endl;
                        
                        if (record_status) alsaStop();
                        alsaStart();
                    }
                    std_msgs::msg::String result_text;
                    result_text.data = mIatTextBuffer;
                    node->voice_words_pub->publish(result_text);

                    mIatTextBuffer.clear();
                }
            }
            else if (sub == "nlp") {
                // 语义理解结果
                std::string resultStr = std::string(buffer, dataLen);     // 注意：这里不能用string resultStr = buffer，因为buffer不一定以0结尾

                // 从说完话到语义结果返回的时长
                long eosRsltTime = event.getData()->getLong("eos_rslt", -1);

                Json::Value resultJson;
                if (reader.parse(resultStr, resultJson, false)) {
                    // 判断是否为有效结果
                    if (resultJson.isMember("intent") &&
                        resultJson["intent"].isMember("rc")) {
                        // AIUI v1的语义结果
                        Json::Value intentJson = resultJson["intent"];
                        processIntentJson(params, intentJson, resultStr, eosRsltTime, sid);
                    } else if (resultJson.isMember("nlp")) {
                        // AIUI v2的语义结果
                        Json::Value nlpJson = resultJson["nlp"];
                        std::string text = nlpJson["text"].asString();

                        if (text.find("{\"intent\":") == 0) {
                            // 通用语义结果
                            Json::Value textJson;
                            if (reader.parse(text, textJson, false)) {
                                Json::Value intentJson = textJson["intent"];
                                processIntentJson(params, intentJson, resultStr, eosRsltTime, sid);
                            }
                        } else {
                            // 大模型语义结果
                            // 流式nlp结果里面有seq和status字段
                            int seq = nlpJson["seq"].asInt();
                            int status = nlpJson["status"].asInt();

                            /* 多意图取最后一次问题的结果进行tts合成 */
                            if (mIntentCnt > 0) {
                                int currentIntentIndex = 0;
                                Json::Value metaNlpJson;
                                Json::Value textJson = resultJson["cbm_meta"].get("text", metaNlpJson);
                                if (reader.parse(textJson.asString(), metaNlpJson, false)) {
                                    currentIntentIndex = metaNlpJson["nlp"]["intent"].asInt();
                                    if ((mIntentCnt - 1) != currentIntentIndex) {
                                        std::cout << "ignore nlp:" << resultStr << std::endl;
                                        return;
                                    }
                                } else {
                                    std::cout << "ignore nlp:" << resultStr << std::endl;
                                    return;
                                }
                            }

#ifndef USE_POST_SEMANTIC_TTS
                            //如果使用应用的语义后合成不需要在调用下面的函数否则tts的播报会重复
                            m_pTtsHelper->addText(text, seq, status);
#endif

                            std::cout << "----------------------------------" << std::endl;
                            std::cout << "params: " << params.asString() << std::endl;
                            std::cout << "nlp: " << resultStr << std::endl;

                            if (seq == 0) {
                                long eosRsltTime = event.getData()->getLong("eos_rslt", -1);
                                std::cout << "eos_result=" << eosRsltTime << "ms" << std::endl;
                            }

                            std::cout << "结果解析：" << std::endl;
                            std::cout << "sid=" << sid << std::endl;
                            std::cout << "seq=" << seq << ", status=" << status << ", answer（应答语）: " << text << std::endl;
                            std::cout << "fullAnswer=" << (mStreamNlpAnswerBuffer.append(text)) << std::endl;

                            if (status == 2) {
                                mStreamNlpAnswerBuffer.clear();
                            }
                        }
                    } else {
                        // 无效结果，把原始结果打印出来
                        std::cout << "----------------------------------" << std::endl;
                        std::cout << "nlp: " << resultStr << std::endl;
                        std::cout << "sid=" << sid << std::endl;
                    }
                }
            } else if (sub == "cbm_intent_split") {
                // 意图拆分的结果
                std::string intentStr = std::string(buffer, dataLen); // 注意：这里不能用string resultStr = buffer，因为buffer不一定以0结尾
                Json::Value tmpJson;
                if (reader.parse(intentStr, tmpJson, false)) {
                    Json::Value intentTextJson = tmpJson["cbm_intent_split"]["text"];
                    if (!intentTextJson.empty() &&
                        reader.parse(intentTextJson.asString(), tmpJson, false)) {
                        mIntentCnt = tmpJson["intent"].size();
                        std::cout << "cbm_intent_cnt: " << mIntentCnt
                             << " text: " << tmpJson.toString() << std::endl;
                    }
                }
            } else {
                // 其他结果
                std::string resultStr = std::string(buffer, dataLen);     // 注意：这里不能用string resultStr = buffer，因为buffer不一定以0结尾

                std::cout << sub << ": " << event.getInfo() << std::endl << resultStr << std::endl;
            }
        } break;

        // 与CMD命令对应的返回结果，arg1为CMD类型，arg2为错误码
        case AIUIConstant::EVENT_CMD_RETURN: {
            if (AIUIConstant::CMD_BUILD_GRAMMAR == event.getArg1()) {
                // 语法构建命令的结果
                // 注：需要集成本地esr引擎才能构建语法
                if (event.getArg2() == 0) {
                    std::cout << "build grammar success." << std::endl;
                } else {
                    std::cout << "build grammar, error=" << event.getArg2() << ", des=" << event.getInfo() << std::endl;
                }
            } else if (AIUIConstant::CMD_UPDATE_LOCAL_LEXICON == event.getArg1()) {
                // 更新本地语法槽的结果
                if (event.getArg2() == 0) {
                    std::cout << "update lexicon success" << std::endl;
                } else {
                    std::cout << "update lexicon, error=" << event.getArg2() << "des=" << event.getInfo() << std::endl;
                }
            } else if (AIUIConstant::CMD_CLONE_VOICE == event.getArg1()) {
                //声音复刻
                int dtype = event.getData()->getInt("sync_dtype", -1);
                int retCode = event.getArg2();
                std::string dataTypeStr;
                if (dtype == AIUIConstant::VOICE_CLONE_REG) { //注册资源
                    dataTypeStr = "注册音频资源";
                } else if (dtype == AIUIConstant::VOICE_CLONE_DEL) { //删除资源
                    dataTypeStr = "删除资源";
                }

                if (AIUIConstant::SUCCESS == retCode ) {
                    // 上传成功，会话的唯一id，用于反馈问题的日志索引字段，注意留存
                    // 注：上传成功立即生效
                    std::string sid = event.getData()->getString("sid", "");
                    // 获取上传调用时设置的自定义tag
                    std::string tag = event.getData()->getString("tag", "");
                    // 获取上传调用耗时，单位：ms
                    long timeSpent = event.getData()->getLong("time_spent", -1);
                    std::cout << "声音复刻" << dataTypeStr << "成功"
                         << "，耗时：" << timeSpent
                         << "ms, sid=" + sid + "，tag=" + tag;
                    if (dtype == AIUIConstant::VOICE_CLONE_REG) {
                        std::string resId = event.getData()->getString("res_id", "");
                        std::cout << "，res id = " << resId << std::endl;

                        //保存声音复刻的的res id
                        gVoiceCloneResId = resId;
                        fstream fs;
                        fs.open("./voice_clone_reg_id.txt", ios::binary | ios::out);
                        fs.write(resId.c_str(), resId.length());
                        fs.close();
                    } else {
                        std::cout << std::endl;
                    }
                } else {
                    std::string result = event.getData()->getString("result", "");
                    std::cout << "声音复刻" << dataTypeStr << "失败，错误码：" <<
                        retCode << " info:" << event.getInfo() << " result:" << result << std::endl;
                }
            } else if (AIUIConstant::CMD_SYNC == event.getArg1()) {
                // 数据同步的返回
                int dtype = event.getData()->getInt("sync_dtype", -1);
                int retCode = event.getArg2();

            #ifdef AIUI_V2
                std::string dataTypeStr;
                std::string text;

                if (dtype == AIUIConstant::SYNC_DATA_UPLOAD) {
                    dataTypeStr = "上传实体";
                } else if (dtype == AIUIConstant::SYNC_DATA_DELETE) {
                    dataTypeStr = "删除实体";
                } else if (dtype == AIUIConstant::SYNC_DATA_DOWNLOAD) {
                    dataTypeStr = "下载实体";
                } else if (dtype == AIUIConstant::SYNC_DATA_SEE_SAY) {
                    dataTypeStr = "所见即可说";
                }

                if (AIUIConstant::SUCCESS == retCode ) {
                    // 上传成功，会话的唯一id，用于反馈问题的日志索引字段，注意留存
                    // 注：上传成功立即生效
                    gSyncSid = event.getData()->getString("sid", "");
                    // 获取上传调用时设置的自定义tag
                    std::string tag = event.getData()->getString("tag", "");
                    // 获取上传调用耗时，单位：ms
                    long timeSpent = event.getData()->getLong("time_spent", -1);
                    std::cout << "同步" << dataTypeStr << "成功"
                         << "，耗时：" << timeSpent
                         << "ms, sid=" + gSyncSid + "，tag=" + tag;
                    if (dtype == AIUIConstant::SYNC_DATA_UPLOAD) {
                        std::cout << "，你可以试着说“打电话给刘德华“" << std::endl;
                    } else {
                        std::cout << std::endl;
                    }
                    // 实体内容
                    if (dtype == AIUIConstant::SYNC_DATA_DOWNLOAD) {
                        text = event.getData()->getString("text", "");
                        std::cout << "下载的实体内容:\n" << Base64Util::decode(text) << std::endl;
                    }
                } else {
                    gSyncSid = "";
                    std::string result = event.getData()->getString("result", "");
                    std::cout << "同步" << dataTypeStr << "失败，错误码：" <<
                        retCode << " info:" << event.getInfo() << " result:" << result << std::endl;
                }
            #else
                if (dtype == AIUIConstant::SYNC_DATA_SCHEMA) {
                    if (AIUIConstant::SUCCESS == retCode) {
                        // 上传成功，记录上传会话的sid，以用于查询数据打包状态
                        // 注：上传成功并不表示数据打包成功，打包成功与否应以同步状态查询结果为准，数据只有打包成功后才能正常使用
                        gSyncSid = event.getData()->getString("sid", "");

                        // 获取上传调用时设置的自定义tag
                        std::string tag = event.getData()->getString("tag", "");

                        // 获取上传调用耗时，单位：ms
                        long timeSpent = event.getData()->getLong("time_spent", -1);

                        std::cout << "同步成功，"
                             << "耗时：" << timeSpent
                             << "ms, sid=" + gSyncSid + "，tag=" + tag +
                                    "，你可以试着说“打电话给刘德华“"
                             << std::endl;
                    } else {
                        gSyncSid = "";
                        std::cout << "同步失败，错误码：" << retCode << std::endl;
                    }
                }
            #endif
            }
        #ifndef AIUI_V2
            else if (AIUIConstant::CMD_QUERY_SYNC_STATUS == event.getArg1()) {
                // 数据同步状态查询的返回
                // 获取同步类型
                int syncType = event.getData()->getInt("sync_dtype", -1);
                if (AIUIConstant::SYNC_DATA_QUERY == syncType) {
                    // 若是同步数据查询，则获取查询结果，结果中error字段为0则表示上传数据打包成功，否则为错误码
                    std::string result = event.getData()->getString("result", "");

                    std::cout << "查询结果：" << result << std::endl;
                }
            }
        #endif
        } break;

        // 开始录音事件
        case AIUIConstant::EVENT_START_RECORD: {
            std::cout << "EVENT_START_RECORD " << std::endl;
        } break;

        // 停止录音事件
        case AIUIConstant::EVENT_STOP_RECORD: {
            std::cout << "EVENT_STOP_RECORD " << std::endl;
        } break;

        // 出错事件
        case AIUIConstant::EVENT_ERROR: {
            // 打印错误码和描述信息(默认关闭大模型回复)
            std::string errorInfo = event.getInfo();
            if (errorInfo.find("nlp") != std::string::npos || 
                errorInfo.find("NLP") != std::string::npos) {
                // NLP 相关错误，忽略
            } else {
                std::cout << "EVENT_ERROR: error=" << event.getArg1() << ", des=" << errorInfo << std::endl;
            }
            //std::cout << "EVENT_ERROR: error=" << event.getArg1() << ", des=" << event.getInfo() << std::endl;
        } break;

        // 连接到服务器
        case AIUIConstant::EVENT_CONNECTED_TO_SERVER: {
            // 获取uid（为客户端在云端的唯一标识）并打印
            std::string uid = event.getData()->getString("uid", "");

            std::cout << "EVENT_CONNECTED_TO_SERVER, uid=" << uid << std::endl;
        } break;

        // 与服务器断开连接
        case AIUIConstant::EVENT_SERVER_DISCONNECTED: {
            std::cout << "EVENT_SERVER_DISCONNECTED " << std::endl;
        } break;
    }
}


#define TEST_ROOT_DIR       "/AIUI/"
#define TEST_ROOT_DIR_PATH  CURRENT_PATH TEST_ROOT_DIR
#
#ifdef TURING_UNIT_SUPPORT
    #define CFG_FILE    "/AIUI/cfg/turing.cfg"
    #define CFG_FILE_PATH CURRENT_PATH CFG_FILE
#else
    #define CFG_FILE    "/AIUI/cfg/aiui.cfg"
    #define CFG_FILE_PATH CURRENT_PATH CFG_FILE
#endif
#
#define TEST_AUDIO      "/AIUI/audio/test.pcm"
#define TEST_AUDIO_PATH CURRENT_PATH TEST_AUDIO
#define LOG_DIR         "/AIUI/msc/aiui.log"
#define LOG_DIR_PATH    CURRENT_PATH LOG_DIR
#define MSC_DIR         "/AIUI/msc/"
#define MSC_DIR_PATH    CURRENT_PATH MSC_DIR
#define TEST_TTS        "/AIUI/text/tts.txt"
#define TEST_TTS_PATH   CURRENT_PATH TEST_TTS
#define TEST_SEE_SAY    "/AIUI/text/see_say.txt"
#define TEST_SEE_SAY_PATH   CURRENT_PATH TEST_SEE_SAY


/**
 * 读取文件内容存到字符串。
 *
 * @param path
 * @return
 */
std::string readFileAsString(const std::string& path)
{
    ifstream t(path, ios_base::in | ios::binary);
    if (!t.is_open()) {
        std::cout << "Error open file: " << path << " fail.";
    }
    std::string str((istreambuf_iterator<char>(t)), istreambuf_iterator<char>());

    return str;
}

/**
 * 创建AIUIAgent对象。
 *
 * @param more
 * @param cfgPath
 */
void createAgent(bool more = true, const char* cfgPath = CFG_FILE_PATH)
{
    if (g_pAgent) {
        return;
    }

    std::string aiuiParams = readFileAsString(cfgPath);

    Json::Value paramJson;
    Json::Reader reader;
    if (reader.parse(aiuiParams, paramJson, false)) {
        if (more) {
            std::cout << paramJson.toString() << std::endl;
        }

        g_pListener->mMoreDetails = more;
        g_pAgent = IAIUIAgent::createAgent(paramJson.toString().c_str(), g_pListener);
    }

    if (!g_pAgent) {
        std::cout << std::string(cfgPath) << ", " << reader.getFormatedErrorMessages() << std::endl;
        return;
    }
}

/**
 * 唤醒AIUI。
 */
void wakeup()
{
    // 可以通过clear_data来控制是否要清除唤醒之前的数据（默认会清除），清除则唤醒之前的会话结果（tts除外）会被丢弃从而不再继续抛出
    SEND_AIUIMESSAGE4(AIUIConstant::CMD_WAKEUP, 0, 0, "clear_data=true");
}

/**
 * 重置唤醒，即回到待唤醒状态。
 */
void resetWakeup()
{
    SEND_AIUIMESSAGE1(AIUIConstant::CMD_RESET_WAKEUP);
}

/**
 * 开启AIUI服务，此接口是与stop()对应，调用stop()之后必须调用此接口才能继续与SDK交互。
 *
 * 注：AIUIAgent创建成功之后AIUI会自动开启，故若非调用过stop()则不需要调用start()。
 */
void start()
{
    SEND_AIUIMESSAGE1(AIUIConstant::CMD_START);
}

/**
 * 停止AIUI服务。
 */
void stop()
{
    SEND_AIUIMESSAGE1(AIUIConstant::CMD_STOP);
}

/**
 * 重置AIUI服务，相当于先调用stop()再调用start()。一般用不到。
 */
void resetAIUI()
{
    SEND_AIUIMESSAGE(AIUIConstant::CMD_RESET, 0, 0, "", nullptr);
}

/**
 * 从文件读音频写入SDK，即用文件数据模型实时录音数据。
 *
 * @param repeat
 */
void writeAudioFromLocal(bool repeat)
{
    if (!g_pAgent) {
        return;
    }

    ifstream testData(TEST_AUDIO_PATH, std::ios::in | std::ios::binary);

    if (testData.is_open()) {
        testData.seekg(0, std::ios::end);
        int total = testData.tellg();
        testData.seekg(0, std::ios::beg);

        char* audio = new char[total];
        testData.read(audio, total);
        testData.close();

        int offset = 0;
        int left = total;
        const int frameLen = 1280;
        char buff[frameLen];

        while (true) {
            if (left < frameLen) {
                if (repeat) {
                    offset = 0;
                    left = total;
                    continue;
                } else {
                    break;
                }
            }

            memset(buff, '\0', frameLen);
            memcpy(buff, audio + offset, frameLen);

            offset += frameLen;
            left -= frameLen;

            // frameData内存会在Message在内部处理完后自动release掉
            AIUIBuffer frameData = aiui_create_buffer_from_data(buff, frameLen);
            SEND_AIUIMESSAGE(AIUIConstant::CMD_WRITE, 0, 0, "data_type=audio,tag=audio-tag", frameData);

            // 必须暂停一会儿模拟人停顿，太快的话后端报错。1280字节16k采样16bit编码的pcm数据对应40ms时长
            AIUI_SLEEP(40);
        }

        // 音频写完后，要发CMD_STOP_WRITE停止写入消息
        SEND_AIUIMESSAGE4(AIUIConstant::CMD_STOP_WRITE, 0, 0, "data_type=audio");

        delete[] audio;
    } else {
        std::cout << "open file failed, path=" << TEST_AUDIO_PATH << std::endl;
    }

    std::cout << "write finish" << std::endl;
}

/**
 * 开启录音。
 */
void startRecordAudio()
{
    SEND_AIUIMESSAGE4(
        AIUIConstant::CMD_START_RECORD, 0, 0, "data_type=audio,pers_param={\"uid\":\"\"},tag=record-tag");
}

/**
 * 停止录音。
 */
void stopRecordAudio()
{
    SEND_AIUIMESSAGE1(AIUIConstant::CMD_STOP_RECORD);
}

/**
 * 写入文本进行交互。
 *
 * @param text 文本内容
 * @param needWakeup 是否需要唤醒
 */
void writeText(const std::string& text, bool needWakeup = true)
{
    AIUIBuffer textData = aiui_create_buffer_from_data(text.c_str(), text.length());

    if (needWakeup) {
        SEND_AIUIMESSAGE(AIUIConstant::CMD_WRITE, 0, 0, "data_type=text,pers_param={\"uid\":\"\"}", textData);
    } else {
        SEND_AIUIMESSAGE(AIUIConstant::CMD_WRITE, 0, 0, "data_type=text,need_wakeup=false", textData);
    }
}

/**
 * 取消语音合成
 */
void cancelTTS(const std::string& sid)
{ 
    if (!g_tts_is_playing && g_current_tts_sid.empty()) {
        return;
    }
    // 设置取消标志
    g_tts_cancelling = true;
    // 发送取消命令
    std::string params = "vcn=x4_lingxiaoying_em_v2";
    if (!sid.empty()) {
        params.append(",sid=").append(sid);
    }
    SEND_AIUIMESSAGE(AIUIConstant::CMD_TTS, AIUIConstant::CANCEL, 0, params.c_str(), nullptr);
}

/**
 * 等待TTS播放完成
 */
bool waitForTtsComplete(int timeout_ms = 5000) {
    auto start_time = std::chrono::steady_clock::now();
    while (g_tts_is_playing) {
        // 检查超时
        auto now = std::chrono::steady_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - start_time);
        if (elapsed.count() > timeout_ms) {
            cancelTTS();
            // 强制重置状态
            g_tts_is_playing = false;
            g_tts_cancelling = false;
            g_allow_new_tts = true;
            std::cout << "等待超时 " << timeout_ms << "ms)" << std::endl;
            return false;
        }
        // 检查TTS是否被取消
        if (!g_tts_is_playing || g_tts_cancelling) {
            std::cout << "TTS已被取消" << std::endl;
            return true;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }
    return true;
}

/**
 * 测试语音合成，返回pcm数据。
 *
 * @param text
 */
void startTTS(const std::string& text, const std::string& tag)
{
    if (initialized && player_mode) {
        player->prepare();
    }else {
        return;
    }
    // 检查是否允许新TTS
    if (!g_allow_new_tts) {
        std::cout << "[TTS] 当前不允许新的TTS" << std::endl;
        return;
    }
    
    // 如果已有TTS在播放，先取消
    if (g_tts_is_playing) {
        cancelTTS();
    }
    
    // 重置状态
    g_tts_is_playing = true;
    g_tts_cancelling = false;

    AIUIBuffer textData = aiui_create_buffer_from_data(text.c_str(), text.length());
    std::string params = "voice_name=x4_lingxiaoying_em_v2";
    if (!tag.empty()) {
        params.append(",tag=").append(tag);
    }
    SEND_AIUIMESSAGE(AIUIConstant::CMD_TTS, AIUIConstant::START, 0, params.c_str(), textData);
}

/**
 * 播放唤醒反馈并等待完成
 */
void playAwakeFeedbackAndWait() {
    // 播放唤醒反馈
    startTTS(awake_word, "wake_feedback");
    waitForTtsComplete(3000);
}

/**
 * 构建asr语法。
 *
 * 注：当前版本已废弃，只有历史版本支持。
 */
void buildAsrGrammar()
{
    std::string grammar = readFileAsString("AIUI/asr/call.bnf");

    SEND_AIUIMESSAGE4(AIUIConstant::CMD_BUILD_GRAMMAR, 0, 0, grammar.c_str());
}

/**
 * 构建esr语法。
 *
 * 注：新版本SDK都只支持esr。
 */
void buildEsrGrammar()
{
    std::string grammar = readFileAsString("AIUI/esr/message.fsa");

    SEND_AIUIMESSAGE4(AIUIConstant::CMD_BUILD_GRAMMAR, 0, 0, grammar.c_str());
}


#if defined(__linux) || defined(__ANDROID__)
    #include <sys/socket.h>
    #include <net/if.h>
    #include <sys/ioctl.h>
/**
 * 获取mac地址。
 *
 * @param mac
 */
static void GenerateMACAddress(char* mac)
{
    // reference: https://stackoverflow.com/questions/1779715/how-to-get-mac-address-of-your-machine-using-a-c-program/35242525
    int sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock < 0) {
        return;
    };

    struct ifconf ifc{};
    char buf[1024];
    int success = 0;

    ifc.ifc_len = sizeof(buf);
    ifc.ifc_buf = buf;
    if (ioctl(sock, SIOCGIFCONF, &ifc) == -1) {
        return;
    }

    struct ifreq* it = ifc.ifc_req;
    const struct ifreq* const end = it + (ifc.ifc_len / sizeof(struct ifreq));
    struct ifreq ifr{};

    for (; it != end; ++it) {
        strcpy(ifr.ifr_name, it->ifr_name);
        if (ioctl(sock, SIOCGIFFLAGS, &ifr) == 0) {
            if (!(ifr.ifr_flags & IFF_LOOPBACK)) {    // don't count loopback
                if (ioctl(sock, SIOCGIFHWADDR, &ifr) == 0) {
                    success = 1;
                    break;
                }
            }
        } else {
            return;
        }
    }

    unsigned char mac_address[6];
    if (success) memcpy(mac_address, ifr.ifr_hwaddr.sa_data, 6);

    sprintf(mac,
            "%02x:%02x:%02x:%02x:%02x:%02x",
            mac_address[0],
            mac_address[1],
            mac_address[2],
            mac_address[3],
            mac_address[4],
            mac_address[5]);
    close(sock);
}
#endif

/**
 * 文件大小查询。
 *
 */
long FileSize(const char* file_path) {
    struct stat st;
    if (stat(file_path, &st) == 0) {
        return st.st_size;
    }
    return -1; 
}

/**
 * 初始化设置。
 *
 * @param log
 */
static void initSetting(bool log = true)
{

    AIUISetting::setAIUIDir(TEST_ROOT_DIR_PATH);
    AIUISetting::setMscDir(MSC_DIR_PATH);
    AIUISetting::setNetLogLevel(log ? aiui_debug : aiui_none);

    long file_size = FileSize(LOG_DIR_PATH);
    if (file_size == -1) {
        std::cerr << "Error: Unable to get file size or file does not exist." << std::endl;
        return;
    }
    else {  
            if (file_size > 10*1024*1024){
            remove(LOG_DIR_PATH);
        }
    }  

    char mac[64] = {0};
    GenerateMACAddress(mac);

    // 为每一个设备设置唯一对应的序列号SN（最好使用设备硬件信息(mac地址，设备序列号等）生成），以便正确统计装机量，
    // 避免刷机或者应用卸载重装导致装机量重复计数
    AIUISetting::setSystemInfo(AIUI_KEY_SERIAL_NUM, mac);

    // 6.6.xxxx.xxxx版本设置用户唯一标识uid（可选，AIUI后台服务需要，不设置则会使用上面的SN作为uid）
    // 5.6.xxxx.xxxx版本SDK不能也不需要设置uid
    // AIUISetting::setSystemInfo(AIUI_KEY_UID, "1234567890");
}

/**************************************************************************
函数功能：语音合成文本回调函数
返回  值：无
**************************************************************************/
void AIUI_Node::tts_Callback(std_msgs::msg::String::SharedPtr msg){
    if (record_status) {
        alsaStop();
    }
    tts_text = msg->data;
    startTTS(tts_text);
}

/**************************************************************************
函数功能：唤醒标志位回调
返回  值：无
**************************************************************************/
void AIUI_Node::awake_Callback(std_msgs::msg::Int8::SharedPtr msg){
    awake_flag = msg->data;
    
    if (awake_flag == 1) {
        // 取消当前TTS
        if (g_tts_is_playing || play_status) {
            cancelTTS();
            int wait_count = 0;
            while (g_tts_cancelling && wait_count < 20) {
                std::this_thread::sleep_for(std::chrono::milliseconds(50));
                wait_count++;
            }
        }

        if (record_status) {
            alsaStop();
        }

        if (!is_wakeup_called) {
            wakeup();
            is_wakeup_called = true;
        }

        std::thread([this]() {
            std::this_thread::sleep_for(std::chrono::milliseconds(200));
            playAwakeFeedbackAndWait();
            if (audioListeningThread) {
                audioListeningThread->resetAudioDevice();
                std::this_thread::sleep_for(std::chrono::milliseconds(50));
            }
            alsaStart();
        }).detach(); 
    } 
    else{
        if (record_status) alsaStop();
    }
}

AIUI_Node::AIUI_Node(const std::string &node_name,
    const rclcpp::NodeOptions &options) : rclcpp::Node(node_name, options){
    RCLCPP_INFO(this->get_logger(),"%s node init!\n",node_name.c_str());
    this->declare_parameter<std::string>("record_device_name","default");

    this->get_parameter("record_device_name",RECORD_DEVICE_NAME);

    /***识别文本话题发布者创建***/
    voice_words_pub = this->create_publisher<std_msgs::msg::String>("voice_words",10);
    /***TTS输出文本话题订阅者创建***/
    tts_words_sub = this->create_subscription<std_msgs::msg::String>(
         "feedback_words",10,std::bind(&AIUI_Node::tts_Callback,this,std::placeholders::_1));
    /***唤醒标志位话题订阅者创建***/
    awake_flag_sub = this->create_subscription<std_msgs::msg::Int8>(
        "awake_flag",10,std::bind(&AIUI_Node::awake_Callback,this,std::placeholders::_1));
    
    RCLCPP_INFO(this->get_logger(), "Chat Client Node initialized");
}

AIUI_Node::~AIUI_Node(){
    // 停止当前TTS
    if (g_tts_is_playing) {
        cancelTTS();
    }
    destroyAgent();
    RCLCPP_INFO(this->get_logger(),"AIUI_Node over!\n");
}

void signal_handler(int) {
  if (node) {
    node->~AIUI_Node(); 
    rclcpp::shutdown();
  }
  exit(0);
}


int main(int argc, char** argv)
{
    rclcpp::init(argc,argv);
    node = std::make_shared<AIUI_Node>("aiui_node",rclcpp::NodeOptions());

    // 注册信号处理器
    signal(SIGINT, signal_handler);

    // 打印SDK版本
    std::cout << "Version: " << getVersion() << std::endl;

    initSetting();

    std::cout << "createAgent" << std::endl;
    if (nullptr == g_pListener)
        g_pListener = new DemoListener;
    createAgent();

    if (!audioListeningThread) {
        audioListeningThread = std::make_unique<AudioListenThread>(g_pAgent);
    }

    rclcpp::spin(node);
    rclcpp::shutdown();
    return 0;
}


