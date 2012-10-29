#define LOG_TAG "DecoderAudio"
#include <common/logwrapper.h>
#include "DecoderAudio.h"
#include <unistd.h>
#include "Utils.h"
using ffplayer::BuddyRunnable;

namespace ffplayer {

DecoderAudio::DecoderAudio(AVStream* stream, PacketQueue *queue,
		Thread *loopThread) :
		mStream(stream), mQueue(queue), mLoopThread(loopThread), mTimer(0), mAudioClock(
				0.0) {
}

int64_t DecoderAudio::now(void) {
	struct timeval tv;
	gettimeofday(&tv, NULL);
	return (int64_t) tv.tv_sec * 1000000ll + tv.tv_usec;
}

// state machine operation
int DecoderAudio::start() {
	CHECK_POINTER_INT(mLoopThread, -1);
	mLoopThread->registerRunnable(this);
	mLoopThread->start();
	return RET_SCCESS;
}

int DecoderAudio::stop() {
	CHECK_POINTER_INT(mQueue, -1);
	mQueue->abort();
	CHECK_POINTER_INT(mLoopThread, -1);
	LOGD("waiting on end of decoder thread");
	if (mLoopThread) {
		mLoopThread->stop();
	}
	return RET_SCCESS;
}

DecoderAudio::~DecoderAudio() {
	CHECK_POINTER_VOID(mBuddy);
	CHECK_POINTER_VOID(mLoopThread);
	mLoopThread->quit();
	if (NULL != mQueue) {
		mQueue->flush();
		av_free(mQueue);
		mQueue = NULL;
	}

	CHECK_POINTER_VOID(mStream);
	avcodec_close(mStream->codec);
	if (BuddyRunnable::GC_BY_EXTERNAL == mBuddy-> getGCFlags()
			&& FOLLOWER == mBuddy->getBuddyType()) {
		delete mBuddy;
		mBuddy = NULL;
	}
}

void DecoderAudio::flush() {
	//TODO: more precise, should put a flush pkt
	CHECK_POINTER_VOID(mQueue);
	mQueue->flush();
	CHECK_POINTER_VOID(mStream);
	avcodec_flush_buffers(mStream->codec);
}

BuddyRunnable::BuddyType DecoderAudio::getBuddyType() {
	return INITIATOR;
}

int DecoderAudio::onBuddyEvent(BuddyRunnable *buddy, BuddyEvent evt) {
	return 0;
}

int DecoderAudio::getGCFlags() {
	return BuddyRunnable::GC_BY_EXTERNAL;
}

int DecoderAudio::bindBuddy(BuddyRunnable * buddy) {
	if (mBuddy) {
		return -1;
	} else {
		mBuddy = buddy;
	}
}

void DecoderAudio::onInitialize() {
	mSamplesSize = AVCODEC_MAX_AUDIO_FRAME_SIZE;
	mSamples = (int16_t *) av_malloc(mSamplesSize);

	if (!mSamples) {
		return;
	}
	if (mStream->start_time != AV_NOPTS_VALUE) {
		mAudioClock = mStream->start_time * av_q2d(mStream->time_base);
	} else {
		mAudioClock = 0.0;
	}
	mLastClock = mAudioClock;
	mLastAbsTime = now();
	CHECK_POINTER_VOID(mBuddy);
	mBuddy->onInitialize();
}

void DecoderAudio::onFinalize() {
	CHECK_POINTER_VOID(mBuddy);
	mBuddy->onInitialize();
	//ourself stuff here
}

int DecoderAudio::decodeRender(AVPacket *packet) {
	/* Process some special Events
	 * 1) BOS
	 * 2) EOS
	 */

	if (&BOS_PKT == packet) {
		LOGI("Audio Decoder Received BOS PKT!");
		/*update our clock*/
		packet->pts;
		return 0;
	} else if (&EOS_PKT == packet) {
		LOGI("Audio Decoder Received EOS PKT!");
		return 0;
	}

	if (packet->pts != AV_NOPTS_VALUE) {
		mAudioClock = av_q2d(mStream->time_base) * packet->pts;
	}

	AVCodecContext *dec = mStream->codec;
	CHECK_POINTER_INT(dec, -1);
	int remainBufSize = mSamplesSize, curOutputBufSize = mSamplesSize;
	int dataSize = 0;
	int16_t *outputBuf = mSamples;
	while ((packet->size > 0) && ((curOutputBufSize = remainBufSize) > 0)) {
		int len = avcodec_decode_audio3(mStream->codec, (int16_t *) outputBuf,
				&curOutputBufSize, packet);
		if (len < 0 || (!curOutputBufSize)) {
			packet->size = 0;
			break;
		}
		packet->data += len;
		packet->size -= len;
		remainBufSize -= curOutputBufSize;
		outputBuf += curOutputBufSize / (sizeof(int16_t));
		dataSize += curOutputBufSize;
		LOGD("Audio Decoder Thread Processing");
	}
	double bytesPerSec = dec->channels * bytesPerSample(dec->sample_fmt)
			* dec->sample_rate;
	mAudioClock += (double) dataSize / (double) (bytesPerSec);
	// TODO:
	// update the delay time
	mTimer = mAudioClock * 1e6 - mLastClock * 1e6;
	//call handler for posting buffer to os audio driver
	rendorHook(mSamples, dataSize);
	usleep(mTimer - (now() - mLastAbsTime));
	mLastClock = mAudioClock;
	mLastAbsTime = now();
	CHECK_POINTER_INT(mBuddy, -1);

	BuddyEvent evt;
	evt.type = AV_SYNC;
	evt.data.dData = mAudioClock;

	mBuddy->onBuddyEvent(this, evt);
	return 0;
}

int DecoderAudio::bytesPerSample(enum SampleFormat format) {
	switch (format) {
	case SAMPLE_FMT_U8:
		return 1;
	case SAMPLE_FMT_S16:
		return 2;
	case SAMPLE_FMT_S32:
		return 4;
	default:
		return 2;
	}
}

void DecoderAudio::run(void* ptr) {
	AVPacket pkt;
	LOGD("decoding audio");
	CHECK_POINTER_VOID(mQueue);
	LOGD("AudioQueue get start()");
	if (mQueue->get(&pkt, true) < 0) {
		LOGE("getAudio Packet error, thread exit!");
		return;
	}LOGD("AudioQueue get end()");

	if (decodeRender(&pkt)) {
		av_free_packet(&pkt);
		LOGD("Audio process error, thread exit!");
		return;
	}
	// Free the packet that was allocated by av_read_frame
	av_free_packet(&pkt);
	// Free audio samples buffer
	av_free(mSamples);
}
}
