#ifndef __WHEELTEC_MIC_H_
#define __WHEELTEC_MIC_H_

#include <vector>
#include <memory>
#include <functional>
#include <atomic>
#include <string>
#include <sys/stat.h>
#include <iostream>
#include <unistd.h>
#include <numeric>
#include <chrono>
#include <thread>
#include <serial/serial.h>
#include <std_msgs/msg/int8.hpp>
#include <std_msgs/msg/string.hpp>
#include "std_msgs/msg/u_int32.hpp"
#include "jsoncpp/json/json.h"
#include "rclcpp/rclcpp.hpp"
#include "wheeltec_mic_msg/srv/set_awake_word.hpp"
#include "wheeltec_mic_msg/srv/get_device_type.hpp"
#include "wheeltec_mic_msg/srv/switch_mic.hpp"
#include "wheeltec_mic_msg/srv/get_audio_data.hpp"
#include "wheeltec_mic_msg/srv/set_beam.hpp"

#define FRAME_HEADER        0XA5        
#define USER_ID             0X01        
#define TIMEOUT             10.0

const unsigned int MAX_PACKET_SIZE = 65536;

enum class MsgType : unsigned char {
    Shake =             0x01,
    AIUI_MSG =          0x04,
    CONTROL =           0x05,
    VOICE =             0x06,
    CONFIRM =           0xFF
};

enum class ServiceType{
    AWAKE_WORD,
    SWITCH_MIC,
    DEVICE_VER,
    SET_AUDIO,
    SET_BEAM
};

struct ServicePacket
{
    unsigned short sid;

    int beam;
    int mode;
    std::string type;
    std::string threshold;
    std::string awake_word;
    std::string mic_type;
    std::string content;

    ServiceType msgType;
};

struct MsgPacket
{
    unsigned char uid;
    unsigned char type;

    unsigned short size;
    unsigned short sid;

    std::string bytes;
};

class Wheeltec_Mic : public rclcpp::Node{
public:
    Wheeltec_Mic(const std::string &node_name);
    ~Wheeltec_Mic();
    // 音频控制接口
    bool startAudioReceiving(unsigned int duration_sec = 5);
    bool stopAudioReceiving(bool auto_save = true);
    bool saveCurrentAudio(const std::string& filename = "");
    void handleAudioData(const unsigned char* data, unsigned int len);
    void run();

    serial::Serial MicArr_Serial;
    serial::Serial Virtual_Serial;  // 虚拟串口

private:
    int angle,serial_baud_rate,virtual_serial_baud_rate;
    bool process_result;
    bool serial_initialized, virtual_serial_initialized;
    bool handshake_completed_ = false;
    unsigned char Receive_Data[1024] = {0};
    std::string device_message,usart_port_name,virtual_usart_port_name,audio_path;
    std::atomic<unsigned short> server_msg_id_counter_{0};

    // 音频相关成员变量
    std::vector<unsigned char> audio_buffer_;
    std::mutex audio_mutex_;
    bool is_audio_receiving_ = false;
    unsigned int audio_received_bytes_ = 0;
    unsigned int stop_after_size_ = 0;      // 停止接收的大小阈值（字节）
    unsigned int audio_duration_ = 5000;    // 默认音频时长（毫秒）
    std::chrono::steady_clock::time_point audio_start_time_;  // 音频开始时间
    unsigned short last_ack_id_ = 0;        // 最后确认的ID

    MsgPacket MsgPkg;
    ServicePacket ServicePkg;

    rclcpp::TimerBase::SharedPtr timer_;
    rclcpp::TimerBase::SharedPtr virtual_serial_timer_;  // 虚拟串口定时器
    rclcpp::Time start_time, last_time;
    rclcpp::Publisher<std_msgs::msg::UInt32>::SharedPtr angle_pub;
    rclcpp::Publisher<std_msgs::msg::String>::SharedPtr voice_words_pub;
    rclcpp::Publisher<std_msgs::msg::Int8>::SharedPtr awake_flag_pub,voice_flag_pub;
    rclcpp::Service<wheeltec_mic_msg::srv::GetDeviceType>::SharedPtr get_device_srv_;
    rclcpp::Service<wheeltec_mic_msg::srv::SetAwakeWord>::SharedPtr set_awake_word_srv_;
    rclcpp::Service<wheeltec_mic_msg::srv::SwitchMic>::SharedPtr switch_mic_srv_;
    rclcpp::Service<wheeltec_mic_msg::srv::GetAudioData>::SharedPtr get_audio_srv_;
    rclcpp::Service<wheeltec_mic_msg::srv::SetBeam>::SharedPtr set_beam_srv_;

    bool Get_Serial_Data();
    bool Get_Virtual_Serial_Data();  // 获取虚拟串口数据
    bool UnPackMsgPacket(const std::string &content, MsgPacket &data);
    void setupMicArrayServices();
    void handle_serial_error();
    void handle_virtual_serial_error();  // 处理虚拟串口错误
    void serial_read_callback();
    void virtual_serial_read_callback();  // 虚拟串口回调
    void initialize_serial();
    void initialize_virtual_serial();  // 初始化虚拟串口
    void enable_virtual_serial(bool enable);
    unsigned short getNewMessageId();
    // 音频控制函数
    bool sendAudioStartCommand(unsigned int duration_sec);
    bool sendAudioStopCommand();
    int uart_analyse(unsigned char buffer);
    int process_data(const unsigned char *buf, int len);
    int process_virtual_serial_data(const unsigned char *buf, int len);  // 处理虚拟串口数据
    int sendHandshakeAck(const unsigned char *buf, int len);
    int uart_analyse_smart(unsigned char buffer);
    int virtual_uart_analyse_smart(unsigned char buffer);  // 虚拟串口数据分析
    std::string MakeMsgPacket(unsigned short sid, MsgType type, const std::string &content);
    std::string convertToSourcePath(const std::string& install_path);

    bool Send_Serial_Data(ServicePacket &pkg);
    bool getDeviceTypeCallback(const std::shared_ptr<wheeltec_mic_msg::srv::GetDeviceType::Request>& request,
                                std::shared_ptr<wheeltec_mic_msg::srv::GetDeviceType::Response>& response);
    bool setAwakeWordCallback(const std::shared_ptr<wheeltec_mic_msg::srv::SetAwakeWord::Request>& request,
                                std::shared_ptr<wheeltec_mic_msg::srv::SetAwakeWord::Response>& response);
    bool switchMicCallback(const std::shared_ptr<wheeltec_mic_msg::srv::SwitchMic::Request>& request,
                                std::shared_ptr<wheeltec_mic_msg::srv::SwitchMic::Response>& response);
    bool getAudioDataCallback(const std::shared_ptr<wheeltec_mic_msg::srv::GetAudioData::Request>& request,
                               std::shared_ptr<wheeltec_mic_msg::srv::GetAudioData::Response>& response);
    bool setBeamCallback(const std::shared_ptr<wheeltec_mic_msg::srv::SetBeam::Request>& request,
                                std::shared_ptr<wheeltec_mic_msg::srv::SetBeam::Response>& response);
    
};

#endif