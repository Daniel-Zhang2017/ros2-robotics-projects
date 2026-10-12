/****************************************************************/
/* Copyright (c) 2025 WHEELTEC Technology, Inc                */
/* 功能：唤醒检测器                                               */
/****************************************************************/

#include "wheeltec_m07.h"

using namespace std::chrono_literals;

// ================ SerialPort 实现 ================
SerialPort::SerialPort(const std::string& port) : fd(-1), port_name(port) {}

SerialPort::~SerialPort() {
    close();
}

bool SerialPort::open(int baud) {
    fd = ::open(port_name.c_str(), O_RDWR | O_NOCTTY);
    if (fd < 0) {
        RCLCPP_ERROR(rclcpp::get_logger("serial"), "无法打开串口 %s", port_name.c_str());
        return false;
    }
    
    // 配置串口
    struct termios tty;
    tcgetattr(fd, &tty);
    
    // 设置波特率
    speed_t speed = B115200;
    switch(baud) {
        case 9600: speed = B9600; break;
        case 19200: speed = B19200; break;
        case 38400: speed = B38400; break;
        case 57600: speed = B57600; break;
        case 115200: speed = B115200; break;
    }
    cfsetispeed(&tty, speed);
    cfsetospeed(&tty, speed);
    
    // 8N1配置
    tty.c_cflag &= ~PARENB;
    tty.c_cflag &= ~CSTOPB;
    tty.c_cflag &= ~CSIZE;
    tty.c_cflag |= CS8;
    tty.c_cflag &= ~CRTSCTS;
    tty.c_cflag |= CREAD | CLOCAL;
    
    tty.c_iflag &= ~(IXON | IXOFF | IXANY);
    tty.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG);
    tty.c_oflag &= ~OPOST;
    
    tty.c_cc[VMIN] = 0;
    tty.c_cc[VTIME] = 1;
    
    tcsetattr(fd, TCSANOW, &tty);
    tcflush(fd, TCIOFLUSH);
    
    RCLCPP_INFO(rclcpp::get_logger("serial"), "串口打开成功: %s @ %d bps", port_name.c_str(), baud);
    return true;
}

int SerialPort::read_data(uint8_t* buf, size_t size) {
    return read(fd, buf, size);
}

void SerialPort::close() {
    if (fd >= 0) ::close(fd);
    fd = -1;
}

// ================ AwakeDetector 实现 ================
AwakeDetector::AwakeDetector(const WheeltecConfig& cfg) 
    : Node("awake_detector"), 
      config(cfg), 
      serial(cfg.port), 
      state(IDLE),
      last_awake_time(this->now()) {
    
    // 声明参数
    this->declare_parameter<std::string>("usart_port_name", cfg.port);
    this->declare_parameter<int>("serial_baud_rate", cfg.baud);
    this->declare_parameter<bool>("debug_mode", cfg.debug);
    this->declare_parameter<double>("min_interval", cfg.min_interval);
    
    // 获取参数
    this->get_parameter("usart_port_name", config.port);
    this->get_parameter("serial_baud_rate", config.baud);
    this->get_parameter("debug_mode", config.debug);
    this->get_parameter("min_interval", config.min_interval);
    
    // 创建发布者
    pub_awake = this->create_publisher<std_msgs::msg::Int8>("awake_flag", 10);
    pub_voice = this->create_publisher<std_msgs::msg::Int8>("voice_flag", 10);
    voice_words_pub = this->create_publisher<std_msgs::msg::String>("voice_words",10);
    
    if (!serial.open(config.baud)) {
        throw std::runtime_error("串口初始化失败");
    }
    publish_voice_flag();
}

void AwakeDetector::process() {
    uint8_t buf[256];
    int n = serial.read_data(buf, sizeof(buf));
    
    if (n > 0) {
        process_bytes(buf, n);
    }
}

void AwakeDetector::publish_voice_flag() {
    auto voice_msg = std_msgs::msg::Int8();
    voice_msg.data = 1;
    pub_voice->publish(voice_msg);
    RCLCPP_INFO(this->get_logger(), "成功打开麦克风设备");
    RCLCPP_INFO(this->get_logger(), "以降噪板设置的唤醒词为准[默认:你好小微]");
}

void AwakeDetector::process_bytes(uint8_t* data, int len) {
    for (int i = 0; i < len; i++) {
        uint8_t byte = data[i];
        
        switch(state) {
            case IDLE:
                if (byte == 0xDE) state = GOT_DE;
                break;
                
            case GOT_DE:
                if (byte == 0x5B) {
                    state = GOT_5B;
                } else {
                    state = IDLE;
                }
                break;
                
            case GOT_5B:
                // 检查是否完整唤醒包
                check_awake_packet(byte);
                state = IDLE;
                break;
        }
        
        if (config.debug && len > 0) {
            static int count = 0;
            if (count++ % 50 == 0) {
                std::cout << "数据: ";
                for (int j = 0; j < std::min(len, 10); j++) {
                    printf("%02X ", data[j]);
                }
                std::cout << std::endl;
            }
        }
    }
}

void AwakeDetector::check_awake_packet(uint8_t third_byte) {
    rclcpp::Time now = this->now();
    double interval = (now - last_awake_time).seconds();
    
    if (interval > config.min_interval) {
        // 发布awake_flag
        auto awake_msg = std_msgs::msg::Int8();
        awake_msg.data = 1;
        pub_awake->publish(awake_msg);

        std_msgs::msg::String msg;
        msg.data = "小车唤醒";
        voice_words_pub->publish(msg);
        last_awake_time = now;
        RCLCPP_INFO(this->get_logger(), "已成功唤醒！");
    }
}

int main(int argc, char** argv) {
    rclcpp::init(argc, argv);
    
    WheeltecConfig cfg;
    
    try {
        auto node = std::make_shared<AwakeDetector>(cfg);
        rclcpp::Rate rate(100);
        
        while (rclcpp::ok()) {
            node->process();
            rclcpp::spin_some(node);
            rate.sleep();
        }
    }
    catch (const std::exception& e) {
        RCLCPP_FATAL(rclcpp::get_logger("main"), "错误: %s", e.what());
        return 1;
    }
    
    rclcpp::shutdown();
    return 0;
}