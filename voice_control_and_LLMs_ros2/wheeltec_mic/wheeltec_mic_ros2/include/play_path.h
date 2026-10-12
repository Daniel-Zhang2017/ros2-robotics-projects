#ifndef __GLOBAL_H__
#define __GLOBAL_H__

#include <iostream>
#include <string>
#include <memory>
#include <cstring>
#include <atomic>
#include <mutex>
#include <condition_variable>
#include <thread>
#include <chrono>
#include "shared_memory.h"
#include "rclcpp/rclcpp.hpp"
#include "std_msgs/msg/int8.hpp" 

std::string Launch;
std::string audio_path = "";
std::string device_type_;
const std::string head = "aplay -D plughw:CARD=Device,DEV=0 ";
const std::string gnome_terminal = "dbus-launch gnome-terminal -- ";
const std::string simple_follower = "ros2 launch simple_follower_ros2 ";
const std::string wheeltec_mic_ros2 = "ros2 launch wheeltec_mic_ros2 ";

/**
 * @brief 使用共享内存同步播报状态
 */
class PlayStateSync {
public:
    static void start() {
        SharedPlayStateData* data = g_shared_mem;
        if (data) {
            data->is_playing = true;
            data->is_finished = false;
            data->sequence++;
        }
    }
    
    static void finish() {
        SharedPlayStateData* data = g_shared_mem;
        if (data) {
            data->is_playing = false;
            data->is_finished = true;
            data->sequence++;
        }
    }
    
    static bool waitForFinish(int timeout_seconds) {
        SharedPlayStateData* data = g_shared_mem;
        if (!data) {
            RCLCPP_WARN(rclcpp::get_logger("SharedPlayState"), 
                        "waitForFinish: 共享内存无效");
            return false;
        }
        
        int elapsed_ms = 0;
        const int max_wait_ms = timeout_seconds * 1000;

        int sleep_ms = 5;      // 初始 5ms
        const int max_sleep_ms = 50;  // 最大 50ms
        
        RCLCPP_INFO(rclcpp::get_logger("SharedPlayState"), 
                    "waitForFinish: 开始等待，超时 %d 秒", timeout_seconds);
        
        while (elapsed_ms < max_wait_ms) {
            if (data->is_finished.load()) {
                RCLCPP_INFO(rclcpp::get_logger("SharedPlayState"), 
                            "waitForFinish: 检测到完成，等待时间 %d ms", elapsed_ms);
                return true;
            }
            
            // 动态调整休眠时间
            std::this_thread::sleep_for(std::chrono::milliseconds(sleep_ms));
            elapsed_ms += sleep_ms;
            
            // 逐渐增加休眠时间，但不超过最大值
            if (sleep_ms < max_sleep_ms && elapsed_ms > 500) {
                sleep_ms = max_sleep_ms;
            }
        }
        
        RCLCPP_WARN(rclcpp::get_logger("SharedPlayState"), 
                    "waitForFinish: 超时 %d 秒", timeout_seconds);
        return false;
    }
    
    static bool isFinished() {
        SharedPlayStateData* data = g_shared_mem;
        return data ? data->is_finished.load() : false;
    }
    
    static bool isPlaying() {
        SharedPlayStateData* data = g_shared_mem;
        return data ? data->is_playing.load() : false;
    }
    
    static void reset() {
        SharedPlayStateData* data = g_shared_mem;
        if (data) {
            data->is_playing = false;
            data->is_finished = false;
            data->sequence++;
        }
    }
    
    static void init() {
        // 确保共享内存已初始化
        SharedMemory::getInstance();
    }
};

#define g_play_state PlayStateSync

namespace audio_utils {
/**
 * @brief 安全地拼接字符串和字符数组
 * @param str 标准字符串
 * @param c_str C风格字符串
 * @return 拼接后的字符数组（需要调用者负责释放）
 */
inline char* join(const std::string& str, const char* c_str) {
    if (!c_str) {
        std::cerr << "错误: 输入的C字符串为空" << std::endl;
        return nullptr;
    }
    
    try {
        size_t total_length = str.length() + std::strlen(c_str) + 1;
        char* result = new char[total_length];
        
        std::strcpy(result, str.c_str());
        std::strcat(result, c_str);
        
        return result;
    }
    catch (const std::bad_alloc& e) {
        std::cerr << "内存分配失败: " << e.what() << std::endl;
        return nullptr;
    }
    catch (const std::exception& e) {
        std::cerr << "字符串拼接失败: " << e.what() << std::endl;
        return nullptr;
    }
}

/**
 * @brief 使用智能指针的拼接函数，避免内存泄漏
 * @param str 标准字符串
 * @param c_str C风格字符串
 * @return 包含拼接结果的unique_ptr
 */
inline std::unique_ptr<char[]> join_smart(const std::string& str, const char* c_str) {
    if (!c_str) {
        std::cerr << "错误: 输入的C字符串为空" << std::endl;
        return std::unique_ptr<char[]>(nullptr);
    }
    
    try {
        size_t total_length = str.length() + std::strlen(c_str) + 1;
        // 手动创建 unique_ptr，不使用 make_unique
        std::unique_ptr<char[]> result(new char[total_length]);
        
        std::strcpy(result.get(), str.c_str());
        std::strcat(result.get(), c_str);
        
        return result;
    }
    catch (const std::exception& e) {
        std::cerr << "字符串拼接失败: " << e.what() << std::endl;
        return std::unique_ptr<char[]>(nullptr);
    }
}

} // namespace audio_utils

#endif