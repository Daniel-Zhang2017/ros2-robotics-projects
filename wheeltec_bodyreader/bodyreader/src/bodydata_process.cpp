#include <rclcpp/rclcpp.hpp>
#include <bodyreader_msg/msg/bodyposture.hpp>
#include <geometry_msgs/msg/twist.hpp>
#include <bodyreader_msg/msg/bodylist.hpp>
#include <bodyreader_msg/msg/body.hpp>
#include <std_msgs/msg/int16.hpp>
#include <std_msgs/msg/int8.hpp>
#include <geometry_msgs/msg/twist.hpp>

#define HEAD 0
#define SHOULDER_SPINE 1
#define LEFT_SHOULDER 2
#define LEFT_ELBOW 3
#define LEFT_HAND 4
#define RIGHT_SHOULDER 5
#define RIGHT_ELBOW 6
#define RIGHT_HAND 7
#define MID_SPINE 8
#define BASE_SPINE 9
#define LEFT_HIP 10
#define LEFT_KNEE 11
#define LEFT_FOOT 12
#define RIGHT_HIP 13
#define RIGHT_KNEE 14
#define RIGHT_FOOT 15
#define LEFT_WRIST 16
#define RIGHT_WRIST 17
#define NECK 18
#define UNKNOWN 255
 
 
int lock_body_id = 0;
int lock_status = 0;		//0-nobody  1-no lock    2-locked
int last_lock_status = 0;
//int ii=0  ; 

bool open_switch = false;

rclcpp::Publisher<bodyreader_msg::msg::Bodyposture>::SharedPtr bodyposture_Pub;
bodyreader_msg::msg::Bodyposture bodyposture_msg;

rclcpp::Publisher<std_msgs::msg::Int8>::SharedPtr mode_Pub;
std_msgs::msg::Int8 mode_msg;

rclcpp::Publisher<geometry_msgs::msg::Twist>::SharedPtr cmd_vel_Pub;

void judge_pose(bodyreader_msg::msg::Body body)
{	
	if(abs((int)(body.joints[LEFT_SHOULDER].worldposition.y - body.joints[LEFT_ELBOW].worldposition.y)) < 100  
		&&  abs((int)(body.joints[LEFT_ELBOW].worldposition.y - body.joints[LEFT_HAND].worldposition.y)) < 150 
		&&  (body.joints[LEFT_SHOULDER].worldposition.x - body.joints[LEFT_ELBOW].worldposition.x) > 200
		&&  (body.joints[LEFT_ELBOW].worldposition.x - body.joints[LEFT_HAND].worldposition.x) > 200)
    {
		if(body.bodyid == lock_body_id)
		{
			bodyposture_msg.left_arm_out = 1;
			printf("Left Arm out !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n");
		}
	}

	if(abs((int)(body.joints[RIGHT_SHOULDER].worldposition.y - body.joints[RIGHT_ELBOW].worldposition.y)) < 100  
		&&  abs((int)(body.joints[RIGHT_ELBOW].worldposition.y - body.joints[RIGHT_HAND].worldposition.y)) < 150 
		&&  (body.joints[RIGHT_ELBOW].worldposition.x - body.joints[RIGHT_SHOULDER].worldposition.x) > 200
		&&  (body.joints[RIGHT_HAND].worldposition.x - body.joints[RIGHT_ELBOW].worldposition.x) > 200)
    {
		if(body.bodyid == lock_body_id)
		{
			bodyposture_msg.right_arm_out = 1;
			printf("Right Arm out !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n");
		}
	}

	if((body.joints[LEFT_HAND].worldposition.y - body.joints[LEFT_ELBOW].worldposition.y) > 130
         && (body.joints[LEFT_SHOULDER].worldposition.y - body.joints[LEFT_ELBOW].worldposition.y) > 80)
    {
		if(body.bodyid == lock_body_id)
		{
			bodyposture_msg.left_hand_raised = 1;
			printf("Left hand raised !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n");
		}
	}

	if((body.joints[RIGHT_HAND].worldposition.y - body.joints[RIGHT_ELBOW].worldposition.y) > 130
         && (body.joints[RIGHT_SHOULDER].worldposition.y - body.joints[RIGHT_ELBOW].worldposition.y) > 80)
    {
		if(body.bodyid == lock_body_id)
		{
			bodyposture_msg.right_hand_raised = 1;
			printf("Right hand raised !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n");
		}
	}

	if((body.joints[LEFT_HAND].worldposition.y - body.joints[BASE_SPINE].worldposition.y) > 50  
		&& (body.joints[RIGHT_HAND].worldposition.y - body.joints[BASE_SPINE].worldposition.y) > 50
		&& abs((int)(body.joints[LEFT_SHOULDER].worldposition.x - body.joints[LEFT_HAND].worldposition.x)) < 100
		//&& (body.joints[LEFT_HAND].worldposition.x - body.joints[LEFT_ELBOW].worldposition.x) > 100
		//&& (body.joints[RIGHT_SHOULDER].worldposition.x - body.joints[RIGHT_ELBOW].worldposition.x) < -100
		//&& (body.joints[RIGHT_HAND].worldposition.x - body.joints[RIGHT_ELBOW].worldposition.x) < -100
		&& abs((int)(body.joints[RIGHT_SHOULDER].worldposition.x - body.joints[RIGHT_HAND].worldposition.x)) < 100
		&& (body.joints[RIGHT_SHOULDER].worldposition.y - body.joints[RIGHT_HAND].worldposition.y) > 50
		&& (body.joints[LEFT_SHOULDER].worldposition.y - body.joints[LEFT_HAND].worldposition.y) > 50
            )
    {
		printf("Akimibo !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n");
		lock_body_id = body.bodyid;
		bodyposture_msg.akimibo = 1;
	}

	if((body.joints[LEFT_FOOT].worldposition.y - body.joints[RIGHT_FOOT].worldposition.y) > 120  
              && (body.joints[BASE_SPINE].worldposition.y - body.joints[LEFT_FOOT].worldposition.y) > 200
              && (body.joints[BASE_SPINE].worldposition.y - body.joints[RIGHT_FOOT].worldposition.y) > 500)
      {
		if(body.bodyid == lock_body_id)
		{
			bodyposture_msg.left_foot_up = 1;
			printf("Left foot up !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n");
		}
	}

	if((body.joints[RIGHT_FOOT].worldposition.y - body.joints[LEFT_FOOT].worldposition.y) > 120  
              && (body.joints[BASE_SPINE].worldposition.y - body.joints[LEFT_FOOT].worldposition.y) > 500
              && (body.joints[BASE_SPINE].worldposition.y - body.joints[RIGHT_FOOT].worldposition.y) > 200)
    {
		if(body.bodyid == lock_body_id)
		{
			bodyposture_msg.right_foot_up = 1;
			printf("Right foot up !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n");
		}
	}


	if((body.joints[LEFT_KNEE].worldposition.y - body.joints[BASE_SPINE].worldposition.y) > -10 
		&& (body.joints[RIGHT_KNEE].worldposition.y - body.joints[BASE_SPINE].worldposition.y) > -10
              )
    {
		if(body.bodyid == lock_body_id)
		{
			bodyposture_msg.fall = 1;
			printf("Fall !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n");
		}
	}


	if (open_switch){
		if((body.joints[LEFT_HAND].worldposition.x - body.joints[BASE_SPINE].worldposition.x) > 0 
			&& (body.joints[BASE_SPINE].worldposition.x - body.joints[RIGHT_HAND].worldposition.x) > 0
	              ) 
	    {
			if(body.bodyid == lock_body_id)
			{
				
				printf("Switch mode !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n");

				if (mode_msg.data == 1) mode_msg.data = 2;
				else if (mode_msg.data == 2) mode_msg.data = 1;
				mode_Pub->publish(mode_msg);
				system("aplay -D plughw:CARD=Device,DEV=0 ~/wheeltec_ros2/src/bodyreader/audio/mode_switch.wav");
			}
		}
	}
}

void bodylist_Callback(bodyreader_msg::msg::Bodylist body_list)
{
	bodyposture_msg.bodyid = 0;
	bodyposture_msg.centerofmass_x = 0;
    bodyposture_msg.centerofmass_y = 0;
    bodyposture_msg.centerofmass_z = 0;
    bodyposture_msg.left_arm_out = 0;
    bodyposture_msg.right_arm_out = 0;
    bodyposture_msg.left_hand_raised = 0;
    bodyposture_msg.right_hand_raised = 0;
    bodyposture_msg.akimibo = 0;
    bodyposture_msg.left_foot_up = 0;
    bodyposture_msg.right_foot_up = 0;
    bodyposture_msg.fall = 0;
    bodyposture_msg.tips =0;
    bodyposture_msg.lock_status =0;

	if(body_list.count !=0) lock_status=1;
    else lock_status=0;

	for(int i = 0; i < body_list.count; ++i)
    {
    	bodyreader_msg::msg::Body body = body_list.bodies[i];

    	judge_pose(body);

    	if(body.bodyid == lock_body_id)
		{
			bodyposture_msg.centerofmass_x = body.centerofmass.x;
			bodyposture_msg.centerofmass_y = body.centerofmass.y;
			bodyposture_msg.centerofmass_z = body.centerofmass.z;

			lock_status = 2;
			bodyposture_msg.bodyid = lock_body_id;
		}
		
    }
    if(lock_status == 1 && last_lock_status != lock_status) bodyposture_msg.tips = 1;
    last_lock_status = lock_status ;


    if(lock_status == 2) 
    {
       if((bodyposture_msg.centerofmass_x / bodyposture_msg.centerofmass_z > 0.35)
           ||(bodyposture_msg.centerofmass_x / bodyposture_msg.centerofmass_z < -0.35)
           ||(bodyposture_msg.centerofmass_z < 1400))
       {
              bodyposture_msg.left_arm_out = 0;
              bodyposture_msg.right_arm_out = 0;
              bodyposture_msg.left_hand_raised = 0;
              bodyposture_msg.right_hand_raised = 0;
              bodyposture_msg.akimibo = 0;
              bodyposture_msg.left_foot_up = 0;
              bodyposture_msg.right_foot_up = 0;
              bodyposture_msg.fall = 0;
       }
    }
    else 
    {
    	cmd_vel_Pub->publish(geometry_msgs::msg::Twist());
    }
	bodyposture_msg.lock_status = lock_status;
    bodyposture_Pub->publish(bodyposture_msg);

}

void recoveryid_Callback(std_msgs::msg::Int16 recoveryid)
{
	lock_body_id = recoveryid.data;
	bodyposture_msg.tips = 2;
}

int main(int argc, char *argv[])
{
  	rclcpp::init(argc, argv);
  	auto node = rclcpp::Node::make_shared("body_process");

	bodyposture_Pub = node->create_publisher<bodyreader_msg::msg::Bodyposture>("/body_posture", 1);
	auto bodylist_sub = node->create_subscription<bodyreader_msg::msg::Bodylist>("/bodylist", 1, bodylist_Callback);
	auto recoveryid_sub = node->create_subscription<std_msgs::msg::Int16>("/recoveryid", 1, recoveryid_Callback);

	mode_Pub = node->create_publisher<std_msgs::msg::Int8>("/mode", 1);
	mode_msg.data = 1;

	cmd_vel_Pub = node->create_publisher<geometry_msgs::msg::Twist>("cmd_vel", 1);

	node->declare_parameter<bool>("open_switch", false);
  	node->get_parameter("open_switch", open_switch);


	rclcpp::spin(node);
}
