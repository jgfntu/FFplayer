#include <android/log.h>
#include "decoder_audio.h"
#include <unistd.h>
#include "mediaclock.h"
#define TAG "FFMpegAudioDecoder"
int DecoderAudio::skipTimes = 0;

DecoderAudio::DecoderAudio(AVStream* stream) : IDecoder(stream)
{
}

DecoderAudio::~DecoderAudio()
{
}

bool DecoderAudio::prepare()
{
    mSamplesSize = AVCODEC_MAX_AUDIO_FRAME_SIZE;
    mSamples = (int16_t *) av_malloc(mSamplesSize);

    if(mSamples == NULL) {
    	return false;
    }
    //(MediaClock::instance())->setCurClock(1000.0 * mStream->start_time * av_q2d(mStream->time_base));
    MediaClock::instance();
    return true;
}

bool DecoderAudio::process(AVPacket *packet)
{
    int size = mSamplesSize;
    //for (;;) {
    	//TODO: this packet may not be decompressed completely!
	    int len = avcodec_decode_audio3(mStream->codec, mSamples, &size, packet);

	    if ((len < 0) && (skipTimes++ > MAX_SKIP_TIME)) {
	        /* if error, we skip the frame */
	        return false;
	    }

   //}

    /* if no pts, then compute it */
    AVCodecContext *dec = mStream->codec;
    double pts = packet->pts * av_q2d(mStream->time_base) * 1000.0;
    double timeDelta = 0.0;
    if (pts != AV_NOPTS_VALUE) {
	    timeDelta = (pts - (MediaClock::instance())->getCurClock());
           /* substract the render time diff */
           double usecsD = (timeDelta - MediaClock::RENDER_DELAY) * 1000;
           unsigned int usecs = usecsD;
           if (usecsD > 0.0) {
	           if (usecs < 1000000) {
		    	usleep(usecs);
		    }else {
		    	sleep((unsigned int)(usecs / 1000000));
		    }
	    }
	    (MediaClock::instance())->setCurClock(pts);

	    onDecode(mSamples, size);
	    //call handler for posting buffer to os audio driver
    } else {
	    timeDelta =  1000.0 * ((double)size /
	        (dec->channels * dec->sample_rate * bytesPerSample(dec->sample_fmt)));
	    onDecode(mSamples, size);

           /* substract the render time diff */
           double usecsD = (timeDelta - MediaClock::RENDER_DELAY) * 1000;
           unsigned int usecs = usecsD;
           if (usecsD > 0.0) {
	           if (usecs < 1000000) {
		    	usleep(usecs);
		    }else {
		    	sleep((unsigned int)(usecs / 1000000));
		    }
	    }
          if (timeDelta > 0.0) {
  	    	(MediaClock::instance())->incClockBy(timeDelta);
          }
    }



    return true;
}


int DecoderAudio::bytesPerSample(enum SampleFormat format) {
	switch(format) {
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

bool DecoderAudio::decode(void* ptr)
{
    AVPacket        pPacket;

    __android_log_print(ANDROID_LOG_INFO, TAG, "decoding audio");

    while(mRunning)
    {
        if(mQueue->get(&pPacket, true) < 0)
        {
            mRunning = false;
            return false;
        }
        if(!process(&pPacket))
        {
            mRunning = false;
            return false;
        }
        // Free the packet that was allocated by av_read_frame
        av_free_packet(&pPacket);
    }

    __android_log_print(ANDROID_LOG_INFO, TAG, "decoding audio ended");

    // Free audio samples buffer
    av_free(mSamples);
    return true;
}
