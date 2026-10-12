#ifndef __MOTION_CONTROL_H_
#define __MOTION_CONTROL_H_

#include <iostream>
#include <unistd.h>
#include <math.h>
#include <play_path.h>
#include <chrono> 
#include <rclcpp/rclcpp.hpp>
#include <std_msgs/msg/string.hpp>
#include <std_msgs/msg/int8.hpp>
#include <std_msgs/msg/int32.hpp>
#include <std_msgs/msg/u_int32.hpp>
#include <rclcpp_action/rclcpp_action.hpp>
#include <nav_msgs/msg/odometry.hpp>
#include <geometry_msgs/msg/vector3.hpp>
#include <geometry_msgs/msg/pose_stamped.hpp>
#include <geometry_msgs/msg/point_stamped.hpp>
#include <geometry_msgs/msg/transform_stamped.hpp>
#include <nav2_msgs/action/navigate_to_pose.hpp>
#include "tf2_geometry_msgs/tf2_geometry_msgs.h"
#include "tf2_ros/transform_listener.h"
#include "tf2_ros/buffer.h"
#include "wheeltec_mic_msg/msg/motion_control.hpp"
#include "turn_on_wheeltec_robot/msg/position.hpp"

using std::placeholders::_1;
using std::placeholders::_2;
using namespace std::chrono_literals;

class Motion : public rclcpp::Node{
public:
	using ClientT = nav2_msgs::action::NavigateToPose;
	using GoalHandle_ = rclcpp_action::ClientGoalHandle<ClientT>;
	Motion(const std::string &node_name);
	~Motion();
	void control();

private:
	float distance;
	float direction_obstacle_x;
	float lidar_range;
	double direction;
	int lidar_count;
	int angle;
	int voice_flag = 0;
	int follow_flag = 0;
	int cmd_vel_flag = 0;
	int turn_fin_flag = 0;
	int goal_reached_flag = 0;
	int goal_status = 0;
	bool if_akm;
	bool is_following_goal = false; 
	bool goal_sent = false;            
	std::vector<float> point;

	geometry_msgs::msg::Twist cmd_vel_msg;

	rclcpp::Publisher<std_msgs::msg::String>::SharedPtr voice_words_pub;
	rclcpp::Publisher<geometry_msgs::msg::Twist>::SharedPtr cmd_vel_pub;

    rclcpp::Subscription<std_msgs::msg::UInt32>::SharedPtr angle_sub;
    rclcpp::Subscription<std_msgs::msg::Int8>::SharedPtr cmd_vel_flag_sub; 
    rclcpp::Subscription<nav_msgs::msg::Odometry>::SharedPtr pose_sub;
    rclcpp::Subscription<wheeltec_mic_msg::msg::MotionControl>::SharedPtr motion_msg_sub;
    rclcpp::Subscription<turn_on_wheeltec_robot::msg::Position>::SharedPtr current_position_sub;

    rclcpp::TimerBase::SharedPtr timer_;

    rclcpp_action::Client<ClientT>::SharedPtr goal_client;

    std::shared_ptr<tf2_ros::TransformListener> tf_listener_;
    std::unique_ptr<tf2_ros::Buffer> tf_buffer_;
    
	void angle_Callback(const std_msgs::msg::UInt32::SharedPtr msg);
	void voice_flag_Callback(const std_msgs::msg::Int8::SharedPtr msg);
	void pose_Callback(const nav_msgs::msg::Odometry::SharedPtr msg);
    void motion_msg_Callback(const wheeltec_mic_msg::msg::MotionControl::SharedPtr msg);
    void current_position_Callback(const turn_on_wheeltec_robot::msg::Position::SharedPtr msg);
    void timer_callback();
    
    int motion_judgement();
    void akm_follow_turn(int angle);
    void follow_turn(int angle);

    void send_goal(std::vector<float> msg);
    void goal_response_callback(const std::shared_ptr<GoalHandle_> future);
    void feedback_callback(GoalHandle_::SharedPtr,const std::shared_ptr<const ClientT::Feedback> feedback);
    void result_callback(const GoalHandle_::WrappedResult &result);
};

#endif
