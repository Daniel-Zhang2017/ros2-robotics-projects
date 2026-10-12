/****************************************************************/
/* Copyright (c) 2023 WHEELTEC Technology, Inc   				*/
/* function:Serial interface service							*/
/* 功能：串口交互服务												*/
/****************************************************************/
#include "wheeltec_m2n.h"
#include <fstream>
#include <chrono>

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
	set_beam_srv_ = this->create_service<wheeltec_mic_msg::srv::SetBeam>(
		"set_beam",[this](const std::shared_ptr<wheeltec_mic_msg::srv::SetBeam::Request> request,
                                std::shared_ptr<wheeltec_mic_msg::srv::SetBeam::Response> response){
		response->success = setBeamCallback(request,response);
		});
    get_audio_srv_ = this->create_service<wheeltec_mic_msg::srv::GetAudioData>(
        "get_audio_data",[this](const std::shared_ptr<wheeltec_mic_msg::srv::GetAudioData::Request> request,
            					std::shared_ptr<wheeltec_mic_msg::srv::GetAudioData::Response> response) {
        response->success = getAudioDataCallback(request, response);
        });
}

/********************************************************
Function: Message count
功能: 消息计数
*********************************************************/
unsigned short Wheeltec_Mic::getNewMessageId() {
    unsigned short id = server_msg_id_counter_.fetch_add(1);
    // 处理循环（0-65535）
    if (id >= 65535) {
        server_msg_id_counter_ = 0;
        id = 0;
    }
    return id;
}

/********************************************************
Function: Obtain device version information 
功能: 获取设备版本信息
*********************************************************/
bool Wheeltec_Mic::getDeviceTypeCallback(const std::shared_ptr<wheeltec_mic_msg::srv::GetDeviceType::Request>& request,
                                std::shared_ptr<wheeltec_mic_msg::srv::GetDeviceType::Response>& response){
	ServicePkg.type 	= "version";
	ServicePkg.sid 		= getNewMessageId();
	ServicePkg.msgType 	= ServiceType::DEVICE_VER;
	process_result = false;

	if(Send_Serial_Data(ServicePkg))
	{
		start_time = rclcpp::Node::now();
		while(!process_result)
		{
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
            // 检查串口数据
            if (!Get_Serial_Data()) {
                // 串口读取失败或无数据
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
            }
			last_time = rclcpp::Node::now();
			if ((last_time - start_time).seconds() > TIMEOUT)
			{
                response->result = "Timeout - No response from device";
                return false;
			}
		}
        if (!device_message.empty()) {
            response->result = device_message;
            return true;
        } else {
            response->result = "Empty response from device";
            return false;
        }
	}
	else
		return false;
}

/********************************************************
Function: Set wake word
功能: 设置唤醒词(更换完成后需要拔插设备)
*********************************************************/
bool Wheeltec_Mic::setAwakeWordCallback(const std::shared_ptr<wheeltec_mic_msg::srv::SetAwakeWord::Request>& request,
	                           std::shared_ptr<wheeltec_mic_msg::srv::SetAwakeWord::Response>& response){
	ServicePkg.type 		= "wakeup_keywords";
	ServicePkg.sid 			= getNewMessageId();
	ServicePkg.msgType 		= ServiceType::AWAKE_WORD;
	ServicePkg.awake_word 	= request->awake_word;
	ServicePkg.threshold 	= request->threshold;
	process_result = false;

	if(Send_Serial_Data(ServicePkg))
	{
		RCLCPP_WARN(this->get_logger(),"Device need to plug and unplug the device again!");
		return true;
	}
	else
		return false;
}

/********************************************************
Function: Manual wake-up (set beam)
功能: 手动唤醒（设置波束:0-5）
*********************************************************/
bool Wheeltec_Mic::setBeamCallback(const std::shared_ptr<wheeltec_mic_msg::srv::SetBeam::Request>& request,
								std::shared_ptr<wheeltec_mic_msg::srv::SetBeam::Response>& response){
	ServicePkg.type 	= "manual_wakeup";
	ServicePkg.sid 		= getNewMessageId();
	ServicePkg.msgType 	= ServiceType::SET_BEAM;
	ServicePkg.beam = request->beam_value;
	process_result = false;

	if(Send_Serial_Data(ServicePkg))
	{
		start_time = rclcpp::Node::now();
		while(!process_result)
		{
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
            // 检查串口数据
            if (!Get_Serial_Data()) {
                // 串口读取失败或无数据
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
            }
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
Function: Switch microphone type
功能: 切换麦克风类型(仅支持M2系列)
*********************************************************/
bool Wheeltec_Mic::switchMicCallback(const std::shared_ptr<wheeltec_mic_msg::srv::SwitchMic::Request>& request,
								std::shared_ptr<wheeltec_mic_msg::srv::SwitchMic::Response>& response){
	ServicePkg.type 	= "switch_mic";
	ServicePkg.sid 		= getNewMessageId();
	ServicePkg.msgType 	= ServiceType::SWITCH_MIC;
	ServicePkg.mic_type = request->mic_name;
	process_result = false;

	if(Send_Serial_Data(ServicePkg))
	{
		start_time = rclcpp::Node::now();
		while(!process_result)
		{
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
            // 检查串口数据
            if (!Get_Serial_Data()) {
                // 串口读取失败或无数据
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
            }
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
Function: Get audio data callback(仅支持M2N)
功能: 获取音频数据回调
*********************************************************/
bool Wheeltec_Mic::getAudioDataCallback(const std::shared_ptr<wheeltec_mic_msg::srv::GetAudioData::Request>& request,
                               std::shared_ptr<wheeltec_mic_msg::srv::GetAudioData::Response>& response)
{
    if (request->mode == 0) {
        // 停止音频模式
        bool stopped = stopAudioReceiving(true);  // 停止并自动保存
        
        response->success = stopped;
        response->message = stopped ? "音频已停止并保存" : "停止音频失败";
        response->file_size = audio_received_bytes_;
        
        return stopped;
    }
    else if (request->mode == 1) {
        // 开始音频模式
        unsigned int duration_sec = request->duration_sec;
        bool started = startAudioReceiving(duration_sec);
        
        response->success = started;
        response->message = started ? 
            "音频接收已启动，将在" + std::to_string(duration_sec) + "秒后自动停止" :
            "启动音频接收失败";
        response->file_size = 0;
        
        return started;
    }
    else {
        RCLCPP_WARN(this->get_logger(), "无效的模式参数: %d (0:停止, 1:开始)", request->mode);
        
        response->success = false;
        response->message = "无效的模式参数 (0:停止, 1:开始)";
        response->file_size = 0;
        
        return false;
    }
}

/********************************************************
Function: Handle audio data
功能: 处理音频数据
*********************************************************/
void Wheeltec_Mic::handleAudioData(const unsigned char* data, unsigned int len)
{
    bool should_stop = false;
    
    {
        std::lock_guard<std::mutex> lock(audio_mutex_);
        
        if (!is_audio_receiving_) {
            return;
        }
        
        if (audio_start_time_ == std::chrono::steady_clock::time_point()) {
            audio_start_time_ = std::chrono::steady_clock::now();
            RCLCPP_INFO(this->get_logger(), "设置音频接收开始时间");
        }
        
        // 添加到缓冲区
        audio_buffer_.insert(audio_buffer_.end(), data, data + len);
        audio_received_bytes_ += len;
        
        // 检查时长限制
        auto current_time = std::chrono::steady_clock::now();
        auto elapsed_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
            current_time - audio_start_time_).count();
        
        // 每1秒输出一次进度
        // static int last_log_sec = -1;
        // int current_sec = static_cast<int>(elapsed_sec);
        
        // if (current_sec != last_log_sec) {
        //     // 显示更精确的时间信息
        //     RCLCPP_INFO(this->get_logger(), 
        //                "音频时间: 已过=%.3f秒 (总时长=%.1f秒)",
        //                elapsed_sec, total_sec);
            
        //     if (current_sec >= 0) {
        //         float progress_percent = (elapsed_sec * 100.0) / total_sec;
        //         RCLCPP_INFO(this->get_logger(), 
        //                    "音频接收进度: %.1f/%.1f 秒 (%.1f%%), %.2f MB",
        //                    elapsed_sec, total_sec,
        //                    progress_percent,
        //                    audio_received_bytes_ / (1024.0 * 1024.0));
        //     }
        //     last_log_sec = current_sec;
        // }
        
        // 检查是否达到停止条件
        if (elapsed_ms >= audio_duration_) {
            should_stop = true;
        }
    }  // 锁在这里自动释放
    
    // 在锁外执行停止操作
    if (should_stop) {
        bool stop_result = stopAudioReceiving(true);
        RCLCPP_INFO(this->get_logger(), "音频接收完成，保存结果: %s",
                   stop_result ? "成功" : "失败");
    }
}

/********************************************************
Function: Send audio start command
功能: 发送音频开始命令
*********************************************************/
bool Wheeltec_Mic::sendAudioStartCommand(unsigned int duration_sec)
{
    ServicePacket audioPkg;
    audioPkg.type = "get_original_audio";
    audioPkg.sid = getNewMessageId();
    audioPkg.msgType = ServiceType::SET_AUDIO;
    audioPkg.mode = 1;
    
    // 保存ServicePkg用于后续的确认检查
    ServicePkg = audioPkg;
    
    RCLCPP_INFO(this->get_logger(), "发送音频开始命令 (ID: %d)", 
               audioPkg.sid);
    
    if (!Send_Serial_Data(audioPkg)) {
        RCLCPP_ERROR(this->get_logger(), "发送音频开始命令失败");
        return false;
    }
    
    auto start_wait = rclcpp::Node::now();
    
    while ((rclcpp::Node::now() - start_wait).seconds() < 3.0) {
        // 处理串口数据以接收确认消息
        Get_Serial_Data();
        
        // 检查是否有确认消息
        if (process_result && last_ack_id_ == ServicePkg.sid) {
            RCLCPP_INFO(this->get_logger(), 
                       "收到音频请求确认，开始计时 (ID: %d)", 
                       ServicePkg.sid);
            
            return true;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    
    RCLCPP_WARN(this->get_logger(), "等待设备确认超时 (ID: %d)", ServicePkg.sid);
    return false;
}

/********************************************************
Function: Send audio stop command
功能: 发送音频停止命令
*********************************************************/
bool Wheeltec_Mic::sendAudioStopCommand()
{
    ServicePacket stopPkg;
    stopPkg.type = "get_original_audio";
    stopPkg.sid = getNewMessageId();
    stopPkg.msgType = ServiceType::SET_AUDIO;
    stopPkg.mode = 0;
    
    RCLCPP_INFO(this->get_logger(), "发送音频停止命令 (ID: %d)", stopPkg.sid);
    
    if (!Send_Serial_Data(stopPkg)) {
        RCLCPP_ERROR(this->get_logger(), "发送音频停止命令失败");
        return false;
    }
    
    auto start_wait = rclcpp::Node::now();
    
    while ((rclcpp::Node::now() - start_wait).seconds() < 3.0) {
        // 处理串口数据以接收确认消息
        Get_Serial_Data();
        
        // 检查是否有确认消息
        if (process_result && last_ack_id_ == ServicePkg.sid) {
            RCLCPP_INFO(this->get_logger(), 
                       "收到停止音频请求确认，开始计时 (ID: %d)", 
                       ServicePkg.sid);
            
            return true;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    
    RCLCPP_WARN(this->get_logger(), "等待设备确认超时 (ID: %d)", ServicePkg.sid);
    return false;
}

/********************************************************
Function: Start audio receiving 
功能: 开始接收音频数据
*********************************************************/
bool Wheeltec_Mic::startAudioReceiving(unsigned int duration_sec)
{
    std::lock_guard<std::mutex> lock(audio_mutex_);
  
    // 启用虚拟串口
    enable_virtual_serial(true);
    if (!virtual_serial_initialized) {  // 如果启用失败，直接返回
        return false;
    }

    if (is_audio_receiving_) {
        RCLCPP_WARN(this->get_logger(), "音频接收已在进行中");
        return false;
    }
    
    // 参数验证（以秒为单位）
    if (duration_sec == 0) {
        duration_sec = 5;  // 默认5秒
        RCLCPP_INFO(this->get_logger(), "使用默认时长: %u 秒", duration_sec);
    } else if (duration_sec < 1) {
        RCLCPP_WARN(this->get_logger(), "时长过短 (%u 秒)，使用最小值1秒", duration_sec);
        duration_sec = 1;
    } else if (duration_sec > 300) {  // 最长5分钟
        RCLCPP_WARN(this->get_logger(), "时长过长 (%u 秒)，使用最大值300秒", duration_sec);
        duration_sec = 300;
    }
    // 转换为毫秒用于内部计时
    audio_duration_ = duration_sec * 1000;  // 秒转毫秒
    
    // 重置缓冲区
    audio_buffer_.clear();
    audio_received_bytes_ = 0;
    
    // 发送开始命令
    bool command_sent = sendAudioStartCommand(duration_sec);
    
    if (command_sent) {
        is_audio_receiving_ = true;
        audio_start_time_ = std::chrono::steady_clock::time_point();
        RCLCPP_INFO(this->get_logger(), "音频接收已启动，将在 %u 秒后自动停止", duration_sec);
        return true;
    }
    
    return false;
}

/********************************************************
Function: Stop audio receiving
功能: 停止接收音频数据
*********************************************************/
bool Wheeltec_Mic::stopAudioReceiving(bool auto_save)
{
    bool save_success = true;
    bool command_sent = true;
    {
        std::lock_guard<std::mutex> lock(audio_mutex_);
        
        if (!is_audio_receiving_) {
            RCLCPP_WARN(this->get_logger(), "没有正在进行的音频接收");
            enable_virtual_serial(false);
            return true;
        }
        
        // 发送停止命令
        command_sent = sendAudioStopCommand();
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
        
        // 停止本地接收
        is_audio_receiving_ = false;
        audio_start_time_ = std::chrono::steady_clock::time_point();
        
        // 自动保存
        if (auto_save) {
            save_success = saveCurrentAudio();
        }
    }
    // 停止虚拟串口
    enable_virtual_serial(false);
    return save_success && command_sent;
}

/********************************************************
Function: Save current audio to file
功能: 保存当前音频到文件
*********************************************************/
bool Wheeltec_Mic::saveCurrentAudio(const std::string& filename)
{
    if (audio_buffer_.empty()) {
        RCLCPP_WARN(this->get_logger(), "音频缓冲区为空");
        return false;
    }
    
    // 计算录制时长（秒）
    auto end_time = std::chrono::steady_clock::now();
    auto duration_ms = 0;
    if (audio_start_time_ != std::chrono::steady_clock::time_point()) {
        duration_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
            end_time - audio_start_time_).count();
    }
    
    // 生成文件名
    std::string file_path = filename;
    if (file_path.empty()) {
        // 获取当前系统时间
        auto now = std::chrono::system_clock::now();
        auto now_time_t = std::chrono::system_clock::to_time_t(now);
        std::tm time_info;
        localtime_r(&now_time_t, &time_info);
        
        // 格式化时间：YYYYMMDD_HHMMSS
        char time_buffer[32];
        std::strftime(time_buffer, sizeof(time_buffer), "%Y%m%d_%H%M%S", &time_info);
        
        // 创建音频目录
        file_path = audio_path + "origin_" + std::string(time_buffer) + ".pcm";
    }
    
    // 保存文件
    try {
        std::ofstream file(file_path, std::ios::binary);
        if (!file.is_open()) {
            RCLCPP_ERROR(this->get_logger(), "无法打开文件: %s", file_path.c_str());
            return false;
        }
        
        file.write(reinterpret_cast<const char*>(audio_buffer_.data()), 
                  audio_buffer_.size());
        file.close();
        
        RCLCPP_INFO(this->get_logger(), "音频保存成功: %s", file_path.c_str());
        
        // 清空缓冲区
        audio_buffer_.clear();
        audio_received_bytes_ = 0;
        
        return true;
    } 
    catch (const std::exception& e) {
        RCLCPP_ERROR(this->get_logger(), "保存音频失败: %s", e.what());
        return false;
    }
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
		case ServiceType::SET_BEAM:
			type_describe["type"]= pkg.type;
			type_describe["content"]["beam"] = pkg.beam;
			break;
        case ServiceType::SET_AUDIO:  
        {
            type_describe["type"]= pkg.type;
            type_describe["content"]["audio"] = pkg.mode;
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
