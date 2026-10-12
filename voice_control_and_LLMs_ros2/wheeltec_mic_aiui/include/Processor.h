#ifndef PROCESSOR_H_
#define PROCESSOR_H_

#ifdef WIN32
    #include <windows.h>

    #define _HAS_STD_BYTE 0
    #define AIUI_SLEEP Sleep
#else
    #include <unistd.h>

    #define AIUI_SLEEP(x) usleep(x * 1000)
#endif

#undef AIUI_LIB_COMPILING

#include <cstring>
#include <fstream>
#include <iostream>
#include <cstdio> 
#include <future>
#include <queue>
#include <mutex>
#include <sys/stat.h>
#include <feedback.h>

#include "aiui/AIUI_V2.h"
#include "aiui/PcmPlayer_C.h"
#include "json/json.h"
#include "../src/utils/StreamNlpTtsHelper.h"
#include "../src/utils/IatResultUtil.h"
#include "../src/utils/Base64Util.h"
#include "AudioListenThread.h"
#include "PCMPlayer.h"

#include "rclcpp/rclcpp.hpp"
#include <std_msgs/msg/string.hpp>
#include <std_msgs/msg/int8.hpp>
#include <signal.h>

#define AIUI_V2

// 是否使用语义后合成。当在AIUI平台应用配置页面打开"语音合成"开关时，需要打开该宏
//#define USE_POST_SEMANTIC_TTS

using namespace aiui_va;
using namespace aiui_v2;

#define CURRENT_PATH "/home/wheeltec/wheeltec_ros2/src/wheeltec_mic_aiui"

IAIUIAgent* g_pAgent = nullptr;

std::mutex threadMutex;
std::unique_ptr<AudioListenThread> audioListeningThread;
std::unique_ptr<PCMPlayer> player;

// 添加TTS管理变量
bool g_tts_is_playing = false;      // TTS是否正在播放
std::string g_current_tts_sid;      // 当前TTS的SID

#define SEND_AIUIMESSAGE(cmd, arg1, arg2, params, data)                               \
    do {                                                                         \
        if (!g_pAgent) break;                                                       \
        IAIUIMessage* msg = IAIUIMessage::create(cmd, arg1, arg2, params, data); \
        g_pAgent->sendMessage(msg);                                                 \
        msg->destroy();                                                          \
    } while (false)

#define SEND_AIUIMESSAGE4(cmd, arg1, arg2, params) SEND_AIUIMESSAGE(cmd, arg1, arg2, params, nullptr)
#define SEND_AIUIMESSAGE3(cmd, arg1, arg2)              SEND_AIUIMESSAGE4(cmd, arg1, arg2, "")
#define SEND_AIUIMESSAGE2(cmd, arg1)                         SEND_AIUIMESSAGE3(cmd, arg1, 0)
#define SEND_AIUIMESSAGE1(cmd)                                    SEND_AIUIMESSAGE2(cmd, 0)

class DemoListener : public IAIUIListener
{
private:
    class TtsHelperListener : public StreamNlpTtsHelper::Listener{
        public:
            void onText(const StreamNlpTtsHelper::OutTextSeg& textSeg) override;
            void onFinish(const string& fullText) override;
            void onTtsData(const Json::Value& bizParamJson, const char* audio, int len) override;
    };

private:
    std::shared_ptr<StreamNlpTtsHelper> m_pTtsHelper;

public:
    DemoListener();
    ~DemoListener();
    void onEvent(const IAIUIEvent& event) override;
    bool mMoreDetails = true;

private:
    fstream mFs;

    // 当前合成sid
    std::string mCurTtsSid;

    // 当前识别sid
    std::string mCurIatSid;

    // 识别结果缓存
    std::string mIatTextBuffer;

    // 流式nlp的应答语缓存
    std::string mStreamNlpAnswerBuffer;

    // 意图的数量
    int mIntentCnt = 0;

private:
    static void processIntentJson(Json::Value& params, Json::Value& intentJson, std::string& resultStr, 
                                int eosRsltTime,std::string& sid);
    void handleEvent(const IAIUIEvent& event);

};

DemoListener* g_pListener = nullptr;

class  AIUI_Node : public rclcpp::Node{
public:
    AIUI_Node(const std::string &node_name,
        const rclcpp::NodeOptions &options);
    ~AIUI_Node();
    void sendMessage(const std::string& message);
    void processTtsQueue(); 
    void onTtsFinished();
    void addToTtsQueue(const std::string& text);
    rclcpp::Publisher<std_msgs::msg::String>::SharedPtr voice_words_pub;

private:
    int awake_flag = 0;                         
    bool is_wakeup_called = false;
    bool waiting_for_response_ = false;
    std::string tts_text; 
    rclcpp::Subscription<std_msgs::msg::String>::SharedPtr tts_words_sub;
    rclcpp::Subscription<std_msgs::msg::Int8>::SharedPtr awake_flag_sub;

    void tts_Callback(std_msgs::msg::String::SharedPtr msg);
    void awake_Callback(std_msgs::msg::Int8::SharedPtr msg);
    std::string removeTags(const std::string& input);
};
std::shared_ptr<AIUI_Node> node;

#endif /* PROCESSOR_H_ */