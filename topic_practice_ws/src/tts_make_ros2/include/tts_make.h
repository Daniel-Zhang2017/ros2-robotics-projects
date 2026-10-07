#ifndef __TTS_MAKE_H_
#define __TTS_MAKE_H_

#include <time.h>
#include <thread>
#include <vector>
#include "rclcpp/rclcpp.hpp"
#include <std_msgs/msg/string.hpp>
#include <std_msgs/msg/int8.hpp>
#include <std_msgs/msg/int32.hpp>

using namespace std;

class TTS : public rclcpp::Node{
public:
    TTS(const std::string &node_name,
         const rclcpp::NodeOptions &options);
    ~TTS();

    int init();

private:
    string source_path;
    string appid;
    string voice_name;
    string tts_text;
    int rdn;
    int volume;
    int pitch;
    int speed;
    int sample_rate;
    char* result;
    const char* params_l;
    const char* params_s;
    const char* params_f;
    const char* params_t;

    string current_time();
    string ws2s(const std::wstring &wstr);
    wstring s2ws(const std::string &str);

    int text_to_speech(const char* src_text, const char* des_path, const char* params);
};

#endif