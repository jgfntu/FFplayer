#ifndef FFMPEG_PACKETQUEUE_H
#define FFMPEG_PACKETQUEUE_H

#include <pthread.h>

extern "C" {

#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"

} // end of extern C

class PacketQueue
{
public:
	/**
	 * packetqueue operation result/status code
	 */
	enum {
		/* operation success */
		PQ_OP_SUCCESS,
		/* memory related error */
		PQ_OP_MEM_ERROR,
		/* input parameter invalid */
		PQ_OP_IN_PARA_INVALID,
		/* abort request */
		PQ_OP_ABORT_REQ,
		/* no more pkt */
		PQ_OP_NO_MORE_PKT,

	};
	/**
	 * constructor and destructor
	 */
	PacketQueue();
	~PacketQueue();

	/**
	 * clean all the avpacket in this queue and release related resources
	 */
    void flush();
    /**
     * put a packet to the queue
     */
	int put(AVPacket* pkt);

	/* return < 0 if aborted, 0 if no packet and > 0 if packet.  */
	int get(AVPacket *pkt, bool block);

	int size();

	void abort();

private:
	AVPacketList*		mFirst;
	AVPacketList*		mLast;
    int					mNbPackets;
    bool				mAbortRequest;
	pthread_mutex_t     mLock;
	pthread_cond_t		mCondition;
};

#endif // FFMPEG_MEDIAPLAYER_H
