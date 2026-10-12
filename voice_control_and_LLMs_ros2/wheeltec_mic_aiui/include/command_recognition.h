#ifndef __CALL_COMMAND_RECOGNITION_H_
#define __CALL_COMMAND_RECOGNITION_H_

#include <iostream>
#include <vector>
#include <unistd.h>
#include <inttypes.h>
#include <feedback.h>
#include <rclcpp/rclcpp.hpp>
#include <std_msgs/msg/string.hpp>
#include <std_msgs/msg/int8.hpp>
#include <std_msgs/msg/int32.hpp>
#include <geometry_msgs/msg/twist.hpp>
#include <geometry_msgs/msg/vector3.hpp>
#include <rclcpp_action/rclcpp_action.hpp>
#include <geometry_msgs/msg/pose_stamped.hpp>
#include <nav2_msgs/action/navigate_to_pose.hpp>
#include "wheeltec_mic_msg/msg/motion_control.hpp"
using std::placeholders::_1;
using std::placeholders::_2;

enum class Car_Status {FRONT, BACK, LEFT, RIGHT, STOP};

class Command : public rclcpp::Node{
public:
	using ClientT = nav2_msgs::action::NavigateToPose;
	using GoalHandle_ = rclcpp_action::ClientGoalHandle<ClientT>;
	Command(const std::string &node_name,
         const rclcpp::NodeOptions &options);
	~Command();
	void run();

private:
	int voice_flag = 0;
	int awake_flag = 0;
	bool if_akm;
	float line_vel_x,ang_vel_z,turn_line_vel_x;
	float I_position_x,I_position_y,I_orientation_z,I_orientation_w;
	float J_position_x,J_position_y,J_orientation_z,J_orientation_w;
	float K_position_x,K_position_y,K_orientation_z,K_orientation_w;
	std::vector<float> point;
	std::string sw = "on";
	
	wheeltec_mic_msg::msg::MotionControl motion_;

	rclcpp::Publisher<std_msgs::msg::Int8>::SharedPtr awake_flag_pub,laser_follow_flag_pub;
    rclcpp::Publisher<wheeltec_mic_msg::msg::MotionControl>::SharedPtr motion_msg_pub;
    rclcpp::Publisher<geometry_msgs::msg::PoseStamped>::SharedPtr pose_nav_pub;
    rclcpp::Publisher<std_msgs::msg::String>::SharedPtr feedback_words_pub;

    rclcpp::Subscription<std_msgs::msg::Int8>::SharedPtr voice_flag_sub;
    rclcpp::Subscription<std_msgs::msg::String>::SharedPtr voice_words_sub;

    rclcpp_action::Client<ClientT>::SharedPtr goal_client;

    int node_kill(const char* progress);

    void voice_flag_Callback(const std_msgs::msg::Int8::SharedPtr msg);
    void voice_words_Callback(const std_msgs::msg::String::SharedPtr msg);

    void send_goal(std::vector<float> msg);
    void car_move(Car_Status status);
    void goal_response_callback(const std::shared_ptr<GoalHandle_> future);
    void feedback_callback(GoalHandle_::SharedPtr,const std::shared_ptr<const ClientT::Feedback> feedback);
    void result_callback(const GoalHandle_::WrappedResult &result);
	
};

#endif
