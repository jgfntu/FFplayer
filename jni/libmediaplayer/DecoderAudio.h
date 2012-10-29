#ifndef __DECODER_AUDIO_H
#define __DECODER_AUDIO_H

extern "C" {
#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"
#include "libavutil/avutil.h"
}
#include "IDecoder.h"
#include "BuddyRunnable.h"
#include "PacketQueue.h"
#include "Thread.h"
namespace ffplayer {

typedef void (*AudioDecodingHandler)(int16_t*, int);

class DecoderAudio: public BuddyRunnable, public IDecoder {
	using BuddyRunnable::BuddyType;
	using BuddyRunnable::BuddyEvent;
public:
	int skipTimes;
	DecoderAudio(AVStream* stream, PacketQueue *queue, Thread *loopThread);
	~DecoderAudio();
	AudioDecodingHandler rendorHook;
	// interface of BuddyRunnable
	BuddyType getBuddyType();
	int onBuddyEvent(BuddyRunnable *buddy, BuddyEvent evt);
	int getGCFlags();
	void run(void *ptr);
	int bindBuddy(BuddyRunnable *buddy);
	void onInitialize();
	void onFinalize();

	// interface of IDecoder
	virtual void flush();
	virtual int stop();
	virtual int start();
private:

	double mTimer;
	int16_t* mSamples;
	int mSamplesSize;
	BuddyRunnable *mBuddy;
	PacketQueue* mQueue;
	AVStream* mStream;
	Thread* mLoopThread;
	double mAudioClock;
	double mLastClock;
	int64_t mLastAbsTime;
	inline int bytesPerSample(enum SampleFormat sample_fmt);
	int decodeRender();
	int64_t now(void);
};
}
#endif //__DECODER_AUDIO_H
