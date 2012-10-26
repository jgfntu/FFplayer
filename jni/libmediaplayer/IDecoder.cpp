#include <android/log.h>
#include "IDecoder.h"
using ffplayer::Runnable;
#define TAG "FFMpegIDecoder"
using ffplayer;
namespace ffplayer {
IDecoder::IDecoder(AVStream* stream, PacketQueue *queue, Thread *loopThread) :
		mStream(stream), mQueue(queue), mLoopThread(loopThread) {
	if (mLoopThread) {
		mLoopThread->registerRunnable(this);
	}
}

IDecoder::~IDecoder() {
	stop();
	if (NULL != mQueue) {
		av_free(mQueue);
		mQueue = NULL;
	}
	avcodec_close(mStream->codec);
}

void IDecoder::enqueue(AVPacket* packet) {
	mQueue->put(packet);
}

void IDecoder::flushDataQueue() {
	mQueue->flush();
}

int IDecoder::packets() {
	return mQueue->size();
}

int IDecoder::start() {
	if (mLoopThread) {
		mLoopThread->start();
	}
}

int associateBuddy(IDecoder* buddy) {
	if (!mBuddy) {
		return -1;
	} else {
		mBuddy = buddy;
		return 0;
	}
}

int IDecoder::stop() {
	mQueue->abort();
	LOGD("waiting on end of decoder thread");
	if (mLoopThread) {
		mLoopThread->stop();
	}
	return RET_SCCESS;
}

void IDecoder::run(void* ptr) {
	decode(ptr);
	if ((BuddyType::INITIATOR == getBuddyType()) && mBuddy) {
		long ts = 100000L;
		mBuddy->onDriveByBuddy(ts);
	}
}

bool IDecoder::decode(void* ptr) {
	return false;
}

PacketQueue * IDecoder::getPktQueque() {
	return mQueue;
}
}
