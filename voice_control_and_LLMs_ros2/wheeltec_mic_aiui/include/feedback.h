#ifndef __GLOBAL_H__
#define __GLOBAL_H__

#include <iostream>
#include <unistd.h>
#include <string.h>
#include <std_msgs/msg/string.hpp>

std_msgs::msg::String feedback_text;

/*****************************Feedback_words********************************/
std::string car_front_ 		= "小车前进";
std::string car_back_ 		= "小车后退"; 
std::string turn_left_		= "小车左转";
std::string turn_right_ 	= "小车右转";
std::string stop_			= "小车停";
std::string sleep_ 			= "小车休眠";
std::string search_voice_ 	= "小车过来";
std::string Tracker_ 		= "遇到障碍物";
std::string awake_ 			= "小车唤醒";
std::string OK_ 			= "好的";
std::string rplidar_open_  	= "关闭雷达跟随";
std::string rplidar_close_ 	= "关闭雷达跟随";

/*******************************Param********************************/
std::string head = "aplay -D plughw:CARD=Device,DEV=0 ";
std::string gnome_terminal = "dbus-launch gnome-terminal -- ";
std::string simple_follower = "ros2 launch simple_follower_ros2 ";
std::string wheeltec_mic_ros2 = "ros2 launch wheeltec_mic_ros2 ";
std::string wheeltec_mic_aiui = "ros2 launch wheeltec_mic_aiui ";
std::string audio_path;
std::string WHOLE;
std::string Launch;

#endif