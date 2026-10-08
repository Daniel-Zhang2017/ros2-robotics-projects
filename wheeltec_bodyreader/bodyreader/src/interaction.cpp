#include <rclcpp/rclcpp.hpp>
#include <std_msgs/msg/string.hpp>
#include <bodyreader_msg/msg/bodyposture.hpp>
#include <geometry_msgs/msg/twist.hpp>
#include <std_msgs/msg/int8.hpp>

using namespace std;
 
rclcpp::Publisher<geometry_msgs::msg::Twist>::SharedPtr cmd_vel_Pub;
rclcpp::Publisher<std_msgs::msg::Int8>::SharedPtr fall_Pub;
int mode = 1;

void bodyposture_Callback(bodyreader_msg::msg::Bodyposture msg)
{
	if (mode == 1)
	{
		std_msgs::msg::Int8 fall_msg;
		geometry_msgs::msg::Twist cmd_vel_msg;
		if(msg.left_foot_up == 1 && msg.right_foot_up == 0)
		{
				cmd_vel_msg.linear.x = -0.15;
		}
		else if(msg.left_foot_up == 0 && msg.right_foot_up == 1)
		{
				cmd_vel_msg.linear.x = 0.15;
		}
		else if(msg.left_hand_raised == 0 && msg.right_hand_raised == 1)
		{
				cmd_vel_msg.angular.z = 0.2;
		}
		else if(msg.left_hand_raised == 1 && msg.right_hand_raised == 0)
		{
				cmd_vel_msg.angular.z = -0.2;
		}
		else if(msg.left_arm_out == 1 && msg.right_arm_out == 0)
		{
				cmd_vel_msg.linear.y = -0.1;
		}
		else if(msg.left_arm_out == 0 && msg.right_arm_out == 1)
		{
				cmd_vel_msg.linear.y = 0.1;
		}
		else if (msg.fall == 1)
		{
				fall_msg.data = 1;
		}
		
		fall_Pub->publish(fall_msg);
		cmd_vel_Pub->publish(cmd_vel_msg);
		

	}
	
}

void mode_Callback(std_msgs::msg::Int8 msg)
{
	mode = msg.data;
}

int main(int argc, char *argv[])
{

	rclcpp::init(argc, argv);
    auto node = rclcpp::Node::make_shared("interaction");

	cmd_vel_Pub = node->create_publisher<geometry_msgs::msg::Twist>("/cmd_vel", 1);
	fall_Pub = node->create_publisher<std_msgs::msg::Int8>("/buzzer_flag", 1);
	auto bodyposture_sub = node->create_subscription<bodyreader_msg::msg::Bodyposture>("/body_posture", 1, bodyposture_Callback);
	auto mode_sub = node->create_subscription<std_msgs::msg::Int8>("/mode", 1, mode_Callback);


	double rate = 10;    //频率10Hz
	rclcpp::Rate loopRate(rate);
	
	while(rclcpp::ok())
	{
		rclcpp::spin_some(node->get_node_base_interface());
		loopRate.sleep();

	}

}
