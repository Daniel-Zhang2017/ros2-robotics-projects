#ifndef	 __RECORD_H__
#define	 __RECORD_H__ 

#include <alsa/asoundlib.h>

#define RET_SUCCESS   0 
//#define RECORD_DEVICE_NAME   "hw:CARD=L6Microphone,DEV=0"  //CARD=L6Microphone,DEV=0/CARD=XFMDPV0018,DEV=0

/******麦克风基础功能参数******/
bool save_pcm_local = true;   //保存音频到本地.
int max_pcm_size = 10240000;  //最大为10M,超过10M后自动删除,以节省空间.
int buffer_frames = 512;

int write_first_data;

#ifdef __cplusplus
extern "C"
{
#endif

int init_success;
int init_rec;
int record_finish;

//录制音频参数集合
typedef struct{
   snd_pcm_t *pcm;                  //pcm handle
   snd_pcm_hw_params_t *hw_params;
   snd_pcm_format_t format;         //数据的格式
   snd_pcm_uframes_t chunk_size;    //产生的frame个数
   unsigned int channels;           //声道
   unsigned int rate;               //采样率
   size_t bits_per_sample;          //一个sample包含的bit数
   size_t bits_per_frame;           //一个frame包含的bit数
   size_t chunk_bytes;              //简单理解为1秒产生的pcm数据大小
   unsigned char* buffer;           //用于保存录制数据的缓存buffer
} record_handle_t;

//自定义的结构体参数
typedef struct{
        int format;    		//录制的pcm格式
        int channel;   		//channel个数
        int rate;      		//采样率
        //int duration;  	//录制的时长
        // 如有需要其他参数可以继续添加
} record_params_t;

record_handle_t record;

record_params_t params = 
    {
        2,      //format
        1,      //channel
        16000   //rate
    };

#ifdef __cplusplus
}
#endif

#endif
