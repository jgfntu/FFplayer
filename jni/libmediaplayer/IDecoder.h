#ifndef __IDECODER_H__
#define __IDECODER_H__

extern "C" {
#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"
#include "libavutil/avutil.h"
}
extern AVPacket BOS_PKT;
extern AVPacket EOS_PKT;
#include "Thread.h"
#include "packetqueue.h"
using ffplayer::Runnable;
namespace ffplayer {
class IDecoder: public Runnable {
public:
	enum DECODER_STATUS {
		STATUS_IDLE,
		STATUS_INIT,
		STATUS_PREPARE,
		STATUS_STARTED,
		STATUS_STOPPED,
		STATUS_SEEKING
	};
	enum {
		RET_SCCESS, RET_ERROR
	};

	typedef enum BuddyType {
		INITIATOR,
		NEUTRAL,
		FOLLOWER
	} BuddyType;
	IDecoder(AVStream* stream, PacketQueue *queue, Thread *loopThread = NULL);
	virtual ~IDecoder();
	void flushDataQueue();
	int start();
	int stop();
	virtual int associateBuddy(IDecoder *buddy);
	virtual BuddyType getBuddyType() = 0;
	virtual int onDriveByBuddy(long timeStamp);
	void enqueue(AVPacket* packet);
	int packets();
	//inherit from Runnable interface
	long getTimer();
	int getGCFlags();
	void run(void *ptr);
	void onInitialize();
	void onFinalize();

protected:
	virtual bool prepare();
	virtual bool decode(void* ptr);
	virtual PacketQueue * getPktQueque();
	PacketQueue* mQueue;
	AVStream* mStream;
	Thread* mLoopThread;
	IDecoder* mBuddy;
};
}
#endif //__IDECODER_H__
