#ifndef __SHARED_MEMORY_H__
#define __SHARED_MEMORY_H__

#include <iostream>
#include <atomic>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>
#include <cstring>
#include <errno.h>

#define SHARED_MEM_NAME "/wheeltec_play_state"
#define SHARED_MEM_SIZE sizeof(SharedPlayStateData)

/**
 * @brief 共享内存数据结构
 */
struct SharedPlayStateData {
    std::atomic<bool> is_playing{false};
    std::atomic<bool> is_finished{false};
    // 添加序列号，用于检测更新
    std::atomic<uint32_t> sequence{0};
};

/**
 * @brief 共享内存管理类
 */
class SharedMemory {
private:
    int fd_;
    SharedPlayStateData* data_;
    bool is_owner_;
    
    SharedMemory() : fd_(-1), data_(nullptr), is_owner_(false) {
        // 尝试创建或打开共享内存
        fd_ = shm_open(SHARED_MEM_NAME, O_CREAT | O_RDWR, 0666);
        if (fd_ == -1) {
            std::cerr << "shm_open failed: " << strerror(errno) << std::endl;
            return;
        }
        
        // 获取共享内存大小
        struct stat sb;
        if (fstat(fd_, &sb) == -1) {
            std::cerr << "fstat failed: " << strerror(errno) << std::endl;
            close(fd_);
            fd_ = -1;
            return;
        }
        
        // 如果共享内存大小为0，设置大小
        if (sb.st_size == 0) {
            if (ftruncate(fd_, SHARED_MEM_SIZE) == -1) {
                std::cerr << "ftruncate failed: " << strerror(errno) << std::endl;
                close(fd_);
                fd_ = -1;
                return;
            }
            is_owner_ = true;
        }
        
        // 映射共享内存
        data_ = (SharedPlayStateData*)mmap(NULL, SHARED_MEM_SIZE, 
                                            PROT_READ | PROT_WRITE, 
                                            MAP_SHARED, fd_, 0);
        if (data_ == MAP_FAILED) {
            std::cerr << "mmap failed: " << strerror(errno) << std::endl;
            close(fd_);
            fd_ = -1;
            data_ = nullptr;
            return;
        }
        
        // 如果是创建者，初始化数据
        if (is_owner_) {
            new (data_) SharedPlayStateData();
        } else {
            std::cout << "SharedMemory: link" << std::endl;
        }
    }
    
    ~SharedMemory() {
        if (data_ != nullptr && data_ != MAP_FAILED) {
            munmap(data_, SHARED_MEM_SIZE);
        }
        if (fd_ != -1) {
            close(fd_);
        }
    }
    
public:
    // 禁止拷贝
    SharedMemory(const SharedMemory&) = delete;
    SharedMemory& operator=(const SharedMemory&) = delete;
    
    static SharedMemory& getInstance() {
        static SharedMemory instance;
        return instance;
    }
    
    /**
     * @brief 获取共享数据指针
     */
    SharedPlayStateData* get() {
        return data_;
    }
    
    /**
     * @brief 检查共享内存是否有效
     */
    bool isValid() const {
        return data_ != nullptr && data_ != MAP_FAILED;
    }
    
    /**
     * @brief 删除共享内存（在程序退出时调用）
     */
    static void cleanup() {
        shm_unlink(SHARED_MEM_NAME);
    }
};

// 全局访问宏
#define g_shared_mem (SharedMemory::getInstance().get())

#endif // __SHARED_MEMORY_H__