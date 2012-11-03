
#ifndef __DECODER_VIDEO_H
#define __DECODER_VIDEO_H
extern "C"
{
#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"
#include "libavutil/avutil.h"

}

#include "IDecoder.h"
#include "BuddyRunnable.h"
#include "Thread.h"
#include "PacketQueue.h"

namespace ffplayer
{
    typedef void (*VideoRenderCB)(AVFrame *);

    class DecoderVideo:
        public BuddyRunnable,
        public IDecoder
    {
        public:
            DecoderVideo(AVStream *    stream,
                         PacketQueue * queue,
                         Thread *      loopThread);

            virtual ~DecoderVideo();

            VideoRenderCB rendorHook;

            // interface of IDecoder
            virtual void flush();

            virtual int stop();

            virtual int start();

        private:
            AVFrame *       mFrame;
            double          mVideoClock;
            BuddyRunnable * mBuddy;
            PacketQueue *   mQueue;
            AVStream *      mStream;
            Thread *        mLoopThread;
            double          mLastPts;

            int synchronize(AVFrame * src_frame,
                            double    pts,
                            double    clock);

            // buddy interface
            BuddyRunnable::BuddyType getBuddyType();

            int onBuddyEvent(BuddyRunnable * buddy,
                             BuddyEvent      evt);

            int bindBuddy(BuddyRunnable * buddy);

            int getGCFlags();

            void run(void * ptr);

            void onInitialize();

            void onFinalize();

            int decodeRender(double clock);

            static int getBuffer(struct AVCodecContext *c,
                                 AVFrame *             pic);

            static void releaseBuffer(struct AVCodecContext *c,
                                      AVFrame *             pic);
    };
}
#endif //__DECODER_VIDEO_H


//~ Formatted by Jindent --- http://www.jindent.com
