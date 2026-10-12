#include "AudioProvider.h"

namespace aiui_v2 {

	const std::string AudioProvider::TAG = "AudioProvider";
	static bool audioDumped = false;

	AudioProvider::AudioProvider()
    	: retries(10), retryDelay(20 * 1000), chanel(1), sampleRate(0), currentDataSize(0)
	{
		snd_pcm_hw_params_t* params;

		int err,i;
		unsigned int buffer_time, period_time;
		deviceName = RECORD_DEVICE_NAME.empty() ? "default" : RECORD_DEVICE_NAME.c_str();
		for (i = 0; i < retries; ++i)
		{
			if (snd_pcm_open(&recordData.handle,  deviceName, SND_PCM_STREAM_CAPTURE, 0) == 0)
			{
				//std::cout << "Open device success. " << " DeviceName: " << deviceName <<std::endl;
				break;
			} else {
	        std::cerr << "snd_pcm_open failed (attempt " << i+1 << "): " << snd_strerror(err) << std::endl;
	        usleep(retryDelay);
	    	}
		}
		assert( i < retries);

		snd_pcm_hw_params_alloca(&params);
		err = snd_pcm_hw_params_any(recordData.handle, params);
		if (err < 0)
		{
			fprintf(stderr, "无法初始化硬件参数: %s\n", snd_strerror(err));
		}
		err = snd_pcm_hw_params_set_access(recordData.handle, params, SND_PCM_ACCESS_RW_INTERLEAVED);
		if (err < 0)
		{
			fprintf(stderr, "无法设置访问模式: %s\n", snd_strerror(err));
		}
		assert(snd_pcm_hw_params_set_format(recordData.handle, params, SND_PCM_FORMAT_S16_LE) == 0);
		assert(snd_pcm_hw_params_set_channels(recordData.handle, params, chanel) == 0);
		unsigned int actual_rate = 0;
		actual_rate = 16000;
		err = snd_pcm_hw_params_set_rate_near(recordData.handle, params, &actual_rate, 0);
		if (err < 0) {
			std::cerr << "设置16000Hz失败，尝试其他采样率" << std::endl;
			err = snd_pcm_hw_params_set_rate_near(recordData.handle, params, &actual_rate, 0);
		}
		
		// 获取实际设置的采样率
		err = snd_pcm_hw_params_get_rate(params, &actual_rate, 0);
		if (err >= 0) {
			sampleRate = actual_rate;
		} else {
			sampleRate = 16000; // 默认值
			std::cerr << "无法获取采样率，使用默认值: " << sampleRate << "Hz" << std::endl;
		}

	    // 获取最大缓冲区时间
	    err = snd_pcm_hw_params_get_buffer_time_max(params, &buffer_time, 0);
	    if (err < 0) {
	        fprintf(stderr, "无法获取最大缓冲区时间: %s\n", snd_strerror(err));
	    }
	    if (buffer_time > 500000) {
	        buffer_time = 500000;
	    }
	    // 设置缓冲区时间
	    err = snd_pcm_hw_params_set_buffer_time_near(recordData.handle, params, &buffer_time, 0);
	    if (err < 0) {
	        fprintf(stderr, "无法设置缓冲区时间: %s\n", snd_strerror(err));
	    }

	    // 设置周期时间
	    period_time = buffer_time / 4;
	    err = snd_pcm_hw_params_set_period_time_near(recordData.handle, params, &period_time, 0);
	    if (err < 0) {
	        fprintf(stderr, "无法设置周期时间: %s\n", snd_strerror(err));
	    }

	    if ((err = snd_pcm_hw_params(recordData.handle,params)) < 0)
	    {
	        printf("写入配置参数失败 : [%s]\n",snd_strerror(err));
	    }

	    // 获取周期大小
	    err = snd_pcm_hw_params_get_period_size(params, &recordData.frames, 0);
	    if (err < 0) {
	        fprintf(stderr, "无法获取周期大小: %s\n", snd_strerror(err));
	    }

	    // 获取缓冲区大小
	    snd_pcm_uframes_t final_buffer_size;
	    err = snd_pcm_hw_params_get_buffer_size(params, &final_buffer_size);
	    if (err < 0) {
	        fprintf(stderr, "无法获取缓冲区大小: %s\n", snd_strerror(err));
	    }

	    recordData.size = recordData.frames * 2 * chanel;
		recordData.buf = (char*)malloc(recordData.size);
		if (recordData.buf == NULL)
		{
			std::cout << "pcm read buf failed!" <<std::endl;
		}

		// 初始化重采样相关参数
		initResampler(sampleRate);
		//std::cout << "AudioProvider finish"<<std::endl;
	}

	// 初始化重采样器
	void AudioProvider::initResampler(unsigned int sourceRate)
	{
	    resampleData.src_rate = sourceRate;
	    resampleData.dst_rate = 16000;
	    
	    if (sourceRate == 16000) {
	        resampleData.ratio = 1.0;
	        resampleData.dst_buf = nullptr;
	        return;
	    }
	    
	    resampleData.ratio = (double)resampleData.dst_rate / (double)resampleData.src_rate;
	    
	    // 计算重采样后的帧数，增加安全边距
	    resampleData.dst_frames = (snd_pcm_uframes_t)(recordData.frames * resampleData.ratio) + 100;
	    resampleData.dst_size = resampleData.dst_frames * 2 * chanel;
	    
	    resampleData.dst_buf = (char*)malloc(resampleData.dst_size);
	    
	    int error;
	    
	    resampleData.src_state = src_new(SRC_SINC_MEDIUM_QUALITY, chanel, &error);
	    
	    if (resampleData.src_state == NULL)
	    {
	        resampleData.src_state = src_new(SRC_SINC_FASTEST, chanel, &error);  
	    }
	    
	    // 分配缓冲区
	    resampleData.src_float = (float*)malloc(recordData.frames * chanel * sizeof(float));
	    resampleData.dst_float = (float*)malloc(resampleData.dst_frames * chanel * sizeof(float));
	    
	    // 清空重采样器状态
	    if (resampleData.src_state) {
	        src_reset(resampleData.src_state);
	    }
	}

	//重采样函数
	int AudioProvider::resampleAudio(const short* input, int input_frames, short* output, int channels)
	{
	    if (resampleData.ratio == 1.0) {
	        memcpy(output, input, input_frames * channels * sizeof(short));
	        return input_frames * channels;
	    }

	    // 转换 short 到 float
	    for (int i = 0; i < input_frames * channels; i++)
	    {
	        resampleData.src_float[i] = (float)input[i] / 32768.0f;
	    }
	    
	    SRC_DATA src_data;
	    src_data.data_in = resampleData.src_float;
	    src_data.data_out = resampleData.dst_float;
	    src_data.input_frames = input_frames;
	    src_data.output_frames = resampleData.dst_frames;
	    src_data.src_ratio = resampleData.ratio;
	    src_data.end_of_input = 0;
	    
	    int total_output_frames = 0;
	    int remaining_input_frames = input_frames;
	    float* input_ptr = resampleData.src_float;
	    float* output_ptr = resampleData.dst_float;
	    
	    // 分批处理，确保质量
	    while (remaining_input_frames > 0) {
	        src_data.data_in = input_ptr;
	        src_data.data_out = output_ptr;
	        src_data.input_frames = remaining_input_frames;
	        src_data.output_frames = resampleData.dst_frames - total_output_frames;
	        
	        int error = src_process(resampleData.src_state, &src_data);
	        
	        if (error) {
	            std::cerr << "[Resample] 处理错误: " << src_strerror(error) << std::endl;
	            break;
	        }
	        
	        total_output_frames += src_data.output_frames_gen;
	        input_ptr += src_data.input_frames_used * channels;
	        output_ptr += src_data.output_frames_gen * channels;
	        remaining_input_frames -= src_data.input_frames_used;
	        
	        // 如果没有产生输出或使用了所有输入，退出
	        if (src_data.output_frames_gen == 0 || src_data.input_frames_used == 0) {
	            break;
	        }
	    }
	    
	    // 转换回 short，使用优化的限幅
	    const float scale = 32768.0f;
	    for (int i = 0; i < total_output_frames * channels; i++)
	    {
	        float sample = resampleData.dst_float[i] * scale;
	        // 使用更快的限幅方式
	        output[i] = (short)(sample > 32767.0f ? 32767 : 
	                           (sample < -32768.0f ? -32768 : sample));
	    }
	    
	    return total_output_frames * channels;
	}

	const char* AudioProvider::startRecord()
	{
	    if (NULL != recordData.handle)
	    {
	        int ret = pcm_read(recordData.handle, recordData.buf);
	        if (ret > 0)  // 读取成功
	        {
	            if (resampleData.ratio == 1.0) {
	                currentDataSize = recordData.size;
	                return recordData.buf;
	            }
	            // 执行重采样
	            int resampled_samples = resampleAudio(
	                (short*)recordData.buf, 
	                recordData.frames,
	                (short*)resampleData.dst_buf,
	                chanel
	            );
	            
	            if (resampled_samples > 0)
	            {
	                currentDataSize = resampled_samples * sizeof(short);
	                return resampleData.dst_buf;
	            }
	        }
	        // 读取失败或重采样失败
	        currentDataSize = 0;
	        return nullptr;
	    }

	    std::cerr << "[ERROR] startRecord: handle NULL" << std::endl;
	    return NULL;
	}

	void AudioProvider::stopRecord()
	{
		assert(snd_pcm_close(recordData.handle) == 0);
	}

	AudioProvider::~AudioProvider()
	{
		if ( recordData.handle != NULL) {
			//snd_pcm_drain(handle);
			snd_pcm_drain(recordData.handle);
			snd_pcm_close(recordData.handle);
		}
		recordData.handle = NULL;
		
		// 释放重采样资源
		if (resampleData.src_state != NULL)
		{
		    src_delete(resampleData.src_state);
		    resampleData.src_state = NULL;
		}
		if (resampleData.dst_buf != NULL) {
		    free(resampleData.dst_buf);
		    resampleData.dst_buf = NULL;
		}
		if (resampleData.src_float != NULL) {
		    free(resampleData.src_float);
		    resampleData.src_float = NULL;
		}
		if (resampleData.dst_float != NULL) {
		    free(resampleData.dst_float);
		    resampleData.dst_float = NULL;
		}
		
		if (recordData.buf != NULL) {
		    free(recordData.buf);
		    recordData.buf = NULL;
		}
	}

	void AudioProvider::resetDevice() {
	    if (!recordData.handle) return;
	    snd_pcm_drop(recordData.handle);
	    // 重置并准备
	    snd_pcm_reset(recordData.handle);
	    snd_pcm_prepare(recordData.handle);
	    // 清空缓冲区
	    if (recordData.buf) {
	        memset(recordData.buf, 0, recordData.size);
	    }
	    // 预热一次
	    char dummy_buf[recordData.size];
	    snd_pcm_readi(recordData.handle, dummy_buf, recordData.frames);
	    // 最后重新准备
	    snd_pcm_prepare(recordData.handle);
	}

	int AudioProvider::pcm_read(snd_pcm_t *handle, char *buf)
	{
	    // 读取音频数据
	    int ret = snd_pcm_readi(handle, buf, recordData.frames);
	    if(ret == -EPIPE)
	    {
	        printf("overrun occurred, recovering...\n");
	        snd_pcm_prepare(recordData.handle);
	        // 尝试重读一次
	        ret = snd_pcm_readi(handle, buf, recordData.frames);
	        if (ret > 0) {
	            printf("recovery successful, read %d frames\n", ret);
	            return ret;
	        } else {
	            printf("recovery failed: %s\n", snd_strerror(ret));
	            return -1;
	        }
	    }
	    else if(ret < 0)
	    {
	        printf("error from read: %s\n", snd_strerror(ret));
	        return -1;
	    }
	    else if (ret == -EAGAIN)
	    {
	        snd_pcm_wait(recordData.handle, 1000);
	        printf("snd_pcm_readi return EAGAIN.\n");
	        return -1;
	    }
	    else if (ret == -ESTRPIPE)
	    {
	        printf("snd_pcm_readi return ESTRPIPE.\n");
	        return -1;
	    }
	    else if(ret != (int)recordData.frames)
	    {
	        printf("short read, read %d frames\n", ret);
	        return -1;
	    }
	    else
	        return ret;
	}
}

