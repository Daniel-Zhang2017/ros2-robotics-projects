#ifndef SRC_TEST_ALSA_AUDIOPROVIDER_H_
#define SRC_TEST_ALSA_AUDIOPROVIDER_H_

#include <alsa/asoundlib.h>
#include <assert.h>
#include <string>
#include <iostream>
#include <samplerate.h>

extern std::string RECORD_DEVICE_NAME;

namespace aiui_v2 {

// 重采样数据结构
struct ResampleData {
    SRC_STATE* src_state;           // 重采样状态
    float* src_float;               // 输入浮点缓冲区
    float* dst_float;               // 输出浮点缓冲区
    char* dst_buf;                  // 重采样输出缓冲区
    unsigned int src_rate;          // 源采样率
    unsigned int dst_rate;          // 目标采样率
    double ratio;                   // 重采样比例
    snd_pcm_uframes_t dst_frames;   // 重采样后的帧数
    size_t dst_size;                // 重采样缓冲区大小
};
 
// 录音音频数据结构
typedef struct _record_data
{
	snd_pcm_t *handle;
	char *buf;
	int size;
	snd_pcm_uframes_t frames;
}	RecordData;

class AudioProvider
{
private :
	static const std::string TAG ;

	//snd_pcm_t* handle;

	unsigned int retries;

	unsigned int retryDelay;

	unsigned int chanel;

	unsigned int sampleRate;

	const char* deviceName;

	RecordData recordData;

	ResampleData resampleData;

	size_t currentDataSize;

	void initResampler(unsigned int sourceRate); 

	int pcm_read(snd_pcm_t *handle, char *buf);

	int resampleAudio(const short* input, int input_frames, short* output, int channels);

public:
	AudioProvider();

	const char* startRecord();

	void stopRecord();

	void resetDevice();

	virtual ~AudioProvider();

	size_t getCurrentDataSize() const { return currentDataSize; }
	
};
}

#endif /* SRC_TEST_ALSA_AUDIOPROVIDER_H_ */
