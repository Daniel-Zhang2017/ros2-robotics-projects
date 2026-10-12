/************************************************************************************************/
/* Copyright (c) 2025 WHEELTEC Technology, Inc   												*/
/* function:Motion controller, Process multiple information sources and output cmd_vel			*/
/* 功能：运动控制器，对多个信息源进行处理并输出cmd_vel													*/
/************************************************************************************************/
#include "motion_control.h"

/**************************************************************************
函数功能：发送导航目标点
返回  值：无
**************************************************************************/
void Motion::send_goal(std::vector<float> msg)
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
	goal.pose.pose.position.z = msg.at(2);
	goal.pose.pose.orientation.x = msg.at(3);
	goal.pose.pose.orientation.y = msg.at(4);
	goal.pose.pose.orientation.z = msg.at(5);
	goal.pose.pose.orientation.w = msg.at(6);

	RCLCPP_INFO(this->get_logger(), "Sending goal");

	auto send_goal_options = rclcpp_action::Client<ClientT>::SendGoalOptions();
	send_goal_options.goal_response_callback = 
		std::bind(&Motion::goal_response_callback,this,_1);
	send_goal_options.feedback_callback = 
		std::bind(&Motion::feedback_callback,this,_1,_2);
	send_goal_options.result_callback = 
		std::bind(&Motion::result_callback,this,_1);

	goal_client->async_send_goal(goal,send_goal_options);
}
/**************************************************************************
函数功能：动作目标响应回调函数
返回  值：无
**************************************************************************/
void Motion::goal_response_callback(const std::shared_ptr<GoalHandle_> future)
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
void Motion::feedback_callback(GoalHandle_::SharedPtr,const std::shared_ptr<const ClientT::Feedback> feedback)
{
	// RCLCPP_INFO(this->get_logger(),"Remaining Distance from Destination: %f",feedback->distance_remaining);
}

/**************************************************************************
函数功能：动作结果回调函数
返回  值：无
**************************************************************************/
void Motion::result_callback(const GoalHandle_::WrappedResult &result)
{
	switch(result.code)
	{
		case rclcpp_action::ResultCode::SUCCEEDED:
			goal_reached_flag = 1;
			is_following_goal = false;
			break;
		case rclcpp_action::ResultCode::ABORTED:
			RCLCPP_ERROR(this->get_logger(), "Goal was aborted");
			goal_status = 1;
			is_following_goal = false;
			return;
		case rclcpp_action::ResultCode::CANCELED:
			goal_status = 1;
			is_following_goal = false;
			RCLCPP_ERROR(this->get_logger(), "Goal was canceled");
			return;
		default:
			goal_status = 1;
			is_following_goal = false;
        	RCLCPP_ERROR(this->get_logger(), "Unknown result code");
        	return;
	}
	RCLCPP_INFO(this->get_logger(), "Result received");
}

/********************************************************
Function:Gets the current wake Angle
功能: 获取当前唤醒角度 
*********************************************************/
void Motion::angle_Callback(const std_msgs::msg::UInt32::SharedPtr msg)
{
	angle = msg->data;
}

/********************************************************
Function:Lidar obstacle azimuth acquisition
功能: 雷达障碍物方位获取 
*********************************************************/
void Motion::current_position_Callback(const turn_on_wheeltec_robot::msg::Position::SharedPtr msg)
{
	distance = msg->distance;
	direction_obstacle_x = msg->angle_x;

	if(direction_obstacle_x>0)
		direction_obstacle_x=direction_obstacle_x-3.1415;
	else
		direction_obstacle_x=direction_obstacle_x+3.1415;
}

/********************************************************
Function:Car coordinate pose acquisition
功能: 小车坐标位姿获取 
*********************************************************/
void Motion::pose_Callback(const nav_msgs::msg::Odometry::SharedPtr msg)
{
	double orientation_z;
	double orientation_w;
	orientation_z = msg->pose.pose.orientation.z;
	orientation_w = msg->pose.pose.orientation.w;

	double direction_r;
	double siny_cosp = 2*(orientation_w*orientation_z);
	double cosy_cosp = 1 - 2*siny_cosp; 
	direction_r = atan2(siny_cosp,cosy_cosp);

	direction = direction_r*180 / 3.1415926;
	if (direction < 0) direction += 360;
}

/********************************************************
Function:Acquire target vel
功能: 获取目标速度
*********************************************************/
void Motion::motion_msg_Callback(const wheeltec_mic_msg::msg::MotionControl::SharedPtr msg)
{
	cmd_vel_msg.linear.x = msg->linear_x;
	cmd_vel_msg.angular.z = msg->angular_z;
	follow_flag = msg->follow_flag;
	cmd_vel_flag = msg->cmd_vel_flag;
}

/**********************************************************
Function:Look for the sound source control steering section
功能: 寻找声源控制转向部分
***********************************************************/
void Motion::follow_turn(int angle)
{
	int ticks;
	float angle_turn_msg = 0.5;		//转向速度(rad)
	float angle_duration;			//转向时间
	float rate = 50;
	rclcpp::Rate loop_rate(rate);	//控制频率50Hz

	cmd_vel_msg.linear.x = 0;

	/***控制转向(以最小角度转向)***/
	if (angle <= 180) cmd_vel_msg.angular.z = angle_turn_msg;
	else{
		angle = 360 - angle;
		cmd_vel_msg.angular.z = -angle_turn_msg;
	}

	angle_duration = angle / angle_turn_msg /180 * 3.14159;
	ticks = int(angle_duration * rate);

	for (int i = 0; i < ticks; i++)
	{
		cmd_vel_pub->publish(cmd_vel_msg);
		loop_rate.sleep();
		// std::cout << i <<std::endl;
	}
	cmd_vel_msg.angular.z = 0;
	cmd_vel_pub->publish(cmd_vel_msg); 
}

/********************************************************************
Function:Look for the sound source control steering section(akm model)
功能: 寻找声源控制转向部分(akm车型)
*********************************************************************/
void Motion::akm_follow_turn(int angle)
{
	float angle_r;
	float goal_angle_r;
	angle_r = (360-angle)*3.14159/180;

	geometry_msgs::msg::PoseStamped map_pose;
	geometry_msgs::msg::PointStamped robot_point;
	geometry_msgs::msg::PointStamped map_point;
	robot_point.header.frame_id = "base_footprint";
	robot_point.header.stamp = this->now();
	robot_point.point.x = cos(angle_r);
	robot_point.point.y = -sin(angle_r);
	robot_point.point.z = 0;
	
	try
	{
		tf_buffer_->lookupTransform("map","base_footprint", this->now(),100ms);
		map_point = tf_buffer_->transform(robot_point,"map");
		RCLCPP_INFO(
        this->get_logger(), "Point of akm in frame of map: x:%f y:%f z:%f",
        map_point.point.x,
        map_point.point.y,
        map_point.point.z);
	}
	catch(const tf2::TransformException & ex)
	{
		RCLCPP_WARN(this->get_logger(), "Failure: %s", ex.what());
		return;
	}
	
	goal_angle_r = (direction+angle-360)/180*3.1415926;
	if (!point.empty()) point.clear();
	point.push_back(map_point.point.x);
	point.push_back(map_point.point.y);
	point.push_back(map_point.point.z);
	point.push_back(0);
	point.push_back(0);
	point.push_back(sin(goal_angle_r/2));
	point.push_back(cos(goal_angle_r/2));
	
	is_following_goal = true;
	goal_sent = false;
	goal_reached_flag = 0;
}


/********************************************************
Function:Obstacle analysis and judgment
功能: 障碍物判断(小于设定距离或者在运动趋势上)
*********************************************************/
int Motion::motion_judgement()
{
	int radius = 0;
	// std::cout << " distance = " << distance <<std::endl;
	if (distance <= lidar_range) radius = 1;
	if (cmd_vel_msg.linear.x > 0 && (direction_obstacle_x  > 1.57 || direction_obstacle_x < -1.57) && radius)
		return 1;
	else if (cmd_vel_msg.linear.x < 0 && (direction_obstacle_x > -1.57 && direction_obstacle_x < 1.57) && radius)
		return 1;
	else if (cmd_vel_msg.angular.z > 0 && direction_obstacle_x < -1.57)
		return 1;
	else if (cmd_vel_msg.angular.z < 0 && direction_obstacle_x > 1.57)
		return 1;
	else
		return 0;
}

/********************************************************
Function:chassis motion control
功能: 底盘运动控制
*********************************************************/
void Motion::timer_callback()
{
    static int temp_cout = 0;
    
    // 处理导航目标发送和状态检查
    if (is_following_goal && !goal_sent){
        send_goal(point);
        goal_sent = true;
        RCLCPP_INFO(this->get_logger(), "Navigation goal sent");
    }

    if (follow_flag){
        if (if_akm) {
            akm_follow_turn(angle);
        }
        else {
            follow_turn(angle);
            follow_flag = 0;
            turn_fin_flag = 1;
        }
        
        if (!goal_status && !if_akm){
            cmd_vel_msg.linear.x = 0.15;
            cmd_vel_msg.angular.z = 0;
            goal_status = 0;
        }
    }
    
    // akm模型的导航完成检查
    if (is_following_goal && goal_reached_flag){
        is_following_goal = false;
        follow_flag = 0;
        turn_fin_flag = 1;

        if (!goal_status){
            cmd_vel_msg.linear.x = 0.15;
            cmd_vel_msg.angular.z = 0;
            goal_status = 0;
        }
        
        goal_reached_flag = 0;
        RCLCPP_INFO(this->get_logger(), "Navigation completed, continuing with follow behavior");
    }

    // 处理速度发布和障碍检测
    if (cmd_vel_flag || turn_fin_flag){
        cmd_vel_pub->publish(cmd_vel_msg);
        if (motion_judgement()){
            temp_cout++;
            if (temp_cout >= lidar_count) {
                temp_cout = 0;
                cmd_vel_flag = 0;
                cmd_vel_msg.linear.x = 0;
                cmd_vel_msg.angular.z = 0;
                cmd_vel_pub->publish(cmd_vel_msg);

                if (!turn_fin_flag){
                    std_msgs::msg::String str_msg;
                    str_msg.data = "遇到障碍物";
                    voice_words_pub->publish(str_msg);
                }
                else{
                    turn_fin_flag = 0;
                    feedback_text.data = "已找到声源";
                    feedback_words_pub->publish(feedback_text);
                    std::cout<<"已找到声源"<<std::endl;
                }
            }
        }
        else{
            // 如果没有检测到障碍物，重置计数器
            temp_cout = 0;
        }
    }
}

void Motion::control()
{
	rclcpp::spin(shared_from_this());
}	

Motion::Motion(const std::string &node_name):
rclcpp::Node(node_name)
{
	this->declare_parameter<float>("lidar_range",0.75);
	this->declare_parameter<int>("lidar_count",3);
	this->declare_parameter<std::string>("audio_path","");
	this->declare_parameter<bool>("if_akm_yes_or_no",false);
	this->get_parameter("lidar_range",lidar_range);
	this->get_parameter("lidar_count",lidar_count);
	this->get_parameter("audio_path",audio_path);
	this->get_parameter("if_akm_yes_or_no",if_akm);

	/***创建离线命令词识别结果话题发布者***/
	voice_words_pub = this->create_publisher<std_msgs::msg::String>("voice_words",10);
	/***速度话题发布者创建***/
	cmd_vel_pub = this->create_publisher<geometry_msgs::msg::Twist>("cmd_vel",10);
	/***语音反馈文本发布者创建***/
	feedback_words_pub = this->create_publisher<std_msgs::msg::String>("feedback_words",10);
	/***导航点动作客户端创建***/
	goal_client =  rclcpp_action::create_client<ClientT>(this,"navigate_to_pose");
	/***创建当前唤醒角度话题订阅者***/
	angle_sub = this->create_subscription<std_msgs::msg::UInt32>(
		"awake_angle",10,std::bind(&Motion::angle_Callback,this,_1));
	/***创建底盘运动话题订阅者***/
	motion_msg_sub = this->create_subscription<wheeltec_mic_msg::msg::MotionControl>(
		"motion_msg",10,std::bind(&Motion::motion_msg_Callback,this,_1));
	/***创建odom_combined话题订阅者***/
	pose_sub = this->create_subscription<nav_msgs::msg::Odometry>(
		"odom_combined",10,std::bind(&Motion::pose_Callback,this,_1));
	/***创建障碍物方位话题订阅者***/
	current_position_sub = this->create_subscription<turn_on_wheeltec_robot::msg::Position>(
		"object_tracker/current_position",10,std::bind(&Motion::current_position_Callback,this,_1));

	tf_buffer_ = std::make_unique<tf2_ros::Buffer>(this->get_clock());
	tf_listener_ = std::make_shared<tf2_ros::TransformListener>(*tf_buffer_);

	timer_ = this->create_wall_timer(
		std::chrono::milliseconds(100),
		std::bind(&Motion::timer_callback, this));
}

Motion::~Motion(){
	RCLCPP_INFO(this->get_logger(),"motion_control_node over!\n");
}

int main(int argc, char const *argv[])
{
	rclcpp::init(argc,argv);
	auto node = std::make_shared<Motion>("motion_control");
	rclcpp::spin(node); 
	rclcpp::shutdown();
	return 0;

}