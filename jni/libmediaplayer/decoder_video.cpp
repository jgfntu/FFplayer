#include <android/log.h>
#include "decoder_video.h"
#include <unistd.h>
#include "mediaclock.h"
#define TAG "FFMpegVideoDecoder"

static uint64_t global_video_pkt_pts = AV_NOPTS_VALUE;

DecoderVideo::DecoderVideo(AVStream* stream) : IDecoder(stream)
{
	 mStream->codec->get_buffer = getBuffer;
	 mStream->codec->release_buffer = releaseBuffer;
}

DecoderVideo::~DecoderVideo()
{
}

bool DecoderVideo::prepare()
{
	mFrame = avcodec_alloc_frame();
	if (mFrame == NULL) {
		return false;
	}
	return true;
}

/* pts is in the unit of second */
double DecoderVideo::synchronize(AVFrame *src_frame, unsigned long pts) {
	/* sync to MediaClock */
	unsigned long  diff = pts - (MediaClock::instance())->getCurClock();
	__android_log_print(ANDROID_LOG_INFO, TAG, "Video Decoder Thread synchronize() pts:%ld, MediaClock:%ld!", (long)pts, (long)MediaClock::instance()->getCurClock());

	__android_log_print(ANDROID_LOG_INFO, TAG, "Video Decoder Thread sync diff:%ld ms!", diff);


	if (0 < diff && diff < 100000) {
		usleep(diff - MediaClock::RENDER_DELAY * 1000 * 0.5);
	}



	return pts;
}

bool DecoderVideo::process(AVPacket *packet)
{
	/* Process some special Events
	 * 1) BOS
	 * 2) EOS
	 */
	if (&BOS_PKT == packet) {
		__android_log_print(ANDROID_LOG_INFO, TAG, "Video Decoder Received BOS PKT!");
		return true;
	}else if (&EOS_PKT == packet) {
		__android_log_print(ANDROID_LOG_INFO, TAG, "Video Decoder Received EOS PKT!");
		return false;
	}

    int got_picture;
    unsigned long pts = 0;

	// Decode video frame
    if (packet->size == 0 || NULL == packet->data)
    {
    	return true;
    }

	avcodec_decode_video2(mStream->codec,
						 mFrame,
						 &got_picture,
						 packet);
	if (packet->pts != AV_NOPTS_VALUE) {
		pts = packet->pts;
	} else if (packet->dts != AV_NOPTS_VALUE) {
		pts = packet->dts;
	} else if (packet->dts == AV_NOPTS_VALUE && mFrame->opaque
			&& *(uint64_t*) mFrame->opaque != AV_NOPTS_VALUE) {
		pts = *(uint64_t *) mFrame->opaque;
	} else {
		pts = 0;
	}

	pts = 1000 * pts * av_q2d(mStream->time_base);


	if (got_picture) {
		pts = synchronize(mFrame, pts);

		onDecode(mFrame, pts);
		__android_log_print(ANDROID_LOG_INFO, TAG, "Video Decoder Thread Processing!");
		return true;
	}

	__android_log_print(ANDROID_LOG_INFO, TAG, "Video Decoder Thread Decoding error ret:%d, pkt:(data %p size:%d)!",
			got_picture, packet->data, packet->size);
    // Free the packet that was allocated by av_read_frame
    av_free_packet(packet);
	return false;
}

bool DecoderVideo::decode(void* ptr)
{
	MediaClock::instance()->waitOnClock();

	AVPacket        pPacket;
	__android_log_print(ANDROID_LOG_INFO, TAG, "decoding video");

    while(mRunning)
    {
        __android_log_print(ANDROID_LOG_INFO, TAG, "Video Queue get start()");
        if(mQueue->get(&pPacket, true) < 0)
        {
            __android_log_print(ANDROID_LOG_INFO, TAG, "getVideo Packet error, thread exit!");
            mRunning = false;
            return false;
        }
        __android_log_print(ANDROID_LOG_INFO, TAG, "Video Queue get end()");
        if(!process(&pPacket))
        {
            __android_log_print(ANDROID_LOG_INFO, TAG, "Video process error, thread exit!");

            mRunning = false;
            return false;
        }

    }

    __android_log_print(ANDROID_LOG_INFO, TAG, "decoding video ended");

    // Free the RGB image
    av_free(mFrame);

    return true;
}

/* These are called whenever we allocate a frame
 * buffer. We use this to store the global_pts in
 * a frame at the time it is allocated.
 */
int DecoderVideo::getBuffer(struct AVCodecContext *c, AVFrame *pic) {
	int ret = avcodec_default_get_buffer(c, pic);
	uint64_t *pts = (uint64_t *)av_malloc(sizeof(uint64_t));
	*pts = global_video_pkt_pts;
	pic->opaque = pts;
	return ret;
}
void DecoderVideo::releaseBuffer(struct AVCodecContext *c, AVFrame *pic) {
	if (pic)
		av_freep(&pic->opaque);
	avcodec_default_release_buffer(c, pic);
}
