#ifndef __CALL_RECONNITION_H_
#define __CALL_RECONNITION_H_

#include <iostream>
#include <string>
#include <time.h>
#include <thread>
#include <vector>
#include "rclcpp/rclcpp.hpp"
#include <std_msgs/msg/string.hpp>
#include <std_msgs/msg/int8.hpp>
#include <std_msgs/msg/int32.hpp>
#include <wheeltec_mic_msg/srv/get_offline_result.hpp>
using namespace std;

class Call : public rclcpp::Node{
public:
    Call(const std::string &node_name,
         const rclcpp::NodeOptions &options);
    ~Call();

    int request_();
    void server_callback_(rclcpp::Client<wheeltec_mic_msg::srv::GetOfflineResult>::SharedFuture msg);

private:
    bool call_;
    int awake_flag = 0;                         //唤醒标志位
    int confidence_threshold = 20;              //置信度
    int seconds_per_order = 10;                 //录音时长
    int recognize_fail_count = 0;               //被动休眠相关变量
    int recognize_fail_count_threshold = 15;    //识别失败次数阈值

    rclcpp::Publisher<std_msgs::msg::String>::SharedPtr voice_words_pub;
    rclcpp::Publisher<std_msgs::msg::Int8>::SharedPtr awake_flag_pub;
    rclcpp::Subscription<std_msgs::msg::Int8>::SharedPtr awake_flag_sub;
    rclcpp::Client<wheeltec_mic_msg::srv::GetOfflineResult>::SharedPtr Call_Client_;

    void awake_flag_Callback(const std_msgs::msg::Int8::SharedPtr msg);

};


#endif