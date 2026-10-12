#include "PCMPlayer.h"

PCMPlayer::PCMPlayer(unsigned int rate = 16000, 
         snd_pcm_format_t fmt = SND_PCM_FORMAT_S16_LE, 
         int ch = 1)
    : sample_rate(rate), format(fmt), channels(ch), playback_handle(nullptr) {}

PCMPlayer::~PCMPlayer() {
    std::lock_guard<std::mutex> lock(alsa_mutex);
    if (playback_handle) {
        snd_pcm_drain(playback_handle);
        snd_pcm_close(playback_handle);
        playback_handle = nullptr;
    }
}

/**
 * 初始化播放器
 */
bool PCMPlayer::init_alsa() {
    int err;
    unsigned int buffer_time, period_time;

    if (playback_handle) {
        snd_pcm_drain(playback_handle);
        snd_pcm_close(playback_handle);
        playback_handle = nullptr;
    }

    if (err = snd_pcm_open(&playback_handle, DEVICE_NAME, SND_PCM_STREAM_PLAYBACK, 0)) {
        std::cerr << "ALSA open error: " << snd_strerror(err) << std::endl;
        return false;
    }

    snd_pcm_hw_params_t* params;
    snd_pcm_hw_params_alloca(&params);
    snd_pcm_hw_params_any(playback_handle, params);

    if ((err = snd_pcm_hw_params_set_access(playback_handle, params, 
                                          SND_PCM_ACCESS_RW_INTERLEAVED))) {
        std::cerr << "Set access error: " << snd_strerror(err) << std::endl;
        return false;
    }

    // 设置音频参数（需要与输入PCM数据匹配）
    snd_pcm_hw_params_set_format(playback_handle, params, format);
    snd_pcm_hw_params_set_channels(playback_handle, params, channels);
    snd_pcm_hw_params_set_rate_near(playback_handle, params, &sample_rate, 0);

    // 设置周期大小（
    snd_pcm_uframes_t period_size = 640;
    snd_pcm_hw_params_set_period_size_near(playback_handle, params, &period_size, NULL);

    // 设置缓冲区大小（周期大小的 4 倍）
    snd_pcm_uframes_t buffer_size = period_size * 4;
    snd_pcm_hw_params_set_buffer_size_near(playback_handle, params, &buffer_size);

    if ((err = snd_pcm_hw_params(playback_handle, params))) {
        std::cerr << "Params set error: " << snd_strerror(err) << std::endl;
        return false;
    }

    // 软件参数优化（减少启动欠载）
    snd_pcm_sw_params_t* sw_params;
    snd_pcm_sw_params_alloca(&sw_params);
    snd_pcm_sw_params_current(playback_handle, sw_params);
    snd_pcm_sw_params_set_start_threshold(playback_handle, sw_params, buffer_size/2);
    snd_pcm_sw_params_set_stop_threshold(playback_handle, sw_params, buffer_size);
    snd_pcm_sw_params_set_avail_min(playback_handle, sw_params, period_size);
    snd_pcm_sw_params(playback_handle, sw_params);
    
    return true;
}

/**
 * 设备重连
 */
bool PCMPlayer::reconnect() {
    if (playback_handle) {
        snd_pcm_drop(playback_handle);
        snd_pcm_close(playback_handle);
        playback_handle = nullptr;
    }
    usleep(50000); 
    return init_alsa();
}

/**
 * 准备PCM音频播放设备
 */
void PCMPlayer::prepare() {
    std::lock_guard<std::mutex> lock(alsa_mutex);
    if (!playback_handle) {
        std::cerr << "Playback device not initialized." << std::endl;
        if (!init_alsa()) {
            std::cerr << "Failed to initialize ALSA device in prepare." << std::endl;
            return;
        }
    }

    snd_pcm_state_t state = snd_pcm_state(playback_handle);
    //std::cout << "PCM device state: " << snd_pcm_state_name(state) << std::endl;

    // 根据设备状态进行相应处理
    switch (state) {
        case SND_PCM_STATE_OPEN:
            // 设备已打开但未设置参数，需要初始化
            if (!init_alsa()) {
                std::cerr << "Failed to reinitialize ALSA device." << std::endl;
                return;
            }
            break;
            
        case SND_PCM_STATE_SETUP:
            // 参数已设置，可以直接准备
            break;
            
        case SND_PCM_STATE_PREPARED:
            // 设备已准备就绪，无需再次准备
            //std::cout << "Device already prepared." << std::endl;
            return;
            
        case SND_PCM_STATE_RUNNING:
            // 设备正在运行，先停止
            //std::cout << "Device is running, stopping first." << std::endl;
            snd_pcm_drop(playback_handle);
            break;
            
        case SND_PCM_STATE_XRUN:
            // 欠载状态，尝试恢复
            std::cout << "Device in XRUN state, recovering..." << std::endl;
            if (snd_pcm_recover(playback_handle, -EPIPE, 1) < 0) {
                std::cerr << "Recovery failed, reinitializing..." << std::endl;
                reconnect();
            }
            break;
            
        case SND_PCM_STATE_SUSPENDED:
            // 挂起状态，尝试恢复
            std::cout << "Device suspended, resuming..." << std::endl;
            if (snd_pcm_resume(playback_handle) == -EAGAIN) {
                int resume_retry = 0;
                while (snd_pcm_resume(playback_handle) == -EAGAIN && resume_retry < 10) {
                    usleep(10000); // 10ms
                    resume_retry++;
                }
            }
            // 如果恢复失败，重新连接
            if (snd_pcm_state(playback_handle) == SND_PCM_STATE_SUSPENDED) {
                std::cerr << "Resume failed, reinitializing..." << std::endl;
                reconnect();
            }
            break;
            
        case SND_PCM_STATE_DRAINING:
            // 正在排空，等待完成
            std::cout << "Device is draining, waiting..." << std::endl;
            usleep(100000); // 等待100ms
            break;
            
        default:
            // 其他状态，尝试重置设备
            std::cout << "Unknown device state, resetting..." << std::endl;
            snd_pcm_drop(playback_handle);
            break;
    }

    // 准备设备，增加重试机制
    int retry_count = 0;
    while (retry_count < 3) {
        int err = snd_pcm_prepare(playback_handle);
        if (err >= 0) {
            //std::cout << "PCM device prepared successfully." << std::endl;
            return;
        }
        
        std::cerr << "Prepare attempt " << (retry_count + 1) 
                  << " failed: " << snd_strerror(err) << std::endl;
        
        if (err == -EBUSY) {
            // 设备繁忙，等待后重试
            usleep(50000); // 50ms
            retry_count++;
        } else {
            // 其他错误，尝试重新连接
            std::cerr << "Prepare error, reconnecting..." << std::endl;
            if (reconnect()) {
                // 重新连接后再次尝试准备
                err = snd_pcm_prepare(playback_handle);
                if (err >= 0) {
                    std::cout << "PCM device prepared after reconnection." << std::endl;
                    return;
                }
            }
            break;
        }
    }
    std::cerr << "Failed to prepare PCM device after " << retry_count << " attempts." << std::endl;
}

/**
 * 播放pcm音频段数据
 */
void PCMPlayer::play_pcm(const char* audio_data, int len, int dts) {
    std::lock_guard<std::mutex> lock(alsa_mutex);
    if (!playback_handle) {
        std::cerr << "Playback handle is null, attempting to reconnect..." << std::endl;
        if (!reconnect()) {
            std::cerr << "Failed to reconnect, cannot play audio." << std::endl;
            return;
        }
    }

    switch (dts) {
        case 0: // 音频开始
            write_audio(audio_data, len);
            break;
            
        case 1: // 音频中间块
            write_audio(audio_data, len);
            break;
            
        case 2: // 音频结束
            write_audio(audio_data, len);
            break;

        case 3: // 独立音频,合成短文本时出现
            write_audio(audio_data, len);
            break;
    }
}

/**
 * 写入播放数据
 */
void PCMPlayer::write_audio(const char* data, int len) {
    snd_pcm_uframes_t frames = len / (channels * snd_pcm_format_width(format)/8);
    int err;
    
    if ((err = snd_pcm_writei(playback_handle, data, frames)) < 0) {
        //std::cerr << "Write error: " << snd_strerror(err) << std::endl;
        if (err == -EPIPE) {
            std::cerr << "Underrun occurred, recovering..." << std::endl;
            if (snd_pcm_recover(playback_handle, err, 0) < 0) {
                std::cerr << "Recovery failed, preparing device..." << std::endl;
                snd_pcm_prepare(playback_handle);
            } else {
                 std::cout << "Recovery Success." << std::endl;
            }
        } else if (err == -ESTRPIPE) { 
            std::cerr << "Stream suspended, resuming..." << std::endl;
            while ((err = snd_pcm_resume(playback_handle)) == -EAGAIN) {
                usleep(1000); 
            }
            if (err < 0) {
                std::cerr << "Resume failed, preparing device..." << std::endl;
                snd_pcm_prepare(playback_handle);
            }
        } else { 
            std::cerr << "Unknown error, preparing device..." << std::endl;
            if (reconnect()) {
                std::cout << "PCMPlayer: 重连成功，重新尝试写入数据" << std::endl;
                if ((err = snd_pcm_writei(playback_handle, data, frames)) < 0) {
                    std::cerr << "重连后仍然写入失败: " << snd_strerror(err) << std::endl;
                }
            } else {
                std::cerr << "PCMPlayer: 重连失败，无法继续播放" << std::endl;
            }
        }
    }
}