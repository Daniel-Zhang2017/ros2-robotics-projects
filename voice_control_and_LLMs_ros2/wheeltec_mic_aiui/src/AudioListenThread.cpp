#include "AudioListenThread.h"
#include <unistd.h>

#define AIUI_SLEEP(x) usleep(x * 1000);

using namespace aiui_v2;

AudioListenThread::AudioListenThread(IAIUIAgent* agent)
    : mAIUIAgent(agent), mRun(true), thread_created(false)
{
	mAudioProvider = std::make_unique<AudioProvider>();
}

AudioListenThread::~AudioListenThread( )
{
	stopRun();
}

void AudioListenThread::stopRun()
{
	if (thread_created) {
	mRun = false;
    if (thread_.joinable()) {
        thread_.join();
    }
	thread_created = false;
	}
}

bool AudioListenThread::run()
{
	if (thread_created == false) {
		mRun = true;
		thread_ = std::thread(&AudioListenThread::threadProc, this);
		thread_created = true;
		return true;
	}
	return false;
}

bool AudioListenThread::threadLoop()
{	
    if (!mRun) {
        return false;
    }

	const char* audioBuffer = mAudioProvider->startRecord();
	int actualLen = mAudioProvider->getCurrentDataSize();

	AIUIBuffer frameData = aiui_create_buffer_from_data(audioBuffer, actualLen);

	IAIUIMessage * writeMsg = IAIUIMessage::create(AIUIConstant::CMD_WRITE,
			0,0, "data_type=audio,sample_rate=16000", frameData);
	if (NULL != mAIUIAgent)
	{
		mAIUIAgent->sendMessage(writeMsg);
		std::cout << "录音中......" << std::endl;
		//std::cout << "write audio data." << std::endl;

	}

	AIUI_SLEEP(120);
	return mRun;
}

void AudioListenThread::threadProc() {
    while (mRun) {
        if (!threadLoop()) {
            break;
        }
    }
}