#ifndef __FEEDBACK_RECOGNITION_H_
#define __FEEDBACK_RECOGNITION_H_

#include <iostream>
#include <unistd.h>
#include <play_path.h>
#include <rclcpp/rclcpp.hpp>
#include <std_msgs/msg/string.hpp>
#include <std_msgs/msg/int8.hpp>
#include <std_msgs/msg/int32.hpp>
#include <geometry_msgs/msg/twist.hpp>
#include <geometry_msgs/msg/vector3.hpp>
#include <geometry_msgs/msg/pose_stamped.hpp>
#include <geometry_msgs/msg/point_stamped.hpp>

// #include <nav_msgs/msg/odometry.hpp>

using namespace std;
using std::placeholders::_1;

class Feedback : public rclcpp::Node{
public:
	Feedback(const std::string &node_name);
	~Feedback();
	void run();

private:
    int laser_follow_flag = 0;      //雷达跟随标志位

    geometry_msgs::msg::Twist cmd_vel_msg;

    rclcpp::Publisher<geometry_msgs::msg::Twist>::SharedPtr cmd_vel_pub;
    rclcpp::Subscription<std_msgs::msg::Int8>::SharedPtr laser_follow_sub;
    rclcpp::Subscription<std_msgs::msg::String>::SharedPtr voice_words_sub;

    void laser_follow_Callback(const std_msgs::msg::Int8::SharedPtr msg);
    void voice_words_Callback(const std_msgs::msg::String::SharedPtr msg);

    int node_kill(const char* progress);
};

#endif