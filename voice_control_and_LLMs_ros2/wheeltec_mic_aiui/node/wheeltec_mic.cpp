/****************************************************************/
/* Copyright (c) 2025 WHEELTEC Technology, Inc   				*/
/* function:Serial port analysis							    */
/* 功能：串口解析										            */
/****************************************************************/
#include "wheeltec_mic.h"

/**************************************
Function: Get the serial port return field
功能: 获取串口返回字段
***************************************/
bool Wheeltec_Mic::UnPackMsgPacket(const std::string &content, MsgPacket &data)
{
	if (content.size() < 7 || ((unsigned char)content.at(0) != FRAME_HEADER))
		return false;

	data.uid  = content[1] & 0xff;
	data.type = content[2] & 0xff;
	data.size = (content[3] & 0xff) | ((content[4] << 8 & 0xff00));
	data.sid  = (content[5] & 0xff) | ((content[6] << 8 & 0xff00));

	switch((MsgType)data.type)
	{
		case MsgType::AIUI_MSG:
		{
			std::string info = content.substr(7,data.size);
			data.bytes  = info;
			return true;
		}
			break;
		case MsgType::CONTROL:
		{
			std::string info = content.substr(7,data.size);
			data.bytes  = info;
			return true;
		}
			break;
		default:
        	break;
	}
    return false;
}

/**************************************
Function: Verify serial port data and parse information
功能: 校验串口数据并解析信息
***************************************/
int Wheeltec_Mic::process_data(const unsigned char *buf, int len)
{
	if (buf[2] == 0xff)
	{
		return -1;
	}

	//校验码校对
	int sum = std::accumulate(buf, buf + len - 1, 0);
	if (((~sum + 1) & 0xff) != buf[len - 1]) 
    {
      // std::cout<<"recv data not crc, drop\n"<<std::endl;
      return -1;
    }

    if (UnPackMsgPacket(std::string((char *)buf,len),MsgPkg))
    {
    	Json::Reader reader;
    	if ((MsgType)MsgPkg.type == MsgType::AIUI_MSG)
    	{
    		Json::Value Aiui_Msg;
    		Json::Value value_iwv;

    		if (reader.parse(MsgPkg.bytes, Aiui_Msg)){
    			//串口反馈信息显示
				//std::cout<< "Aiui_Msg: "<<  Aiui_Msg <<std::endl;
    			if (Aiui_Msg["type"].asString() == "aiui_event")
    			{
	    			std_msgs::msg::Int8 awake_msg;
	    			awake_msg.data = 1;
	    			awake_flag_pub->publish(awake_msg);
		    		Json::Value content = Aiui_Msg["content"];
						if (content["eventType"].asString() == "4")
							{
								std::string iwv_msg = content["info"].asString();
								if (reader.parse(iwv_msg,value_iwv))
				    		{
				    			angle = value_iwv["ivw"]["angle"].asInt();
				    			std_msgs::msg::UInt32 angle_msg;
				    			angle_msg.data = angle;
				    			angle_pub->publish(angle_msg);
				    			std::cout << ">>>>>唤醒角度为: " << angle << "°"<< std::endl;
				    		}
				    	else
								std::cout << "reader json fail!"<< std::endl;
								}    				
    			}
    			else
    			{
    				device_message = Aiui_Msg["content"].asString();
    				process_result = true;
    			}
    		}
    	return 1;
    	}
    }
    return -1;
}

/**************************************
Function: Receive and filter data
功能: 过滤数据
***************************************/
int Wheeltec_Mic::uart_analyse(unsigned char buffer)
{
	static int count=0, frame_len=0;
	Receive_Data[count] = buffer;
    if(Receive_Data[0] != FRAME_HEADER || (count == 1 && Receive_Data[1] != USER_ID))  
      count = 0,frame_len = 0;
    else 
      count++;
	if (count == 7){  
      frame_len = (Receive_Data[4]<<8 | Receive_Data[3]) + 7 + 1;
      if (frame_len > 1024) {
          //RCLCPP_ERROR(this->get_logger(), "Frame length exceeds buffer size");
          count = frame_len = 0;
          memset(Receive_Data, 0, sizeof(Receive_Data));
          return 0;
      }
	}
	if(count == frame_len && frame_len > 0){
		int ret = process_data(Receive_Data,frame_len);
		count = 0,frame_len = 0;
		memset(Receive_Data, 0, 1024);
		return ret;
	}
	return 0;
}

/**************************************
Function: Receive the information sent by the device
功能: 接收下位机发送的信息
***************************************/
bool Wheeltec_Mic::Get_Serial_Data()
{
	if (!serial_initialized) return false;

	try {
		size_t available = MicArr_Serial.available();
  		if (available == 0) return false;
		const size_t max_read_size = 128;
		size_t to_read = std::min(available, max_read_size);
		unsigned char buffer[max_read_size];
		size_t bytes_read = MicArr_Serial.read(buffer, to_read);
		bool result = false;

		for (size_t i = 0; i < bytes_read; i++) {
			if (uart_analyse(buffer[i])) {
				result = true;
			}
		}
		return result;
	} catch (const serial::IOException& e) {
		RCLCPP_ERROR(this->get_logger(), "Serial read error: %s", e.what());
		handle_serial_error();
	}
	return false;
}

/**************************************
Function: Handle serial port errors
功能: 处理串口异常
***************************************/
void Wheeltec_Mic::handle_serial_error() 
{
    serial_initialized = false;
    MicArr_Serial.close();
    
    RCLCPP_INFO(this->get_logger(), "Attempting to reconnect...");
    for (int i = 0; i < 3; ++i) {
        try {
            MicArr_Serial.open();
            if (MicArr_Serial.isOpen()) {
                MicArr_Serial.flush();
                serial_initialized = true;
                RCLCPP_INFO(this->get_logger(), "Reconnected successfully");
                return;
            }
        } catch (...) {
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
    }
    RCLCPP_ERROR(this->get_logger(), "Failed to reconnect to serial port");
}

/**************************************
Function: Serial port data processing callback
功能: 串口数据处理回调函数
***************************************/
void Wheeltec_Mic::serial_read_callback()
{
    if (!serial_initialized) {
        RCLCPP_ERROR(this->get_logger(), "Serial port not initialized");
        return;
    } 
    Get_Serial_Data();
}

/**************************************
Function: Loop access to the lower computer data and issue topics
功能: 循环获取下位机数据与发布话题
***************************************/
void Wheeltec_Mic::run()
{
    if (!serial_initialized) {
        RCLCPP_ERROR(this->get_logger(), "Serial port initialization failed");
        return;
    }

    timer_ = this->create_wall_timer(
        std::chrono::milliseconds(50),
        std::bind(&Wheeltec_Mic::serial_read_callback, this));
    
    rclcpp::spin(shared_from_this());
}

/**************************************
Function: Initialize serial port with retry
功能: 串口初始化
***************************************/
void Wheeltec_Mic::initialize_serial() {
    const int max_retries = 3;
    serial::Timeout timeout = serial::Timeout::simpleTimeout(1000); 
    for (int retry = 0; retry < max_retries; ++retry) {
        try {
            if (MicArr_Serial.isOpen()) MicArr_Serial.close();
            
            MicArr_Serial.setPort(usart_port_name);
            MicArr_Serial.setBaudrate(serial_baud_rate);
            MicArr_Serial.setTimeout(timeout);
            MicArr_Serial.open();
            
            if (MicArr_Serial.isOpen()) {
                MicArr_Serial.flush();
                serial_initialized = true;
                RCLCPP_INFO(get_logger(), "Serial port initialized successfully");
                
                sleep(1.0);
                std_msgs::msg::Int8 flag_msg;
                flag_msg.data = 1;
                voice_flag_pub->publish(flag_msg);
								std::cout << ">>>>>成功打开麦克风设备" << std::endl;
								std::cout << ">>>>>以降噪板设置的唤醒词为准[默认:小微小微] " << std::endl;
                return;
            }
        } catch (const std::exception& e) {
            //RCLCPP_ERROR(get_logger(), "Serial init attempt %d failed: %s", retry+1, e.what());
        }
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
    RCLCPP_FATAL(get_logger(), "Failed to initialize serial port after %d attempts", max_retries);
    RCLCPP_ERROR(this->get_logger(),"wheeltec_mic can not open serial port,Please check the serial port cable! ");
}

/**************************************
Function: Constructor, executed only once, for initialization
功能: 构造函数, 用于初始化
***************************************/
Wheeltec_Mic::Wheeltec_Mic(const std::string &node_name)
:rclcpp::Node(node_name),serial_initialized(false){
	memset(&Receive_Data, 0, sizeof(Receive_Data));

	setupMicArrayServices();

	this->declare_parameter<std::string>("usart_port_name","/dev/ttyCH343USB0");
	this->declare_parameter<int>("serial_baud_rate",115200);

	this->get_parameter("usart_port_name",usart_port_name);
	this->get_parameter("serial_baud_rate",serial_baud_rate);

	/***唤醒标志位话题发布者创建***/
	awake_flag_pub = this->create_publisher<std_msgs::msg::Int8>("awake_flag",10);
	/***麦克风设备串口打开标志位话题发布者创建***/
	voice_flag_pub = this->create_publisher<std_msgs::msg::Int8>("voice_flag",10);
	/***唤醒角度话题发布者创建***/
	angle_pub = this->create_publisher<std_msgs::msg::UInt32>("awake_angle",10);

	initialize_serial();

}

Wheeltec_Mic::~Wheeltec_Mic()
{
	RCLCPP_INFO(this->get_logger(),"wheeltec_mic_node over!\n");

	if (timer_) {
		timer_->cancel();
	}

	if (MicArr_Serial.isOpen()) {
		MicArr_Serial.close();
	}
}

int main(int argc,char **argv)
{
	rclcpp::init(argc,argv);
	auto mic = std::make_shared<Wheeltec_Mic>("wheeltec_mic");
  	mic->run();
  	rclcpp::shutdown();
	return 0;
}