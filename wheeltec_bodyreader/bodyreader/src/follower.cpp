#include <rclcpp/rclcpp.hpp>
#include <std_msgs/msg/string.hpp>
#include <bodyreader_msg/msg/bodyposture.hpp>
#include <geometry_msgs/msg/twist.hpp>
#include <std_msgs/msg/int8.hpp>

using namespace std;

rclcpp::Publisher<geometry_msgs::msg::Twist>::SharedPtr cmd_vel_Pub;

float target_x_angle = 0;
float target_distance = 2000;
float x_p = 0;
float x_d = 0;
float z_p = 0;
float z_d = 0;
int mode  = 2;   //1:sleep  2:follow

void bodyposture_Callback(bodyreader_msg::msg::Bodyposture msg)
{
	if (mode == 2)
	{
		geometry_msgs::msg::Twist cmd_vel_msg;
		static float last_x_angle = 0;
		static float last_distance = 0;
		float x_angle;
		float distance;

		if(msg.centerofmass_x == 0 && msg.centerofmass_y == 0 && msg.centerofmass_z == 0)
		{
			x_angle = target_x_angle;
			distance = target_distance;
		}
		else{
		x_angle = msg.centerofmass_x / msg.centerofmass_z;
		distance = msg.centerofmass_z;
		}
		//printf("x_angle=%f\n",x_angle);
		//printf("distance=%f\n",distance);
		
		float error_x_angle = x_angle - target_x_angle;
		float error_distance = distance - target_distance;
		if(error_x_angle > -0.01 && error_x_angle < 0.01)  error_x_angle = 0;
		if(error_distance > -80 && error_distance < 80) error_distance = 0;

		cmd_vel_msg.linear.x = error_distance*x_p/1000 + (error_distance - last_distance)*x_d/1000;
		if(cmd_vel_msg.linear.x < -0.5)  cmd_vel_msg.linear.x = -0.5;
		else if(cmd_vel_msg.linear.x > 0.5) cmd_vel_msg.linear.x = 0.5;
		
		cmd_vel_msg.angular.z = error_x_angle*z_p + (error_x_angle - last_x_angle)*z_d;
		if(cmd_vel_msg.angular.z < -1)  cmd_vel_msg.angular.z = -1;
		else if(cmd_vel_msg.angular.z > 1) cmd_vel_msg.angular.z = 1;
		
		cmd_vel_Pub->publish(cmd_vel_msg);

		last_x_angle = error_x_angle;
		last_distance = error_distance;
	}
	
}


void mode_Callback(std_msgs::msg::Int8 msg)
{
	mode = msg.data;
}


int main(int argc, char *argv[])
{

	rclcpp::init(argc, argv);
    auto node = rclcpp::Node::make_shared("follower");

	cmd_vel_Pub = node->create_publisher<geometry_msgs::msg::Twist>("/cmd_vel", 1);
	auto bodyposture_sub = node->create_subscription<bodyreader_msg::msg::Bodyposture>("/body_posture", 1, bodyposture_Callback);
	auto mode_sub = node->create_subscription<std_msgs::msg::Int8>("/mode", 1, mode_Callback);

	node->declare_parameter<float>("bodyfollow_x_p", 0.01);
  	node->declare_parameter<float>("bodyfollow_x_d", 0.01);
    node->declare_parameter<float>("bodyfollow_z_p", 0.01);
  	node->declare_parameter<float>("bodyfollow_z_d", 0.01);
  	node->declare_parameter<int>("mode", 2);
  	node->get_parameter("bodyfollow_x_p", x_p);
  	node->get_parameter("bodyfollow_x_d", x_d);
  	node->get_parameter("bodyfollow_z_p", z_p);
  	node->get_parameter("bodyfollow_z_d", z_d);
  	node->get_parameter("mode", mode);

	double rate = 10;    //频率10Hz
	rclcpp::Rate loopRate(rate);

/*	printf("bodyfollow_x_p = %f\n", x_p);
	printf("bodyfollow_x_d = %f\n", x_d);
	printf("bodyfollow_z_p = %f\n", z_p);
	printf("bodyfollow_z_d = %f\n", z_d);*/
	
	while(rclcpp::ok())
	{

		rclcpp::spin_some(node->get_node_base_interface());
		loopRate.sleep();

	}

}
