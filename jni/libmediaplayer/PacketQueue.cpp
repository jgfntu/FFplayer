/* mediaplayer.cpp
 **
 */
#define DEBUG 1
#define LOG_TAG "PacketQueue"
#include <common/logwrapper.h>
#include "PacketQueue.h"
namespace ffplayer {
PacketQueue::PacketQueue() {
	pthread_mutex_init(&mLock, NULL);
	pthread_cond_init(&mCondition, NULL);
	mFirst = NULL;
	mLast = NULL;
	mNbPackets = 0;
	;
	mAbortRequest = false;
}

PacketQueue::~PacketQueue() {
	flush();
	pthread_mutex_destroy(&mLock);
	pthread_cond_destroy(&mCondition);
}

int PacketQueue::size() {
	return mNbPackets;
}

void PacketQueue::flush() {
	AVPacketList *pkt, *pkt1;
	LOGD("Ready to flush packetqueue");
	pthread_mutex_lock(&mLock);
	for (pkt = mFirst; pkt != NULL; pkt = pkt1) {
		LOGD("free packet %p", pkt->pkt.data);
		pkt1 = pkt->next;
		av_free_packet(&pkt->pkt);
		av_freep(&pkt);
	}
	mLast = NULL;
	mFirst = NULL;
	mNbPackets = 0;
	pthread_mutex_unlock(&mLock);
	LOGD("packetqueue flushed");
}

int PacketQueue::put(AVPacket* pkt) {
	if (!pkt)
		return PacketQueue::PQ_OP_IN_PARA_INVALID;

	AVPacketList *pkt1;
	/* duplicate the packet */
	if (av_dup_packet(pkt) < 0)
		return -1;

	pkt1 = (AVPacketList *) av_malloc(sizeof(AVPacketList));
	if (!pkt1)
		return -1;
	pkt1->pkt = *pkt;
	pkt1->next = NULL;

	pthread_mutex_lock(&mLock);

	if (!mLast && !mFirst) {
		mLast = mFirst = pkt1;
	} else {
		mLast->next = pkt1;
	}

	mLast = pkt1;
	mNbPackets++;

	pthread_cond_signal(&mCondition);
	pthread_mutex_unlock(&mLock);
	return PacketQueue::PQ_OP_SUCCESS;
}

/* return < 0 if aborted, 0 if no packet and > 0 if packet.  */
int PacketQueue::get(AVPacket *pkt, bool block) {
	LOGD("PacketQueue::get() in");
	AVPacketList *pkt1;
	int ret;

	pthread_mutex_lock(&mLock);

	for (;;) {
		if (mAbortRequest) {
			ret = PacketQueue::PQ_OP_ABORT_REQ;
			break;
		}

		pkt1 = mFirst;
		if (pkt1) {
			mFirst = pkt1->next;
			if (!mFirst)
				mLast = NULL;
			mNbPackets--;
			*pkt = pkt1->pkt;
			av_free(pkt1);
			ret = PacketQueue::PQ_OP_SUCCESS;
			break;
		} else if (!block) {
			ret = PacketQueue::PQ_OP_NO_MORE_PKT;
			break;
		} else {
			pthread_cond_wait(&mCondition, &mLock);
		}
	}
	pthread_mutex_unlock(&mLock);

	LOGD("PacketQueue::get() out ret:%d", ret);
	return ret;

}

void PacketQueue::abort() {
	LOGD("PacketQueue::abort() in");
	pthread_mutex_lock(&mLock);
	mAbortRequest = true;
	pthread_cond_signal(&mCondition);
	pthread_mutex_unlock(&mLock);
}
}
