#include <rclcpp/rclcpp.hpp>
#include <signal.h>
#include <stdlib.h>
#include <ctime>
#include <std_msgs/msg/int8.hpp>
#include <std_msgs/msg/string.hpp>
#include <stdio.h>
#include <bodyreader_msg/msg/bodyposture.hpp>

using namespace std;
int temp1 = 0;
int temp2 = 0;
int interaction = 0;
bool voice_feedback ;

void bodyposture_Callback(bodyreader_msg::msg::Bodyposture msg)
{
	if (voice_feedback)
	{
		if(msg.akimibo)
		{
			system("aplay -D plughw:CARD=Device,DEV=0 ~/wheeltec_ros2/src/wheeltec_bodyreader/bodyreader/audio/cal_finish.wav");
			//sleep(1);
		}

		if(msg.tips == 1)
		{
			system("aplay -D plughw:CARD=Device,DEV=0 ~/wheeltec_ros2/src/wheeltec_bodyreader/bodyreader/audio/akimbo_cal.wav");
		}
		else if (msg.tips == 2)
		{
			system("aplay -D plughw:CARD=Device,DEV=0 ~/wheeltec_ros2/src/wheeltec_bodyreader/bodyreader/audio/recovery.wav");
		}

		if(interaction==1){

			if(msg.left_foot_up == 1 && msg.right_foot_up == 0)
			{
				temp1++;
				temp2 = 0;
				if(temp1 > 5)
					{system("aplay -D plughw:CARD=Device,DEV=0 ~/wheeltec_ros2/src/wheeltec_bodyreader/bodyreader/audio/backward.wav");
					temp1 = 0;}
			}
			else if(msg.left_foot_up == 0 && msg.right_foot_up == 1)
			{
				temp1++;
				temp2 = 0;
				if(temp1 > 5)
					{system("aplay -D plughw:CARD=Device,DEV=0 ~/wheeltec_ros2/src/wheeltec_bodyreader/bodyreader/audio/forward.wav");
					temp1 = 0;}
			}
			else if(msg.left_hand_raised == 0 && msg.right_hand_raised == 1)
			{
				temp2++;
				temp1 = 0;
				if(temp2 > 5)
					{system("aplay -D plughw:CARD=Device,DEV=0 ~/wheeltec_ros2/src/wheeltec_bodyreader/bodyreader/audio/turn_right.wav");
					temp2 = 0;}
			}
			else if(msg.left_hand_raised == 1 && msg.right_hand_raised == 0)
			{
				temp2++;
				temp1 = 0;
				if(temp2 > 5)
					{system("aplay -D plughw:CARD=Device,DEV=0 ~/wheeltec_ros2/src/wheeltec_bodyreader/bodyreader/audio/turn_left.wav");
					temp2 = 0;}
			}
			else if(msg.left_arm_out == 1 && msg.right_arm_out == 0)
			{
					system("aplay -D plughw:CARD=Device,DEV=0 ~/wheeltec_ros2/src/wheeltec_bodyreader/bodyreader/audio/left_translation.wav");
					temp1 = 0;
					temp2 = 0;
			}
			else if(msg.left_arm_out == 0 && msg.right_arm_out == 1)
			{
					system("aplay -D plughw:CARD=Device,DEV=0 ~/wheeltec_ros2/src/wheeltec_bodyreader/bodyreader/audio/right_translation.wav");
					temp1 = 0;
					temp2 = 0;
			}
			else if (msg.fall == 1)
			{
					system("aplay -D plughw:CARD=Device,DEV=0 ~/wheeltec_ros2/src/wheeltec_bodyreader/bodyreader/audio/fall.wav");
					temp1 = 0;
					temp2 = 0;
			}

		}
	}
	
}


void mode_Callback(std_msgs::msg::Int8 msg)
{
	interaction = msg.data;
}



int main(int argc, char** argv)
{

  	rclcpp::init(argc, argv);
  	auto node = rclcpp::Node::make_shared("bodyreader_feedback");
  	node->declare_parameter<bool>("voice_feedback", true);
  	node->declare_parameter<int>("interaction", 0);
  	node->get_parameter("voice_feedback", voice_feedback);
  	node->get_parameter("interaction", interaction);
	
	//ros::Subscriber laser_follow_flag_sub = nd.subscribe("laser_follow_flag", 1, laser_follow_flagCallback);//雷达跟随开启标志位订阅


	//printf("interaction = %d\n",interaction);

	auto bodyposture_sub = node->create_subscription<bodyreader_msg::msg::Bodyposture>("/body_posture", 1, bodyposture_Callback);

	auto mode_sub = node->create_subscription<std_msgs::msg::Int8>("/mode", 1, mode_Callback);

	rclcpp::spin(node);



}
