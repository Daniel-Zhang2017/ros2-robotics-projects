/****************************************************************/
/* Copyright (c) 2025 WHEELTEC Technology, Inc   				*/
/* function:Serial port analysis							    */
/* 功能：串口解析										            */
/****************************************************************/
#include "wheeltec_m2n.h"

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
    
    data.size = (content[3] & 0xff) | ((content[4] & 0xff) << 8); 
    data.sid  = (content[5] & 0xff) | ((content[6] & 0xff) << 8);  

    // 验证长度
    if (content.size() < 7 + data.size + 1) {
        RCLCPP_WARN(this->get_logger(), "消息长度不足，需要: %d, 实际: %zu", 
                   7 + data.size + 1, content.size());
        return false;
    }

    std::string info = content.substr(7, data.size);
    data.bytes = info;
    
    // 验证校验码
    unsigned char received_checksum = content[7 + data.size];
    unsigned char calculated_checksum = 0;
    for (int i = 0; i < 7 + data.size; i++) {
        calculated_checksum += content[i];
    }
    calculated_checksum = ((~calculated_checksum) + 1) & 0xFF;
    
    if (received_checksum != calculated_checksum) {
        RCLCPP_WARN(this->get_logger(), "校验码错误: 期望0x%02X, 实际0x%02X", 
                   calculated_checksum, received_checksum);
        return false;
    }
    
    return true;
}

/**************************************
Function: Verify serial port data and parse information
功能: 校验串口数据并解析信息
***************************************/
int Wheeltec_Mic::process_data(const unsigned char *buf, int len)
{
    if (len < 8) {
        RCLCPP_WARN(this->get_logger(), "数据长度不足: %d", len);
        return -1;
    }
    unsigned char msg_type = buf[2];
    // 计算消息长度（小端序）
    unsigned short reported_len = (buf[3] & 0xff) | ((buf[4] & 0xff) << 8);
    unsigned short total_len_needed = 7 + reported_len + 1;
    
    if (len != total_len_needed) {
        RCLCPP_WARN(this->get_logger(), "长度不匹配: 报告=%d, 实际=%d, 需要=%d", 
                   reported_len, len, total_len_needed);
        return -1;
    }

    // 验证校验码（所有消息类型都需要验证）
    int sum = std::accumulate(buf, buf + len - 1, 0);
    unsigned char calculated_checksum = ((~sum) + 1) & 0xff;
    
    if (calculated_checksum != buf[len - 1]) {
        RCLCPP_WARN(this->get_logger(), "校验码错误! 类型:0x%02X, 期望:0x%02X, 实际:0x%02X", 
                   msg_type, calculated_checksum, buf[len - 1]);
        return -1;
    }

    // ====================== 握手相关处理 ======================
    if (msg_type == 0x01) {
        if (!handshake_completed_) {
            RCLCPP_INFO(this->get_logger(), "收到握手消息 (ID: %d)", 
                       (buf[5] & 0xff) | ((buf[6] & 0xff) << 8));
        }
        return sendHandshakeAck(buf, len);
    }
    // ====================== 握手处理结束 ======================

    // 使用统一的UnPackMsgPacket处理
    if (!UnPackMsgPacket(std::string((char *)buf, len), MsgPkg)) {
        RCLCPP_WARN(this->get_logger(), "消息解析失败，类型: 0x%02X", msg_type);
        return -1;
    }

    // ====================== 其他消息处理 ======================
    Json::Reader reader;
    Json::Value json_msg;
    Json::Value value_iwv;
    
    if ((MsgType)msg_type == MsgType::AIUI_MSG) {  // AIUI消息
        if (reader.parse(MsgPkg.bytes, json_msg)) {
            // 处理唤醒消息...
            if (json_msg["type"].asString() == "aiui_event") {
                Json::Value content = json_msg["content"];
                if (content["eventType"].asString() == "4")
                {
                    std_msgs::msg::Int8 awake_msg;
                    awake_msg.data = 1;
                    awake_flag_pub->publish(awake_msg);
                    std::string iwv_msg = content["info"].asString();
                    if (reader.parse(iwv_msg,value_iwv))
                    {
                        angle = value_iwv["ivw"]["angle"].asInt();
                        std_msgs::msg::UInt32 angle_msg;
                        angle_msg.data = angle;
                        angle_pub->publish(angle_msg);

                        std_msgs::msg::String msg;
                        msg.data = "小车唤醒";
                        voice_words_pub->publish(msg);
                        std::cout << ">>>>>唤醒角度为: " << angle << "°"<< std::endl;
                    }
                else
                        std::cout << "reader json fail!"<< std::endl;
                }
            } else {
                device_message = json_msg["content"].asString();
                process_result = true;
            }
            return 1;
        }
    } 
    else if ((MsgType)msg_type == MsgType::CONTROL) {  // 控制消息
        if (reader.parse(MsgPkg.bytes, json_msg)) {
            // 打印完整的JSON结构
            RCLCPP_INFO(this->get_logger(), "JSON内容: %s", 
                       json_msg.toStyledString().c_str());

            if (json_msg.isMember("code") && json_msg.isMember("content")) {
                int code = json_msg["code"].asInt();
                std::string content = json_msg["content"].asString();
                
                // 设置设备消息和结果标志
                device_message = content;
                process_result = true;
                last_ack_id_ = MsgPkg.sid;  // 保存确认ID
                
                // 如果是音频请求确认
                if (content == "success" && json_msg.isMember("type")) {
                    std::string type = json_msg["type"].asString();
                    if (type == "get_original_audio") {
                        RCLCPP_INFO(this->get_logger(), "收到音频请求确认 (ID: %d)", MsgPkg.sid);
                    }
                }
                return 1;
            }
        } else {
            RCLCPP_WARN(this->get_logger(), "控制消息JSON解析失败");
            RCLCPP_WARN(this->get_logger(), "原始载荷: %s", MsgPkg.bytes.c_str());
        }
    }
    
    return -1;
}

/********************************************************
Function: Process data from the second serial port
功能: 处理虚拟串口数据
*********************************************************/
int Wheeltec_Mic::process_virtual_serial_data(const unsigned char *buf, int len)
{
    // 检查数据长度
    if (len < 8) {
        RCLCPP_WARN(this->get_logger(), "虚拟串口数据长度不足: %d", len);
        return 0;
    }
    
    // 检查帧头
    if (buf[0] != FRAME_HEADER || buf[1] != USER_ID) {
        return 0;
    }
    
    unsigned char msg_type = buf[2];
    
    // 计算消息长度（小端序）
    unsigned short reported_len = (buf[3] & 0xff) | ((buf[4] & 0xff) << 8);
    unsigned short total_len_needed = 7 + reported_len + 1;
    
    if (len != total_len_needed) {
        RCLCPP_WARN(this->get_logger(), "虚拟串口长度不匹配: 报告=%d, 实际=%d, 需要=%d", 
                   reported_len, len, total_len_needed);
        return 0;
    }
    
    // 验证校验码
    int sum = std::accumulate(buf, buf + len - 1, 0);
    unsigned char calculated_checksum = ((~sum) + 1) & 0xff;
    
    if (calculated_checksum != buf[len - 1]) {
        RCLCPP_WARN(this->get_logger(), "虚拟串口校验码错误! 类型:0x%02X, 期望:0x%02X, 实际:0x%02X", 
                   msg_type, calculated_checksum, buf[len - 1]);
        return 0;
    }
    
    // ====================== 音频数据处理 ======================
    if (msg_type == 0x06) {  // VOICE 消息类型
        // 解析消息包
        if (!UnPackMsgPacket(std::string((char *)buf, len), MsgPkg)) {
             RCLCPP_WARN(this->get_logger(), "虚拟串口消息解析失败，类型: 0x%02X", msg_type);
            return 0;
        }
        
        static int audio_packet_count = 0;
        audio_packet_count++;
        
        // 记录包数
        if (audio_packet_count % 500 == 0) {
            RCLCPP_DEBUG(this->get_logger(), "收到第 %d 个音频包", audio_packet_count);
        }
        
        // 快速验证音频数据
        if (MsgPkg.size == 0 || MsgPkg.bytes.empty()) {
            return 0;
        }
        
        // 处理音频数据
        if (is_audio_receiving_) {
            handleAudioData((const unsigned char*)MsgPkg.bytes.data(), MsgPkg.bytes.size());
            return 2;  
        } 
    }
    // ====================== 音频数据处理结束 ======================
    
    // 对于非音频消息，也可以在此处理或转发
    if (msg_type == 0x01) {  // 握手消息
        RCLCPP_DEBUG(this->get_logger(), "虚拟串口收到握手消息");
    }
    
    return 1;  // 表示成功处理了数据
}

/********************************************************
Function: Start handshake process
功能:  发送握手确认
*********************************************************/
int Wheeltec_Mic::sendHandshakeAck(const unsigned char *buf, int len)
{
    if (len < 7) {
        return -1;
    }
    
    try {
        // 解析收到的握手消息ID
        unsigned short msg_id = (buf[5] & 0xff) | ((buf[6] << 8) & 0xff00);
        // 只在第一次发送握手确认时打印日志
        if (!handshake_completed_) {
            RCLCPP_INFO(this->get_logger(), "已发送握手确认 (ID: %d)", msg_id);
        }
        // 构建握手确认数据 (固定为 0xA5 0x00 0x00 0x00)
        unsigned char handshake_ack_data[4] = {0xA5, 0x00, 0x00, 0x00};
        // 构建完整消息
        std::string ack_message = MakeMsgPacket(msg_id, MsgType::CONFIRM, 
                                          std::string((char*)handshake_ack_data, 4));
        // 发送握手确认
        MicArr_Serial.write(ack_message);
        handshake_completed_ = true;
        return 1;
    } 
    catch (const std::exception& e) {
        RCLCPP_ERROR(this->get_logger(), "发送握手确认失败: %s", e.what());
        return -1;
    }
}

/**************************************
Function: Receive and filter data (智能长度检测)
功能: 过滤数据
***************************************/
int Wheeltec_Mic::uart_analyse_smart(unsigned char buffer)
{
    if (!serial_initialized) return false;
    
    static std::vector<unsigned char> rx_buffer;
    static int frame_count = 0;
    
    rx_buffer.push_back(buffer);
    
    while (rx_buffer.size() >= 7) {
        // 检查帧头
        if (rx_buffer[0] != FRAME_HEADER || rx_buffer[1] != USER_ID) {
            rx_buffer.erase(rx_buffer.begin());
            continue;
        }
        
        // 解析长度（小端序）
        unsigned short reported_len = (rx_buffer[3] & 0xff) | ((rx_buffer[4] & 0xff) << 8);
        unsigned int total_len = 7 + reported_len + 1;
        
        // 验证长度合理性
        if (total_len > MAX_PACKET_SIZE || total_len < 8) {
            static int error_count = 0;
            if (error_count++ % 100 == 0) {
                RCLCPP_WARN(this->get_logger(), "无效的消息长度: %u", total_len);
            }
            rx_buffer.erase(rx_buffer.begin());
            continue;
        }
        
        // 检查是否有完整帧
        if (rx_buffer.size() >= total_len) {
            frame_count++;
            
            // 减少日志频率
            // if (frame_count % 100 == 0) {
            //     unsigned char msg_type = rx_buffer[2];
            //     RCLCPP_DEBUG(this->get_logger(), "处理第 %d 帧，类型: 0x%02X，长度: %u", 
            //                frame_count, msg_type, reported_len);
            // }
            
            // 处理帧
            int ret = process_data(rx_buffer.data(), total_len);
            
            // 删除已处理的数据
            if (total_len <= rx_buffer.size()) {
                rx_buffer.erase(rx_buffer.begin(), rx_buffer.begin() + total_len);
            } else {
                rx_buffer.clear();
            }
            
            return ret;
        } else {
            // 帧不完整，等待更多数据
            break;
        }
    }
    
    // 清理过大的缓冲区
    if (rx_buffer.size() > 65536) {
        RCLCPP_WARN(this->get_logger(), "缓冲区过大，清空: %zu 字节", rx_buffer.size());
        rx_buffer.clear();
    }
    
    return 0;
}

/**************************************
Function: Receive and filter data for the second serial port
功能: 虚拟串口的数据分析
***************************************/
int Wheeltec_Mic::virtual_uart_analyse_smart(unsigned char buffer)
{
    if (!virtual_serial_initialized) return false;
    
    static std::vector<unsigned char> rx_buffer;
    static int frame_count = 0;
    
    rx_buffer.push_back(buffer);
    
    while (rx_buffer.size() >= 7) {
        // 检查帧头
        if (rx_buffer[0] != FRAME_HEADER || rx_buffer[1] != USER_ID) {
            rx_buffer.erase(rx_buffer.begin());
            continue;
        }
        
        // 解析长度（小端序）
        unsigned short reported_len = (rx_buffer[3] & 0xff) | ((rx_buffer[4] & 0xff) << 8);
        unsigned int  total_len = 7 + reported_len + 1;
        
        // 验证长度合理性
        if (total_len > MAX_PACKET_SIZE || total_len < 8) {
            static int error_count = 0;
            if (error_count++ % 100 == 0) {
                RCLCPP_WARN(this->get_logger(), "虚拟串口无效的消息长度: %u", total_len);
            }
            rx_buffer.erase(rx_buffer.begin());
            continue;
        }
        
        // 检查是否有完整帧
        if (rx_buffer.size() >= total_len) {
            frame_count++;
            
            // 处理帧
            int ret = process_virtual_serial_data(rx_buffer.data(), total_len);
            
            // 删除已处理的数据
            if (total_len <= rx_buffer.size()) {
                rx_buffer.erase(rx_buffer.begin(), rx_buffer.begin() + total_len);
            } else {
                rx_buffer.clear();
            }
            
            return ret;
        } else {
            // 帧不完整，等待更多数据
            break;
        }
    }
    
    // 清理过大的缓冲区
    if (rx_buffer.size() > 65536) {
        RCLCPP_WARN(this->get_logger(), "虚拟串口缓冲区过大，清空: %zu 字节", rx_buffer.size());
        rx_buffer.clear();
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
        
        // 一次性读取所有可用数据
        std::vector<unsigned char> buffer(available);
        size_t bytes_read = MicArr_Serial.read(buffer.data(), available);
        bool result = false;

        if (bytes_read > 0) {
            static int total_bytes_read = 0;
            total_bytes_read += bytes_read;
            
            // 批量处理所有数据
            for (size_t i = 0; i < bytes_read; i++) {
                // 使用uart_analyse_smart，因为它处理单个字节
                int ret = uart_analyse_smart(buffer[i]);
                if (ret != 0) {
                    result = true;
                }
            }
        }
        
        return result;
    } catch (const serial::IOException& e) {
        RCLCPP_ERROR(this->get_logger(), "串口读取错误: %s", e.what());
        handle_serial_error();
    }
    return false;
}

/**************************************
Function: Receive data from the second serial port
功能: 接收虚拟串口发送的信息
***************************************/
bool Wheeltec_Mic::Get_Virtual_Serial_Data()
{
    if (!virtual_serial_initialized) return false;

    try {
        size_t available = Virtual_Serial.available();
        if (available == 0) return false;
        
        // 一次性读取所有可用数据
        std::vector<unsigned char> buffer(available);
        size_t bytes_read = Virtual_Serial.read(buffer.data(), available);
        bool result = false;

        if (bytes_read > 0) {
            // 批量处理所有数据
            for (size_t i = 0; i < bytes_read; i++) {
                // 使用virtual_uart_analyse_smart处理
                int ret = virtual_uart_analyse_smart(buffer[i]);
                if (ret != 0) {
                    result = true;
                }
            }
        }
        
        return result;
    } catch (const serial::IOException& e) {
        RCLCPP_ERROR(this->get_logger(), "虚拟串口读取错误: %s", e.what());
        handle_virtual_serial_error();
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
Function: Handle the second serial port errors
功能: 处理虚拟串口异常
***************************************/
void Wheeltec_Mic::handle_virtual_serial_error() 
{
    virtual_serial_initialized = false;
    Virtual_Serial.close();
    
    RCLCPP_INFO(this->get_logger(), "Attempting to reconnect to second serial port...");
    for (int i = 0; i < 3; ++i) {
        try {
            Virtual_Serial.open();
            if (Virtual_Serial.isOpen()) {
                Virtual_Serial.flush();
                virtual_serial_initialized = true;
                RCLCPP_INFO(this->get_logger(), "Second serial port reconnected successfully");
                return;
            }
        } catch (...) {
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
    }
    RCLCPP_ERROR(this->get_logger(), "Failed to reconnect to second serial port");
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
Function: Second serial port data processing callback
功能: 虚拟串口数据处理回调函数
***************************************/
void Wheeltec_Mic::virtual_serial_read_callback()
{
    if (!virtual_serial_initialized) {
        RCLCPP_ERROR(this->get_logger(), "Second serial port not initialized");
        return;
    } 
    Get_Virtual_Serial_Data();
}

/**************************************
Function: Loop access to the lower computer data and issue topics
功能: 循环获取下位机数据与发布话题
***************************************/
void Wheeltec_Mic::run()
{
    if (!serial_initialized) {
        RCLCPP_ERROR(this->get_logger(), "real serial port initialization failed");
    }
    
    if (!virtual_serial_initialized) {
        RCLCPP_WARN(this->get_logger(), "virtual serial port initialization failed, continuing without it");
    }

    // 创建定时器，20ms读取一次主串口数据
    timer_ = this->create_wall_timer(
        std::chrono::milliseconds(20), 
        std::bind(&Wheeltec_Mic::serial_read_callback, this));
    
    RCLCPP_INFO(this->get_logger(), "Wheeltec Mic Node started");
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
                // ========== 清除串口缓存 ==========
                // 清理输入缓冲区
                MicArr_Serial.flushInput();
                // 清理输出缓冲区
                MicArr_Serial.flushOutput();
                // 读取并丢弃串口中所有已缓存的数据
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
                size_t bytes_to_discard = MicArr_Serial.available();
                if (bytes_to_discard > 0) {
                    std::vector<unsigned char> discard_buffer(bytes_to_discard);
                    MicArr_Serial.read(discard_buffer.data(), bytes_to_discard);
                }
                
                // 再等待一小段时间确保完全清空
                std::this_thread::sleep_for(std::chrono::milliseconds(50));
                
                // 重置握手状态
                handshake_completed_ = false;
                
                RCLCPP_INFO(get_logger(), "Serial port initialized successfully");
                // ========== 缓存清理完成 ==========
                
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
Function: enable the virtual serial port
功能: 使能虚拟串口
***************************************/
void Wheeltec_Mic::enable_virtual_serial(bool enable)
{
    if (enable) {
        // 需要启用虚拟串口
        if (!virtual_serial_initialized) {
            // 尝试初始化串口
            initialize_virtual_serial();
            if (!virtual_serial_initialized) {
                RCLCPP_ERROR(this->get_logger(), "无法打开虚拟串口，音频接收失败");
                return;
            }
            
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
            size_t bytes_to_discard = Virtual_Serial.available();
            if (bytes_to_discard > 0) {
                std::vector<unsigned char> discard_buffer(bytes_to_discard);
                Virtual_Serial.read(discard_buffer.data(), bytes_to_discard);
            }
        }
        // 启动定时器（如果未运行）
        if (!virtual_serial_timer_) {
            virtual_serial_timer_ = this->create_wall_timer(
                std::chrono::milliseconds(10),
                std::bind(&Wheeltec_Mic::virtual_serial_read_callback, this));
            RCLCPP_INFO(this->get_logger(), "虚拟串口定时器已启动");
        }
    } else {
        // 需要禁用虚拟串口
        if (virtual_serial_timer_) {
            virtual_serial_timer_->cancel();
            virtual_serial_timer_.reset();
        }
        
        // **新增：关闭前清空缓冲区**
        if (Virtual_Serial.isOpen()) {
            std::this_thread::sleep_for(std::chrono::milliseconds(20));
            size_t bytes_to_discard = Virtual_Serial.available();
            if (bytes_to_discard > 0) {
                std::vector<unsigned char> discard_buffer(bytes_to_discard);
                Virtual_Serial.read(discard_buffer.data(), bytes_to_discard);
            }
            Virtual_Serial.close();
        }
        
        virtual_serial_initialized = false;
        RCLCPP_INFO(this->get_logger(), "虚拟串口已关闭");
    }
}

/**************************************
Function: Initialize the virtual serial port
功能: 初始化虚拟串口
***************************************/
void Wheeltec_Mic::initialize_virtual_serial() {
    const int max_retries = 3;
    serial::Timeout timeout = serial::Timeout::simpleTimeout(1000); 
    for (int retry = 0; retry < max_retries; ++retry) {
        try {
            if (Virtual_Serial.isOpen()) Virtual_Serial.close();
            
            Virtual_Serial.setPort(virtual_usart_port_name);
            Virtual_Serial.setBaudrate(virtual_serial_baud_rate);
            Virtual_Serial.setTimeout(timeout);
            Virtual_Serial.open();
            
            if (Virtual_Serial.isOpen()) {
                Virtual_Serial.flush();
                virtual_serial_initialized = true;
                
                // 清除串口缓存
                Virtual_Serial.flushInput();
                Virtual_Serial.flushOutput();
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
                size_t bytes_to_discard = Virtual_Serial.available();
                if (bytes_to_discard > 0) {
                    std::vector<unsigned char> discard_buffer(bytes_to_discard);
                    Virtual_Serial.read(discard_buffer.data(), bytes_to_discard);
                }
                
                std::this_thread::sleep_for(std::chrono::milliseconds(50));
                
                RCLCPP_INFO(get_logger(), "Virtual serial port initialized successfully: %s", virtual_usart_port_name.c_str());
                return;
            }
        } catch (const std::exception& e) {
            RCLCPP_ERROR(get_logger(), "Virtual serial init attempt %d failed: %s", retry+1, e.what());
        }
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
    RCLCPP_WARN(get_logger(), "Failed to initialize virtual serial port after %d attempts", max_retries);
}

std::string Wheeltec_Mic::convertToSourcePath(const std::string& install_path)
{
    std::string source_path = install_path;
    
    size_t install_pos = source_path.find("/install/");
    if (install_pos != std::string::npos) {
        std::string workspace_path = source_path.substr(0, install_pos);
        
        size_t pkg_start = source_path.find("/share/");
        if (pkg_start != std::string::npos) {
            pkg_start += 7; // 跳过"/share/"
            size_t pkg_end = source_path.find("/", pkg_start);
            if (pkg_end != std::string::npos) {
                std::string pkg_name = source_path.substr(pkg_start, pkg_end - pkg_start);
                // 构建源码路径: workspace/src/pkg_name/audio/
                source_path = workspace_path + "/src/wheeltec_mic/" + pkg_name + "/audio/";
            }
        }
    }
    return source_path;
}

/**************************************
Function: Constructor, executed only once, for initialization
功能: 构造函数, 用于初始化
***************************************/
Wheeltec_Mic::Wheeltec_Mic(const std::string &node_name)
:rclcpp::Node(node_name),serial_initialized(false),virtual_serial_initialized(false){
    memset(&Receive_Data, 0, sizeof(Receive_Data));

    setupMicArrayServices();

    this->declare_parameter<std::string>("usart_port_name","/dev/ttyCH343USB0");
    this->declare_parameter<int>("serial_baud_rate",115200);
    this->declare_parameter<std::string>("virtual_usart_port_name","/dev/ttyUSB1");
    this->declare_parameter<int>("virtual_serial_baud_rate",115200);
    this->declare_parameter<std::string>("audio_path","");

    this->get_parameter("usart_port_name",usart_port_name);
    this->get_parameter("serial_baud_rate",serial_baud_rate);
    this->get_parameter("virtual_usart_port_name",virtual_usart_port_name);
    this->get_parameter("virtual_serial_baud_rate",virtual_serial_baud_rate);
    this->get_parameter("audio_path",audio_path);
    
    if (!audio_path.empty()) {
        if (audio_path.find("/install/") != std::string::npos) {
            audio_path = convertToSourcePath(audio_path);
        }
        if (audio_path.back() != '/') {
            audio_path += '/';
        }
    }

    /***唤醒标志位话题发布者创建***/
    awake_flag_pub = this->create_publisher<std_msgs::msg::Int8>("awake_flag",10);
    /***麦克风设备串口打开标志位话题发布者创建***/
    voice_flag_pub = this->create_publisher<std_msgs::msg::Int8>("voice_flag",10);
    /***唤醒角度话题发布者创建***/
    angle_pub = this->create_publisher<std_msgs::msg::UInt32>("awake_angle",10);
    /***命令词话题发布者创建***/
    voice_words_pub = this->create_publisher<std_msgs::msg::String>("voice_words",10);

    initialize_serial();
    initialize_virtual_serial();
}

Wheeltec_Mic::~Wheeltec_Mic()
{
    RCLCPP_INFO(this->get_logger(),"wheeltec_mic_node over!\n");

    if (timer_) {
        timer_->cancel();
    }
    
    if (virtual_serial_timer_) {
        virtual_serial_timer_->cancel();
    }

    if (MicArr_Serial.isOpen()) {
        MicArr_Serial.close();
    }
    
    if (Virtual_Serial.isOpen()) {
        Virtual_Serial.close();
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