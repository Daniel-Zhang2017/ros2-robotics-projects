/************************************************************************************************/
/* Copyright (c) 2025 WHEELTEC Technology, Inc   												*/
/* function:Command controller, command word recognition results into the corresponding action	*/
/* 功能：命令控制器，命令词识别结果转化为对应的执行动作													*/
/************************************************************************************************/
#include "command_recognition.h"
using std::placeholders::_1;

/**************************************************************************
函数功能：获取节点进程pid
返回  值：int pid
**************************************************************************/
int Command::node_kill(const char*  progress)
{
	char get_pid[32] = "pgrep -f ";
	strcat(get_pid,progress);
	FILE *fp = popen(get_pid,"r");
	if (fp == NULL)
	{
		printf("popen failed,get_pid = %s",get_pid);
		return -1;
	}
	
	char pid[16] = {0};
	fgets(pid,16,fp);
	if (strlen(pid) == 0)
	{
		pclose(fp);
		return -1;
	}
	pclose(fp);
	
	char cmd[32] = "kill -9 ";
	strcat(cmd,pid);
	system(cmd);
	return 0;
}

/**************************************************************************
函数功能：发送导航目标点
返回  值：无
**************************************************************************/
void Command::send_goal(std::vector<float> msg)
{
	if (!goal_client->wait_for_action_server())
	{
		RCLCPP_ERROR(this->get_logger(), "Action server not available!");
        rclcpp::shutdown();
	}

	auto goal = ClientT::Goal();
	goal.pose.header.stamp = this->now();
	goal.pose.header.frame_id = "map";
	goal.pose.pose.position.x = msg.at(0);
	goal.pose.pose.position.y = msg.at(1);
	goal.pose.pose.orientation.z = msg.at(2);
	goal.pose.pose.orientation.w = msg.at(3);

	RCLCPP_INFO(this->get_logger(), "Sending goal");

	auto send_goal_options = rclcpp_action::Client<ClientT>::SendGoalOptions();
	send_goal_options.goal_response_callback = 
		std::bind(&Command::goal_response_callback,this,_1);
	send_goal_options.feedback_callback = 
		std::bind(&Command::feedback_callback,this,_1,_2);
	send_goal_options.result_callback = 
		std::bind(&Command::result_callback,this,_1);

	goal_client->async_send_goal(goal,send_goal_options);
}

/**************************************************************************
函数功能：动作目标响应回调函数
返回  值：无
**************************************************************************/
void Command::goal_response_callback(const std::shared_ptr<GoalHandle_> future)
{
	auto goal_handle = future.get();
	if (!goal_handle)
	{
		RCLCPP_ERROR(this->get_logger(), "Goal was rejected by server");
	}
	else RCLCPP_INFO(this->get_logger(), "Goal accepted by server, waiting for result");
}

/**************************************************************************
函数功能：动作反馈信息处理回调函数
返回  值：无
**************************************************************************/
void Command::feedback_callback(GoalHandle_::SharedPtr,const std::shared_ptr<const ClientT::Feedback> feedback)
{
	// RCLCPP_INFO(this->get_logger(),"Remaining Distance from Destination: %f",feedback->distance_remaining);
}

/**************************************************************************
函数功能：动作结果回调函数
返回  值：无
**************************************************************************/
void Command::result_callback(const GoalHandle_::WrappedResult &result)
{
	switch(result.code)
	{
		case rclcpp_action::ResultCode::SUCCEEDED:
			feedback_text.data = "已到达目标点";
			feedback_words_pub->publish(feedback_text);
			break;
		case rclcpp_action::ResultCode::ABORTED:
			RCLCPP_ERROR(this->get_logger(), "Goal was aborted");
		  	return;
		case rclcpp_action::ResultCode::CANCELED:
			RCLCPP_ERROR(this->get_logger(), "Goal was canceled");
			return;
		default:
        	RCLCPP_ERROR(this->get_logger(), "Unknown result code");
        	return;
	}
	RCLCPP_INFO(this->get_logger(), "Result received");
}

/**************************************************************************
函数功能：寻找语音开启成功标志位sub回调函数
入口参数：voice_flag_msg  voice_control.cpp
返回  值：无
**************************************************************************/
void Command::voice_flag_Callback(std_msgs::msg::Int8::SharedPtr msg){
	voice_flag = msg->data;
	if (voice_flag){
		feedback_text.data = "语音打开成功";
		feedback_words_pub->publish(feedback_text);
		std::cout<<"语音打开成功"<<std::endl;
	}
}

/**************************************************************************
函数功能：寻找语音开启成功标志位sub回调函数
入口参数：status 五个小车基础运动状态
返回  值：无
**************************************************************************/
void Command::car_move(Car_Status status){
	motion_.linear_x = 0;
	motion_.angular_z = 0;
	motion_.cmd_vel_flag =1;
	motion_.follow_flag = 0;

	switch (status) {
		case Car_Status::FRONT:
			motion_.linear_x = line_vel_x;
			break;
		case Car_Status::BACK:
			motion_.linear_x = -line_vel_x;
			break;
		case Car_Status::LEFT:
			motion_.linear_x = turn_line_vel_x;
			motion_.angular_z = ang_vel_z;
			break;
		case Car_Status::RIGHT:
			motion_.linear_x = turn_line_vel_x;
			motion_.angular_z = -ang_vel_z;
			break;
		case Car_Status::STOP:
			break;
    }

    motion_msg_pub->publish(motion_);
}


/**************************************************************************
函数功能：识别结果sub回调函数
入口参数：命令词字符串
返回  值：无
**************************************************************************/
void Command::voice_words_Callback(std_msgs::msg::String::SharedPtr msg){
	/***语音指令***/
	std::string str1 = msg->data;    //取传入数据
	std::string str2 = "小车前进";
	std::string str3 = "小车后退"; 
	std::string str4 = "小车左转";
	std::string str5 = "小车右转";
	std::string str6 = "小车停";
	std::string str7 = "小车休眠";
	std::string str8 = "小车过来";
	std::string str9 = "小车去i点";
	std::string str10 = "小车去j点";
	std::string str11 = "小车去k点";
	std::string str12 = "失败5次";
	std::string str13 = "失败10次";
	std::string str14 = "遇到障碍物";
	std::string str15 = "小车雷达跟随";
	std::string str16 = "关闭雷达跟随";
/***********************************
指令：小车前进
动作：底盘运动控制器使能，发布速度指令
***********************************/
	if (str1 == str2){
		car_move(Car_Status::FRONT);
		feedback_text.data = "小车前进";
		feedback_words_pub->publish(feedback_text);
		std::cout<<"好的：小车前进"<<std::endl;
	}
/***********************************
指令：小车后退
动作：底盘运动控制器使能，发布速度指令
***********************************/
	else if (str1 == str3){
		car_move(Car_Status::BACK);
		feedback_text.data = "小车后退";
		feedback_words_pub->publish(feedback_text);
		std::cout<<"好的：小车后退"<<std::endl;
	}
/***********************************
指令：小车左转
动作：底盘运动控制器使能，发布速度指令
***********************************/
	else if (str1 == str4){
		car_move(Car_Status::LEFT);
		feedback_text.data = "小车左转";
		feedback_words_pub->publish(feedback_text);
		std::cout<<"好的：小车左转"<<std::endl;
	}
/***********************************
指令：小车右转
动作：底盘运动控制器使能，发布速度指令
***********************************/
	else if (str1 == str5){
		car_move(Car_Status::RIGHT);
		feedback_text.data = "小车右转";
		feedback_words_pub->publish(feedback_text);
		std::cout<<"好的：小车右转"<<std::endl;
	}
/***********************************
指令：小车停
动作：底盘运动控制器失能，发布速度空指令
***********************************/
	else if (str1 == str6){
		car_move(Car_Status::STOP);
		feedback_text.data = "小车停";
		feedback_words_pub->publish(feedback_text);
		std::cout<<"好的：小车停"<<std::endl;
	}
/***********************************************
指令：小车休眠
动作：底盘运动控制器失能，发布速度空指令，唤醒标志位置零
***********************************************/
	else if (str1 == str7){
		std_msgs::msg::Int8 awake_flag_msg;
		awake_flag_msg.data = 0;
		awake_flag_pub->publish(awake_flag_msg);

		feedback_text.data = "小车休眠";
		feedback_words_pub->publish(feedback_text);
		std::cout<<"小车休眠，等待下一次唤醒"<<std::endl;
	}
/***********************************
指令：小车过来
动作：寻找声源标志位置位
***********************************/
	else if (str1 == str8){
		motion_.linear_x = 0;
		motion_.angular_z = 0;
		motion_.cmd_vel_flag =0;
		motion_.follow_flag = 1;
		motion_msg_pub->publish(motion_);
		feedback_text.data = "小车寻找声源";
		feedback_words_pub->publish(feedback_text);
		std::cout<<"好的：小车寻找声源"<<std::endl;
	}
/***********************************
指令：小车去I点
动作：底盘运动控制器失能(导航控制)，发布目标点
***********************************/
	else if (str1 == str9){
		geometry_msgs::msg::PoseStamped point_I;
		if (!point.empty()) point.clear();
		point.push_back(I_position_x);
		point.push_back(I_position_y);
		point.push_back(I_orientation_z);
		point.push_back(I_orientation_w);
		send_goal(point);
		feedback_text.data = "好的";
		feedback_words_pub->publish(feedback_text);
		std::cout<<"好的：小车自主导航至I点"<<std::endl;
	}
/***********************************
指令：小车去J点
动作：底盘运动控制器失能(导航控制)，发布目标点
***********************************/
	else if (str1 == str10){
		if (!point.empty()) point.clear();
		point.push_back(J_position_x);
		point.push_back(J_position_y);
		point.push_back(J_orientation_z);
		point.push_back(J_orientation_w);
		send_goal(point);
		feedback_text.data = "好的";
		feedback_words_pub->publish(feedback_text);
		std::cout<<"好的：小车自主导航至J点"<<std::endl;
	}
/***********************************
指令：小车去K点
动作：底盘运动控制器失能(导航控制)，发布目标点
***********************************/
	else if (str1 == str11){
		if (!point.empty()) point.clear();
		point.push_back(K_position_x);
		point.push_back(K_position_y);
		point.push_back(K_orientation_z);
		point.push_back(K_orientation_w);
		send_goal(point);
		feedback_text.data = "好的";
		feedback_words_pub->publish(feedback_text);
		std::cout<<"好的：小车自主导航至K点"<<std::endl;
	}
	else if (str1 == str12){
		std::cout<<"您已经连续【输入空指令or识别失败】5次，累计达15次自动进入休眠，输入有效指令后计数清零"<<std::endl;
	}
	else if (str1 == str13){
		std::cout<<"您已经连续【输入空指令or识别失败】10次，累计达15次自动进入休眠，输入有效指令后计数清零"<<std::endl;
	}
/***********************************
辅助指令：遇到障碍物
动作：用户界面打印提醒
***********************************/
	else if (str1 == str14){
		feedback_text.data = "遇到障碍物";
		feedback_words_pub->publish(feedback_text);
		std::cout<<"小车遇到障碍物，已停止运动"<<std::endl;
	}

/***********************************
辅助指令：小车雷达跟随
动作：用户界面打印提醒并开启节点
***********************************/
	else if (str1 == str15 && sw == "on"){
		sw = "off";
		feedback_text.data = "好的";
		feedback_words_pub->publish(feedback_text);

		std_msgs::msg::Int8 laser_follow_flag_msg;
		laser_follow_flag_msg.data = 0;
		laser_follow_flag_pub->publish(laser_follow_flag_msg);

		Launch = gnome_terminal + wheeltec_mic_ros2 + "laserfollower.launch.py";
		system(Launch.c_str());
		std::cout<<"好的：小车雷达跟随"<<std::endl;
	}
/***********************************
辅助指令：关闭雷达跟随
动作：用户界面打印提醒并关闭节点
***********************************/
	else if (str1 == str16 && sw == "off"){
		sw = "on";
		feedback_text.data = "好的";
		feedback_words_pub->publish(feedback_text);
		std::cout<<"好的：关闭雷达跟随"<<std::endl;
	}
}

Command::Command(const std::string &node_name,
	const rclcpp::NodeOptions &options)
: rclcpp::Node(node_name,options){
	RCLCPP_INFO(this->get_logger(),"%s node init!\n",node_name.c_str());
	/***声明参数并获取***/
	this->declare_parameter<std::string>("audio_path","");
	this->declare_parameter<bool>("if_akm_yes_or_no",false);
	this->declare_parameter<float>("line_vel_x",0.2);
	this->declare_parameter<float>("ang_vel_z",0.2);
	this->declare_parameter<float>("I_position_x",1);
	this->declare_parameter<float>("I_position_y",0);
	this->declare_parameter<float>("I_orientation_z",0);
	this->declare_parameter<float>("I_orientation_w",1);
	this->declare_parameter<float>("J_position_x",1);
	this->declare_parameter<float>("J_position_y",0);
	this->declare_parameter<float>("J_orientation_z",0);
	this->declare_parameter<float>("J_orientation_w",1);
	this->declare_parameter<float>("K_position_x",1);
	this->declare_parameter<float>("K_position_y",0);
	this->declare_parameter<float>("K_orientation_z",0);
	this->declare_parameter<float>("K_orientation_w",1);
	this->get_parameter("audio_path",audio_path);
	this->get_parameter("line_vel_x",line_vel_x);
	this->get_parameter("ang_vel_z",ang_vel_z);
	this->get_parameter("if_akm_yes_or_no",if_akm);
	this->get_parameter("I_position_x",I_position_x);
	this->get_parameter("I_position_y",I_position_y);
	this->get_parameter("I_orientation_z",I_orientation_z);
	this->get_parameter("I_orientation_w",I_orientation_w);
	this->get_parameter("J_position_x",J_position_x);
	this->get_parameter("J_position_y",J_position_y);
	this->get_parameter("J_orientation_z",J_orientation_z);
	this->get_parameter("J_orientation_w",J_orientation_w);
	this->get_parameter("K_position_x",K_position_x);
	this->get_parameter("K_position_y",K_position_y);
	this->get_parameter("K_orientation_z",K_orientation_z);
	this->get_parameter("K_orientation_w",K_orientation_w);

	/***唤醒标志位话题发布者创建***/
	awake_flag_pub = this->create_publisher<std_msgs::msg::Int8>("awake_flag",10); 
	/***雷达跟随标志位话题发布者创建***/
	laser_follow_flag_pub = this->create_publisher<std_msgs::msg::Int8>("laser_follow_flag",10); 
	/***底盘运动信息话题发布者创建***/
	motion_msg_pub = this->create_publisher<wheeltec_mic_msg::msg::MotionControl>("motion_msg",10);
	/***导航点位置话题发布者创建***/
	pose_nav_pub = this->create_publisher<geometry_msgs::msg::PoseStamped>("goal_pose",10);
	/***导航点动作客户端创建***/
	goal_client =  rclcpp_action::create_client<ClientT>(this,"navigate_to_pose");
	/***语音反馈文本发布者创建***/
	feedback_words_pub = this->create_publisher<std_msgs::msg::String>("feedback_words",10);
	/***识别结果话题订阅者创建***/
	voice_words_sub = this->create_subscription<std_msgs::msg::String>(
		"voice_words",10,std::bind(&Command::voice_words_Callback,this,_1));
	/***寻找语音开启标志位话题订阅者创建***/
	voice_flag_sub = this->create_subscription<std_msgs::msg::Int8>(
		"voice_flag",10,std::bind(&Command::voice_flag_Callback,this,_1));

	if (if_akm) turn_line_vel_x = 0.2;
	else turn_line_vel_x = 0;

	sleep(3);
	std::cout<<"您可以语音控制啦!"<<std::endl;
	std::cout<<"小车前进———————————>向前"<<std::endl;
	std::cout<<"小车后退———————————>后退"<<std::endl;
	std::cout<<"小车左转———————————>左转"<<std::endl;
	std::cout<<"小车右转———————————>右转"<<std::endl;
	std::cout<<"小车停———————————>停止"<<std::endl;
	std::cout<<"小车休眠———————————>休眠，等待下一次唤醒"<<std::endl;
	std::cout<<"小车过来———————————>寻找声源"<<std::endl;
	std::cout<<"小车去I点———————————>小车自主导航至I点"<<std::endl;
	std::cout<<"小车去J点———————————>小车自主导航至J点"<<std::endl;
	std::cout<<"小车去K点———————————>小车自主导航至K点"<<std::endl;
	std::cout<<"小车雷达跟随———————————>小车打开雷达跟随"<<std::endl;
	std::cout<<"关闭雷达跟随———————————>小车关闭雷达跟随"<<std::endl;
}

void Command::run(){
	rclcpp::spin(shared_from_this());
}

Command::~Command(){
	RCLCPP_INFO(this->get_logger(),"command_recognition_node over!\n");
}

int main(int argc, char *argv[])
{
	rclcpp::init(argc,argv);
	auto node = std::make_shared<Command>("command_recognition",rclcpp::NodeOptions());
	rclcpp::spin(node);  
	rclcpp::shutdown();
	return 0;
}