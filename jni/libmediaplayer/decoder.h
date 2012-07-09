#ifndef FFMPEG_DECODER_H
#define FFMPEG_DECODER_H

extern "C" {

#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"
#include "libavutil/avutil.h"
}
extern AVPacket BOS_PKT;
extern AVPacket EOS_PKT;
#include "thread.h"
#include "packetqueue.h"

class IDecoder : public Thread
{
public:
	IDecoder(AVStream* stream);
	virtual ~IDecoder();
	void flushDataQueue();
    void						stop();
	void						enqueue(AVPacket* packet);
	int							packets();

protected:
    PacketQueue*                mQueue;
    AVStream*             		mStream;

    virtual bool                prepare();
    virtual bool                decode(void* ptr);
    virtual bool                process(AVPacket *packet);
	void						handleRun(void* ptr);
};

#endif //FFMPEG_DECODER_H
