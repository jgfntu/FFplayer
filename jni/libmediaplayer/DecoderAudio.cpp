#define LOG_TAG "DecoderAudio"

#include <common/logwrapper.h>
#include "DecoderAudio.h"
#include <unistd.h>
#include "Utils.h"

using ffplayer::BuddyRunnable;

namespace ffplayer
{
    DecoderAudio::DecoderAudio(AVStream *    stream,
                               PacketQueue * queue,
                               Thread *      loopThread):
        mStream(stream),
        mQueue(queue),
        mLoopThread(loopThread),
        mTimer(0),
        mAudioClock(0.0),
        mBuddy(NULL)
    {
    }

    int64_t DecoderAudio::now(void)
    {
        struct timeval tv;


        gettimeofday(&tv, NULL);

        return (int64_t) tv.tv_sec * 1000000ll + tv.tv_usec;
    }

    // state machine operation
    int DecoderAudio::start()
    {
        CHECK_POINTER_INT(mLoopThread, -1);
        mLoopThread -> registerRunnable(this);
        mLoopThread -> start();

        return RET_SCCESS;
    }

    int DecoderAudio::stop()
    {
        CHECK_POINTER_INT(mQueue, -1);
        mQueue -> abort();
        CHECK_POINTER_INT(mLoopThread, -1);
        LOGE("waiting on end of mLoopThread thread");

        if (mLoopThread)
        {
            // mLoopThread->stop();
            mLoopThread -> quit();
        }

        return RET_SCCESS;
    }

    DecoderAudio::~DecoderAudio()
    {
        CHECK_POINTER_VOID(mBuddy);
        CHECK_POINTER_VOID(mLoopThread);
        mLoopThread -> quit();

        if (NULL != mQueue)
        {
            mQueue -> flush();
            av_free(mQueue);

            mQueue = NULL;
        }

        CHECK_POINTER_VOID(mStream);
        avcodec_close(mStream -> codec);

        if ((BuddyRunnable::GC_BY_EXTERNAL == mBuddy -> getGCFlags()) && (FOLLOWER == mBuddy -> getBuddyType()))
        {
            delete mBuddy;

            mBuddy = NULL;
        }
    }

    void DecoderAudio::flush()
    {
        // TODO: more precise, should put a flush pkt
        CHECK_POINTER_VOID(mQueue);
        mQueue -> flush();
        CHECK_POINTER_VOID(mStream);
        avcodec_flush_buffers(mStream -> codec);
    }

    BuddyRunnable::BuddyType DecoderAudio::getBuddyType()
    {
        return INITIATOR;
    }

    int DecoderAudio::onBuddyEvent(BuddyRunnable * buddy,
                                   BuddyEvent      evt)
    {
        return 0;
    }

    int DecoderAudio::getGCFlags()
    {
        return BuddyRunnable::GC_BY_EXTERNAL;
    }

    int DecoderAudio::bindBuddy(BuddyRunnable * buddy)
    {
        if (mBuddy)
        {
            return -1;
        }
        else
        {
            mBuddy = buddy;
        }
    }

    void DecoderAudio::onInitialize()
    {
        mSamplesSize = AVCODEC_MAX_AUDIO_FRAME_SIZE;
        mSamples     = (int16_t *) av_malloc(mSamplesSize);

        if (!mSamples ||!mStream)
        {
            return;
        }

        if (mStream -> start_time != AV_NOPTS_VALUE)
        {
            mAudioClock = mStream -> start_time * av_q2d(mStream -> time_base);
        }
        else
        {
            mAudioClock = 0.0;
        }

        LOGD("onInitialize mAudioClock:%f!", mAudioClock);

        mLastClock   = mAudioClock;
        mLastAbsTime = now();

        LOGD("onInitialize mLastAbsTime:%lld, now():%lld!", mLastAbsTime, now());
        CHECK_POINTER_VOID(mBuddy);
        mBuddy -> onInitialize();
    }

    void DecoderAudio::onFinalize()
    {
        CHECK_POINTER_VOID(mBuddy);

        if (mBuddy)
        {
            mBuddy -> onInitialize();
        }

        // Free audio samples buffer
        if (mSamples)
        {
            av_free(mSamples);

            mSamples = NULL;
        }

        // ourself stuff here
    }

    int DecoderAudio::decodeRender()
    {
        /*
         *  Process some special Events
         * 1) BOS
         * 2) EOS
         */
        AVPacket pkt;

        av_init_packet(&pkt);
        CHECK_POINTER_INT(mQueue, -1);
        LOGD("AudioQueue get start()");

        if (mQueue -> get(&pkt, true) != PacketQueue::PQ_OP_SUCCESS)
        {
            LOGE("getAudio Packet error, thread exit!");

            return -1;
        }

        LOGD("AudioQueue get end()");

        if (&BOS_PKT == &pkt)
        {
            LOGI("Audio Decoder Received BOS PKT!");

            /* update our clock */
            pkt.pts;

            return 0;
        }
        else if (&EOS_PKT == &pkt)
        {
            LOGI("Audio Decoder Received EOS PKT!");

            return 0;
        }

        if (pkt.pts != AV_NOPTS_VALUE)
        {
            mAudioClock = av_q2d(mStream -> time_base) * pkt.pts;
        }

        LOGD("after adjust 1 mAudioClock:%f!", mAudioClock);

        AVCodecContext * dec = mStream -> codec;

        CHECK_POINTER_INT(dec, -1);

        int       remainBufSize    = mSamplesSize,
                  curOutputBufSize = mSamplesSize;
        int       dataSize         = 0;
        int16_t * outputBuf        = mSamples;
        AVPacket  dupPkt           = pkt;

        while ((dupPkt.size > 0) && ((curOutputBufSize = remainBufSize) > 0))
        {
            LOGD("Before avcodec_decode_audio3()");

            int len = avcodec_decode_audio3(mStream -> codec, (int16_t *) outputBuf, &curOutputBufSize, &pkt);

            LOGD("after avcodec_decode_audio3() len:%d, curOutputBufSize:%d", len, curOutputBufSize);

            if ((len < 0) || (!curOutputBufSize))
            {
                dupPkt.size = 0;

                break;
            }

            LOGD("After avcodec_decode_audio3()");

            dupPkt.data   += len;
            dupPkt.size   -= len;
            remainBufSize -= curOutputBufSize;
            outputBuf     += curOutputBufSize / (sizeof(int16_t));
            dataSize      += curOutputBufSize;

            LOGD("Audio Decoder Thread Processing");
        }

        LOGD("Jump out of while loop");

        double bytesPerSec = dec -> channels * bytesPerSample(dec -> sample_fmt) * dec -> sample_rate;

        mAudioClock += (double) dataSize / (double) (bytesPerSec);

        LOGD("after adjust 2 mAudioClock:%f, mLastClock:%f!", mAudioClock, mLastClock);

        // TODO:
        // update the delay time
        mTimer = (mAudioClock - mLastClock);

        // call handler for posting buffer to os audio driver
        LOGD("Before rendorHook() mTimer:%f", mTimer);
        rendorHook(mSamples, dataSize);

        long delta = now() - mLastAbsTime;

        LOGD("delta:%lld, mLastAbsTime:%lld, now():%lld", delta, mLastAbsTime, now());

        mLastAbsTime = now();

        usleep(mTimer * 1e6);

        mLastClock = mAudioClock;

        CHECK_POINTER_INT(mBuddy, -1);

        BuddyEvent evt;

        evt.type       = AV_SYNC;
        evt.data.dData = mAudioClock;

        LOGD("Ready to call mBuddy's onBuddyEvent()");
        mBuddy -> onBuddyEvent(this, evt);
        av_free_packet(&pkt);
        LOGD("Ready to call av_free_packet() called!");

        return 0;
    }

    int DecoderAudio::bytesPerSample(enum SampleFormat format)
    {
        switch (format)
        {
            case SAMPLE_FMT_U8 :
                return 1;

            case SAMPLE_FMT_S16 :
                return 2;

            case SAMPLE_FMT_S32 :
                return 4;

            default :
                return 2;
        }
    }

    void DecoderAudio::run(void * ptr)
    {
        ISSUE_FUNCTION_IN();

        if (decodeRender())
        {
            LOGD("Audio process error, thread exit!");

            return;
        }

        LOGD("decodeRender finish!");
    }
}


//~ Formatted by Jindent --- http://www.jindent.com
