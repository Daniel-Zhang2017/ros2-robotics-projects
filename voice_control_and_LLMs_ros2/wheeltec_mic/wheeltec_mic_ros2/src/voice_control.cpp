/****************************************************************/
/* Copyright (c) 2023 WHEELTEC Technology, Inc   				*/
/* function:Speech recognition processing						*/
/* 功能：语音识别处理												*/
/****************************************************************/
#include "voice_control.h"

int FileSize(const char *fname)
{
    struct stat statbuf;
    if (stat(fname, &statbuf) == 0)
        return statbuf.st_size;
    return -1;
}

/************************************************
Function: Example Initialize recording parameters
功能: 初始化录音参数
*************************************************/
int SpeechProcess::record_params_init(record_handle_t* pcm_handle,record_params_t* params,const char* device_name)
{
	int err;
	unsigned int buffer_time, period_time, sample_rate;

	if (pcm_handle == NULL)
	{
		return -1;
	}

	if ((err = snd_pcm_open(&(pcm_handle->pcm),device_name,SND_PCM_STREAM_CAPTURE,0))< 0)
	{
		cout << "无法打开音频设备:" << device_name << "("<< snd_strerror (err) <<")"<<endl;
		exit(1);
	}

	/*参数结构体，可用于指定PCM流的配置*/
	snd_pcm_hw_params_t *hwparams; 

	/*分配硬件参数结构对象，并判断是否分配成功*/
	snd_pcm_hw_params_alloca(&hwparams);

	/*对硬件对象进行初始化默认设置*/
    if((err = snd_pcm_hw_params_any(pcm_handle->pcm,hwparams)) < 0)
    {
    	cout << "初始化参数结构失败:" << "("<< snd_strerror (err) <<")"<<endl;
        goto Init_fail;
    }

 	/*
    	设置数据为交叉模式(降噪板默认输出单通道PCM)
    	INTERLEAVED/NONINTERLEAVED:交叉/非交叉模式。
    	表示在多声道数据传输的过程中是采样交叉的模式还是非交叉的模式。
    	对多声道数据，如果采样交叉模式，使用一块buffer即可，其中各声道的数据交叉传输；
	如果使用非交叉模式，需要为各声道分别分配一个buffer，各声道数据分别传输。
	*/
    if ((err = snd_pcm_hw_params_set_access(pcm_handle->pcm,hwparams,SND_PCM_ACCESS_RW_INTERLEAVED)) < 0)
    {
    	cout << "访问类型设置失败:" << "("<< snd_strerror (err) <<")"<<endl;
        goto Init_fail;
    }

    /*获取格式设置，并设置pcm数据格式*/
    pcm_handle->format = get_formattype_from_params(params);
    if ((err = snd_pcm_hw_params_set_format(pcm_handle->pcm,hwparams,pcm_handle->format)) < 0)
    {
    	cout << "设置PCM数据格式失败:" << "("<< snd_strerror (err) <<")"<<endl;
        goto Init_fail;
    }

    /*获取声道并设置*/
    if ((err = snd_pcm_hw_params_set_channels(pcm_handle->pcm,hwparams,params->channel)) < 0)
    {
    	cout << "channel设置失败:" << "("<< snd_strerror (err) <<")"<<endl;
        goto Init_fail;
    }

    /*获取采样率并设置*/
    sample_rate = params->rate;
	if ((err = snd_pcm_hw_params_set_rate_near(pcm_handle->pcm, hwparams, &sample_rate, 0)) < 0) {
	    cout << "采样率设置失败:" << "(" << snd_strerror(err) << ")" << endl;
	    goto Init_fail;
	}
	pcm_handle->rate = sample_rate;

    //设置周期数
    if ((err = snd_pcm_hw_params_get_buffer_time_max(hwparams,&buffer_time,0)) < 0)
    {
        printf("snd_pcm_hw_params_get_buffer_time_max fail.\n");
        goto Init_fail;
    }
    if (buffer_time > 500000){
        buffer_time = 500000;
    }
    period_time = buffer_time / 4;
    if ((err = snd_pcm_hw_params_set_buffer_time_near(pcm_handle->pcm,hwparams,&buffer_time,0)) < 0)
    {
        printf("snd_pcm_hw_params_set_buffer_time_near fail.\n");
        goto Init_fail;
    }
     if ((err = snd_pcm_hw_params_set_period_time_near(pcm_handle->pcm,hwparams,&period_time,0)) < 0)
    {
        printf("snd_pcm_hw_params_set_period_time_near fail.\n");
        goto Init_fail;
    }

    /*将配置写入驱动程序*/
    if ((err = snd_pcm_hw_params(pcm_handle->pcm,hwparams)) < 0)
    {
    	cout << "写入驱动程序设置参数失败:" << "("<< snd_strerror (err) <<")"<<endl;
        goto Init_fail;
    }

    /*准备音频接口*/
    if ((err = snd_pcm_prepare(pcm_handle->pcm)) < 0)
    {
    	cout << "无法使用音频接口:" << "("<< snd_strerror (err) <<")"<<endl;
        goto Init_fail;
    }

    //获取相关参数
    snd_pcm_uframes_t buffer_size;
    snd_pcm_hw_params_get_period_size(hwparams,&(pcm_handle->chunk_size),0);
    snd_pcm_hw_params_get_buffer_size(hwparams,&buffer_size);
    if (pcm_handle->chunk_size == buffer_size)
     {
        printf("Can't use period equal to buffer size (%lu == %lu).\n",pcm_handle->chunk_size,buffer_size);
        goto Init_fail;
    }

	/*配置一个数据缓冲区用来缓冲数据*/
    pcm_handle->bits_per_sample = snd_pcm_format_width(pcm_handle->format)/8;
    pcm_handle->bits_per_frame = pcm_handle->bits_per_sample*params->channel;
    pcm_handle->chunk_bytes = pcm_handle->chunk_size*pcm_handle->bits_per_frame;
    pcm_handle->buffer = (unsigned char *)malloc(pcm_handle->chunk_bytes);
    if (!pcm_handle->buffer) 
    { 
    	cout << "Error malloc" <<endl;
        goto Init_fail; 
    }
    // cout << "已初始化录音参数" <<endl;
    return 0;

Init_fail: 
	snd_pcm_close(pcm_handle->pcm);
    return -1; 
}

/**************************************
Function: Resample audio using libsamplerate
功能: 重采样音频数据
***************************************/
int SpeechProcess::resample_audio(const short *input, int input_samples, 
                   short **output, int *output_samples, 
                   int channels, double ratio) {
    if (!input || input_samples <= 0 || channels <= 0 || ratio <= 0) {
        printf("Invalid resample parameters\n");
        return -1;
    }
    
    // 添加输入数据检查
    if (input_samples < channels * 100) {  // 至少需要100帧
        printf("Input samples too small: %d\n", input_samples);
        return -1;
    }
    
    SRC_STATE *src_state;
    SRC_DATA src_data;
    int error;
    
    src_state = src_new(SRC_SINC_FASTEST, channels, &error);
    if (!src_state) {
        printf("Error initializing samplerate converter: %s\n", src_strerror(error));
        return -1;
    }
    
    // 更精确的输出缓冲区大小计算
    int max_output_samples = (int)(input_samples * ratio) + channels * 100;
    
    float *input_float = (float*)malloc(input_samples * sizeof(float));
    float *output_float = (float*)malloc(max_output_samples * sizeof(float));
    
    if (!input_float || !output_float) {
        printf("Error allocating float buffers\n");
        free(input_float);
        free(output_float);
        src_delete(src_state);
        return -1;
    }
    
    // 使用更精确的转换
    float scale_factor = 1.0f / 32768.0f;
    for (int i = 0; i < input_samples; i++) {
        input_float[i] = (float)input[i] * scale_factor;
        // 防止溢出
        if (input_float[i] > 1.0f) input_float[i] = 1.0f;
        if (input_float[i] < -1.0f) input_float[i] = -1.0f;
    }
    
    *output = (short*)malloc(max_output_samples * sizeof(short));
    if (!*output) {
        printf("Error allocating output buffer\n");
        free(input_float);
        free(output_float);
        src_delete(src_state);
        return -1;
    }
    
    src_data.data_in = input_float;
    src_data.data_out = output_float;
    src_data.input_frames = input_samples / channels;
    src_data.output_frames = max_output_samples / channels;
    src_data.src_ratio = ratio;
    src_data.end_of_input = 1;  // 标记为输入结束
    
    error = src_process(src_state, &src_data);
    
    if (error) {
        printf("Error during resampling: %s\n", src_strerror(error));
        free(input_float);
        free(output_float);
        free(*output);
        src_delete(src_state);
        return -1;
    }
    
    *output_samples = src_data.output_frames_gen * channels;
    
    // 输出限制
    float out_scale = 32767.0f;
    for (int i = 0; i < *output_samples; i++) {
        float sample = output_float[i] * out_scale;
        if (sample > 32767.0f) sample = 32767.0f;
        if (sample < -32768.0f) sample = -32768.0f;
        (*output)[i] = (short)sample;
    }
    
    // printf("Resampling: %d samples -> %d samples (ratio: %.3f, frames: %ld -> %ld)\n", 
    //        input_samples, *output_samples, ratio,
    //        src_data.input_frames, src_data.output_frames_gen);
    
    free(input_float);
    free(output_float);
    src_delete(src_state);
    
    return 0;
}

/***********************************************
Function: Initialize offline resource parameters
功能: 初始化离线资源参数
************************************************/
int SpeechProcess::init_asr_params(){
	init_rec = 0;
	init_success = 0;
	write_first_data = 0;

    std::string jet_path = BEGIN_PREFIX + source_path + ASR_RES_PATH;
    std::string grammer_path = source_path + GRM_BUILD_PATH;
    std::string bnf_path = source_path + GRM_FILE;
    denoise_sound_path = source_path + DENOISE_SOUND_PATH;

    APPID = const_cast<char *>(appid.c_str());

	Recognise_Result inital = initial_asr_paramers(
		const_cast<char*>(jet_path.c_str()),
		const_cast<char*>(grammer_path.c_str()), 
		const_cast<char*>(bnf_path.c_str()), 
		const_cast<char*>(LEX_NAME.c_str()));
	if (!inital.whether_recognised)
	{
		cout <<"fail_reason :" << inital.fail_reason << endl;
		return -1;
	}
	return 0;
}

/**************************************
Function: Get file size
功能: 获取文件大小
***************************************/
int SpeechProcess::filesize(const char *fname)
{
	struct stat statbuf;
    if (stat(fname, &statbuf) == 0)
        return statbuf.st_size;
    return -1;
}

/**************************************
Function: Text encoding conversion
功能: 文本编码转换
***************************************/

std::string SpeechProcess::s2s(const std::string &str)
{
	using convert_typeX =  std::codecvt_utf8<wchar_t>;
	std::wstring_convert<convert_typeX, wchar_t> converterX;
	std::wstring wstr = converterX.from_bytes(str);
	return converterX.to_bytes(wstr);
}

/**************************************
Function: Audio format selection
功能: 音频格式选择
***************************************/
snd_pcm_format_t SpeechProcess::get_formattype_from_params(record_params_t* params)
{
    if(params!=NULL){  
        switch (params->format) {  
        case 0:
            return SND_PCM_FORMAT_S8;
        case 1:
            return SND_PCM_FORMAT_U8;
        case 2:
            return SND_PCM_FORMAT_S16_LE;
        case 3:
            return SND_PCM_FORMAT_S16_BE;
        default:  return SND_PCM_FORMAT_S16_LE;
        }
    }
    return SND_PCM_FORMAT_S16_LE;
}

/**************************************
Function: Text encoding conversion
功能: 送入音频进行识别
***************************************/
int SpeechProcess::business_data_t(unsigned char* record)
{
    record_data = record;
    if (!init_success && init_rec)
    {
    	int len = 3*PCM_MSG_LEN;
        char *pcm_buffer = new char[len];
        if (NULL == pcm_buffer) {
            cout << ">>>>>buffer is null" << endl;
            return -1;
        }
        memcpy(pcm_buffer, record_data, len);

        if (write_first_data++ == 0) {
#if whether_print_log
        	cout <<"***************write the first voice**********" <<endl;
#endif
            demo_xf_mic(pcm_buffer, len, 1);
        } else {
#if whether_print_log
        	cout <<"***************write the middle voice**********" <<endl;
#endif
            demo_xf_mic(pcm_buffer, len, 2);
        }

        if (whether_finised) {
            delete[] pcm_buffer;
            record_finish = 1;
            whether_finised = 0;
        }
    }
    return 0;
}

/**************************************
Function: Get audio data
功能: 获取音频
***************************************/
int SpeechProcess::get_record_sound(const char *fname)
{
    int ret;
    const char *filename = fname;
    int fd = open(filename, O_WRONLY | O_CREAT | O_APPEND, S_IRUSR | S_IWUSR);
    if (fd == -1)
    {
        cout << "无法创建音频文件" <<endl;
        exit(1);
    }
    
    // 使用 fdopen 创建 FILE* 指针
    FILE *pcm_file = fdopen(fd, "a");
    if (!pcm_file) {
        cout << "无法打开音频文件流" << endl;
        close(fd);
        exit(1);
    }
    
    init_success = record_params_init(&record, &params, record_device_name.c_str());
    if (init_success != RET_SUCCESS)
    {
        cout << "音频初始化失败!" <<endl;
        fclose(pcm_file);
        close(fd);
        exit(1);
    }

    // 检查是否需要重采样
    bool need_resample = (record.rate != 16000);
    double ratio = 16000.0 / (double)record.rate;
    // if (need_resample) {
    //     printf("需要重采样: %.6f (%uHz -> 16000Hz)\n", ratio, record.rate);
    // } else {
    //     printf("采样率已是16000Hz，跳过重采样处理\n");
    // }
    cout<<endl;
    cout<<">>>>>开始一次语音识别！"<<endl;
    
    while (init_success == RET_SUCCESS) {
        ret = snd_pcm_readi(record.pcm, record.buffer, record.chunk_size);
        if (ret == -EAGAIN) {
            snd_pcm_wait(record.pcm, 1000);
            continue;
        }
        else if (ret == -EPIPE) {
            snd_pcm_prepare(record.pcm);
            printf("snd_pcm_readi return EPIPE, recovered.\n");
            continue;
        }
        else if (ret == -ESTRPIPE) {
            printf("snd_pcm_readi return ESTRPIPE.\n");
            break;
        }
        else if (ret < 0) {
            printf("snd_pcm_readi return fail: %d\n", ret);
            break;
        }
        else if (ret != record.chunk_size) {
            printf("读取数据不完整: %d/%lu\n", ret, record.chunk_size);
        }

        if (ret > 0) {
            init_rec = 1;

            short *process_data = NULL;
            int process_samples = 0;
            int input_samples = ret * params.channel;

            if (need_resample) {
                if (resample_audio((short*)record.buffer, input_samples, 
                                  &process_data, &process_samples, 
                                  params.channel, ratio) == 0) {
                     //printf("重采样成功: %d -> %d 样本\n", input_samples, resampled_samples);
                    if (process_samples > 0) {
                        if (save_pcm_local) {
                            if (-1 != FileSize(filename)) {
                                int file_size = FileSize(filename);
                                if (file_size > max_pcm_size) {
                                    freopen(filename, "w", pcm_file);
                                }
                            }
                            size_t written = fwrite(process_data, 
                                                  sizeof(short), 
                                                  process_samples, 
                                                  pcm_file);
                            fflush(pcm_file);
                        }
                        business_data_t((unsigned char*)process_data);
                    }
                    free(process_data);
                } else {
                    printf("重采样失败，使用原始数据\n");
                    if (save_pcm_local) {
                        fwrite(record.buffer, record.bits_per_sample, ret * params.channel, pcm_file);
                    }
                    business_data_t(record.buffer);
                }
            } else {
                // 无需重采样，直接使用原始数据
                process_data = (short*)record.buffer;
                process_samples = input_samples;
                
                if (save_pcm_local) {
                    if (-1 != FileSize(filename)) {
                        int file_size = FileSize(filename);
                        if (file_size > max_pcm_size) {
                            freopen(filename, "w", pcm_file);
                        }
                    }
                    size_t written = fwrite(process_data, 
                                          sizeof(short), 
                                          process_samples, 
                                          pcm_file);
                    fflush(pcm_file);
                }
                business_data_t((unsigned char*)process_data);
            }
        }
        if (record_finish) break;
        std::this_thread::sleep_for(std::chrono::milliseconds(5));
    }
    init_rec = 0;
    fclose(pcm_file);
    close(fd);
    finish_record_sound();
    return 0;
}

/**************************************
Function: Finish recording audio
功能: 结束录制音频
***************************************/
int SpeechProcess::finish_record_sound()
{
	if(record.buffer != NULL) free(record.buffer);
    if(!init_success) snd_pcm_close(record.pcm);
    printf(">>>>>停止录音........\n"); 
}

Effective_Result SpeechProcess::show_result(char *str)
{
    Effective_Result current;
    current.effective_confidence = 0;
    strcpy(current.effective_word, " ");
    
    // 添加空指针和长度检查
    if (str == nullptr || strlen(str) < 50) {
        return current;
    }

    std::string result_str(str);
    size_t rawtext_start = result_str.find("<rawtext>");
    size_t rawtext_end = result_str.find("</rawtext>");
    size_t confidence_start = result_str.find("<confidence>");
    size_t confidence_end = result_str.find("</confidence>");
 
 	// 检查所有标签是否都存在   
    if (rawtext_start == std::string::npos || rawtext_end == std::string::npos ||
        confidence_start == std::string::npos || confidence_end == std::string::npos) {
        return current;
    }

    // 提取置信度
    if (confidence_start + 12 < confidence_end) {
        std::string conf_str = result_str.substr(confidence_start + 12, confidence_end - (confidence_start + 12));
        try {
            current.effective_confidence = std::stoi(conf_str);
        } catch (...) {
            current.effective_confidence = 0;
        }
    }

    // 提取识别结果
    if (current.effective_confidence >= confidence && 
        rawtext_start + 9 < rawtext_end) {
        std::string word_str = result_str.substr(rawtext_start + 9, rawtext_end - (rawtext_start + 9));
        if (word_str.length() < sizeof(current.effective_word)) {
            strncpy(current.effective_word, word_str.c_str(), sizeof(current.effective_word)-1);
            current.effective_word[sizeof(current.effective_word)-1] = '\0';
        }
    }
    return current;
}

/********************************************************
Function: Get the offline command word recognition result
功能: 获取离线命令词识别结果
*********************************************************/
bool SpeechProcess::Get_Offline_Recognise_Result(const std::shared_ptr<wheeltec_mic_msg::srv::GetOfflineResult::Request>& request,
							std::shared_ptr<wheeltec_mic_msg::srv::GetOfflineResult::Response>& response){
	if (request->offline_recognise_start)
	{
        if (device_type == "M07") {
            // 如果播报尚未开始，等待播报完成
            if (!g_play_state::isFinished()) {
                
                int wait_ms = 0;
                const int max_wait_ms = 3000;
                const int sleep_ms = 20;
                
                while (wait_ms < max_wait_ms) {
                    if (g_play_state::isFinished()) {
                        RCLCPP_INFO(this->get_logger(), "播报已完成");
                        break;
                    }
                    std::this_thread::sleep_for(std::chrono::milliseconds(sleep_ms));
                    wait_ms += sleep_ms;
                }
                
                if (wait_ms >= max_wait_ms) {
                    //RCLCPP_WARN(this->get_logger(), "等待播报超时");
                }
            } else {
                //RCLCPP_INFO(this->get_logger(), "播报已完成，直接开始识别");
            }
        }
		whether_finised = 0;
		record_finish = 0;
		time_per_order = request->time_per_order;
		confidence = request->confidence_threshold;
		int ret = create_asr_engine(&asr_data);
		if (MSP_SUCCESS != ret)
		{
			cout<<"创建语音识别引擎失败！"<<endl;
			return false;
		}

		get_record_sound(denoise_sound_path.c_str());

		if (whole_result != "")
		{
			Effective_Result effective_ans = show_result(whole_result);
			if (effective_ans.effective_confidence >= confidence)
			{
				cout<<">>>>>是否识别成功: 是 " <<endl;
				cout<<">>>>>关键字的置信度: [" << effective_ans.effective_confidence << "] " <<endl;
				cout<<">>>>>关键字识别结果: [" << effective_ans.effective_word << "] " <<endl;

				response->result = "ok";
				response->fail_reason = "";
				std::string txt_uft8 = s2s(effective_ans.effective_word);
				response->text = txt_uft8;

				std_msgs::msg::String msg;
				msg.data = effective_ans.effective_word;
				voice_words_pub->publish(msg);			
			}
			else
			{
				cout<<">>>>>是否识别成功: 否 " <<endl;
				cout<<">>>>>关键字的置信度: [" << effective_ans.effective_confidence << "] " <<endl;
				cout<<">>>>>关键字置信度较低，文本不予显示" <<endl;

				response->result = "fail";
				response->fail_reason = "low_confidence error or 11212_license_expired_error";
				response->text = " ";
			}
		}
		else
		{
			response->result = "fail";
			response->fail_reason = "no_valid_sound error";
			response->text = " ";
			cout<<">>>>>未能检测到有效声音,请重试" <<endl;
		}
		whole_result = "";
		/*[1-3]语音识别结束]*/
		delete_asr_engine();
		write_first_data = 0;
		sleep(1.0);
	}
	cout<<endl;
	return true;
}

void SpeechProcess::run()
{
    rclcpp::executors::MultiThreadedExecutor executor;
    executor.add_node(shared_from_this());
    executor.spin();
}

SpeechProcess::SpeechProcess(const std::string &node_name) 
: rclcpp::Node(node_name){
	/***声明参数并获取***/
	this->declare_parameter<string>("appid","5fa0b8b9");
	this->get_parameter("appid",appid);
	this->declare_parameter<string>("record_device_name", "default");
	this->get_parameter("record_device_name", record_device_name);
	this->declare_parameter<std::string>("device_type", "default");
	this->get_parameter("device_type", device_type);
	/***识别命令词话题发布者创建***/
	voice_words_pub = this->create_publisher<std_msgs::msg::String>("voice_words",10);

	get_offline_result_srv_ = this->create_service<wheeltec_mic_msg::srv::GetOfflineResult>(
		"get_offline_result_srv",[this](const std::shared_ptr<wheeltec_mic_msg::srv::GetOfflineResult::Request> request,
									std::shared_ptr<wheeltec_mic_msg::srv::GetOfflineResult::Response> response){
									Get_Offline_Recognise_Result(request,response);
		});

	int ret = init_asr_params();
	if(ret == RET_SUCCESS)
	{
		RCLCPP_INFO(this->get_logger(),"Initialization Offline resource parameter success!");
	}

    g_play_state::init();

	timeout_thread_ = std::thread(&SpeechProcess::timeoutCheckLoop, this);
}

/********************************************************
Function: Calculates whether recording times out
功能: 检测录音是否超时
*********************************************************/
void SpeechProcess::timeoutCheckLoop()
{
    rclcpp::Time start_time;
    bool recording = false;
    
    while(!stop_timeout_check_) {   
        if (init_rec && !recording) {
            start_time = rclcpp::Node::now();
            recording = true;
        }
        
        if (recording && init_rec && whether_finised != 1) {
            auto current_time = rclcpp::Node::now();
            if ((current_time - start_time).seconds() > time_per_order) {
                cout << ">>>>>>超出离线命令词最长识别时间" << endl;
                whether_finised = 1;
                recording = false;
                record_finish = 1;
            }
        } else if (!init_rec) {
            recording = false;
        }
        
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
}

SpeechProcess::~SpeechProcess()
{
    stop_timeout_check_ = true;
    if (timeout_thread_.joinable()) {
        timeout_thread_.join();
    }

	RCLCPP_INFO(this->get_logger(),"voice_control node over!\n");
}

// void exit_sighandler(int sig)
// {
// 	record_finish = 1;
// }

int main(int argc, char **argv)
{
	rclcpp::init(argc,argv);
	// /*注册信号捕获退出接口*/
	// signal(2,exit_sighandler);
    atexit([]() {
        SharedMemory::cleanup();
    });
    auto node = std::make_shared<SpeechProcess>("voice_control");
    node->run(); 
  	rclcpp::shutdown();
	return 0;
} 
