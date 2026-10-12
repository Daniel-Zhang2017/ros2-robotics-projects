/****************************************************************/
/* Copyright (c) 2025 WHEELTEC Technology, Inc   				*/
/* function:Serial interface service							*/
/* 功能：串口交互服务												*/
/****************************************************************/
#include "wheeltec_mic.h"

/********************************************************
Function: Initialize service
功能: 初始化服务
*********************************************************/
void Wheeltec_Mic::setupMicArrayServices()
{
	get_device_srv_ = this->create_service<wheeltec_mic_msg::srv::GetDeviceType>(
		"get_device_info",[this](const std::shared_ptr<wheeltec_mic_msg::srv::GetDeviceType::Request> request,
                                std::shared_ptr<wheeltec_mic_msg::srv::GetDeviceType::Response> response){
		response->success = getDeviceTypeCallback(request,response);
		});
	set_awake_word_srv_ = this->create_service<wheeltec_mic_msg::srv::SetAwakeWord>(
		"set_awake_word",[this](const std::shared_ptr<wheeltec_mic_msg::srv::SetAwakeWord::Request> request,
                                std::shared_ptr<wheeltec_mic_msg::srv::SetAwakeWord::Response> response){
		response->success = setAwakeWordCallback(request,response);
		if(response->success) exit(1);
		});
	switch_mic_srv_ = this->create_service<wheeltec_mic_msg::srv::SwitchMic>(
		"switch_mic_type",[this](const std::shared_ptr<wheeltec_mic_msg::srv::SwitchMic::Request> request,
                                std::shared_ptr<wheeltec_mic_msg::srv::SwitchMic::Response> response){
		response->success = switchMicCallback(request,response);
		});	
}

/********************************************************
Function: Obtain device version information 
功能: 获取设备版本信息
*********************************************************/
bool Wheeltec_Mic::getDeviceTypeCallback(const std::shared_ptr<wheeltec_mic_msg::srv::GetDeviceType::Request>& request,
                                std::shared_ptr<wheeltec_mic_msg::srv::GetDeviceType::Response>& response){
	ServicePkg.type 	= "version";
	ServicePkg.sid 		= Receive_Data[5] | Receive_Data[6]<<8;
	ServicePkg.msgType 	= ServiceType::DEVICE_VER;
	process_result = false;

	if(Send_Serial_Data(ServicePkg))
	{
		start_time = rclcpp::Node::now();
		while(!process_result)
		{
			Get_Serial_Data();
			last_time = rclcpp::Node::now();
			if ((last_time - start_time).seconds() > TIMEOUT)
			{
				RCLCPP_ERROR(this->get_logger(),"Service response time exceeded!");
				return false;
			}
		}
		response->result = device_message;
		return true;
	}
	else
		return false;
}

/********************************************************
Function: Set wake word
功能: 设置唤醒词(更换完成后需要等待设备重启)
*********************************************************/
bool Wheeltec_Mic::setAwakeWordCallback(const std::shared_ptr<wheeltec_mic_msg::srv::SetAwakeWord::Request>& request,
	                           std::shared_ptr<wheeltec_mic_msg::srv::SetAwakeWord::Response>& response){
	ServicePkg.type 		= "wakeup_keywords";
	ServicePkg.sid 			= Receive_Data[5] | Receive_Data[6]<<8;
	ServicePkg.msgType 		= ServiceType::AWAKE_WORD;
	ServicePkg.awake_word 	= request->awake_word;
	ServicePkg.threshold 	= request->threshold;
	process_result = false;

	if(Send_Serial_Data(ServicePkg))
	{
		RCLCPP_WARN(this->get_logger(),"Device is restarting,please wait a moment to run!");
		return true;
	}
	else
		return false;
}

/********************************************************
Function: Switch microphone type
功能: 切换麦克风类型
*********************************************************/
bool Wheeltec_Mic::switchMicCallback(const std::shared_ptr<wheeltec_mic_msg::srv::SwitchMic::Request>& request,
								std::shared_ptr<wheeltec_mic_msg::srv::SwitchMic::Response>& response){
	ServicePkg.type 	= "switch_mic";
	ServicePkg.sid 		= Receive_Data[5] | Receive_Data[6]<<8;
	ServicePkg.msgType 	= ServiceType::SWITCH_MIC;
	ServicePkg.mic_type = request->mic_name;
	process_result = false;

	if(Send_Serial_Data(ServicePkg))
	{
		start_time = rclcpp::Node::now();
		while(!process_result)
		{
			Get_Serial_Data();
			last_time = rclcpp::Node::now();
			if ((last_time - start_time).seconds() > TIMEOUT)
			{
				RCLCPP_ERROR(this->get_logger(),"Service response time exceeded!");
				return false;
			}
		}
		response->result = device_message;
		return true;
	}
	else
		return false;
}

/********************************************************
Function: Send data packet
功能: 下发数据包
*********************************************************/
bool Wheeltec_Mic::Send_Serial_Data(ServicePacket &pkg)
{
	std::string section;
	std::string Master_Message;
	Json::Value type_describe;

	switch(pkg.msgType)
	{
		case ServiceType::DEVICE_VER:
			type_describe["type"]= pkg.type;
			break;
		case ServiceType::AWAKE_WORD:
		{
			type_describe["type"]= pkg.type;
			type_describe["content"]["keyword"] = pkg.awake_word;
			type_describe["content"]["threshold"] = pkg.threshold;
		}
			break;
		case ServiceType::SWITCH_MIC:
		{
			type_describe["type"]= pkg.type;
			type_describe["content"]["mic"] = pkg.mic_type;
		}
			break;
		default:
			break;
	}
	section = type_describe.toStyledString();
	//std::cout<< " section = "<< section <<std::endl;
	Master_Message = MakeMsgPacket(pkg.sid,MsgType::CONTROL,section);
	try
  	{
    	MicArr_Serial.write(Master_Message);
	}
  	catch (serial::IOException& e)   
	{
	    RCLCPP_ERROR(this->get_logger(),("Unable to send data through serial port")); 
	    return false;
	}
	return true;
}

/********************************************************
Function: Make a packet
功能: 制作数据包
*********************************************************/
std::string Wheeltec_Mic::MakeMsgPacket(unsigned short sid, MsgType type, const std::string &content)
{
	const unsigned short size = content.size();

	std::string data;

	data += (char)FRAME_HEADER;         	/* head     */
    data += (char)USER_ID;         			/* uid      */
    data += (char)type;                 	/* type     */
    data += (char)(size & 0xff);        	/* len_low  */
    data += (char)((size >> 8) & 0xff); 	/* len_high */
    data += (char)(sid & 0xff);         	/* sid_low  */
    data += (char)((sid >> 8) & 0xff);  	/* sid_high */

    data += content;

    int sum = std::accumulate(data.cbegin(),data.cend(),0);

    data += (char)((~sum +1) & 0xff);

    return data;
}
