/* user_interface_h */
#include <iostream>
#include <sstream>
#include <string>
#include "asr_offline_record_sample.h"
/***************************参数配置区域，用户可通过修改这些词来进行 ********************************/
#define whether_print_log 0 //是否打印log
#define TIMEOUT 10 			//在客户端获取服务端结果时，超时时间

/******麦克风基础功能参数******/
int PCM_MSG_LEN = 1024; //在录音时会发布音频流,单次发布大小为2048B

//录音文件保存的地址,最好设置为绝对地址
const std::string ORIGINAL_SOUND_PATH = "/audio/vvui_ori.pcm";
const std::string DENOISE_SOUND_PATH = "/audio/vvui_deno.pcm";
//资源文件存储地址
const std::string SYSTEM_PATH = "/tmp/system.tar";
const std::string SYSTEM_CONFIG_PATH = "/tmp/config.txt";

/******与离线命令词识别相关参数******/
std::string source_path = "/home/wheeltec/wheeltec_ros2/src/wheeltec_mic/wheeltec_mic_ros2";
std::string appid ;
std::string denoise_sound_path;
const std::string BEGIN_PREFIX = "fo|";
const std::string ASR_RES_PATH = "/config/msc/res/asr/common.jet";		//离线语法识别资源路径，重要，与麦克风及appid绑定
const std::string GRM_BUILD_PATH = "/config/msc/res/asr/GrmBuilld";  	//构建离线语法识别网络生成数据保存路径
const std::string GRM_FILE = "/config/call.bnf";						//构建离线识别语法网络所用的语法文件，用户自修改文件
const std::string LEX_NAME = "contact";
char *APPID = (char*)" ";






