
#ifndef FFMPEG_THREAD_H
#define FFMPEG_THREAD_H

#include "../jni/jniUtils.h"
#include <pthread.h>
#include "BuddyRunnable.h"
#include <common/logwrapper.h>

using namespace ffplayer;

namespace ffplayer
{
    class Thread
    {
        public:
            enum
            {
                THREAD_IDEL, THREAD_RUNNING, THREAD_STOPPED, THREAD_EXITED
            };

            Thread();

            virtual ~Thread();

            virtual void start();

            virtual void stop();

            virtual void quit();

            virtual int registerRunnable(BuddyRunnable * run);

            inline int getStatus()
            {
                return mStatus;
            }

        protected:
            int             mStatus;
            BuddyRunnable * mRunnable;

        private:
            pthread_t       mThread;
            pthread_mutex_t mThreadStatusLock;
            pthread_mutex_t mRunnableLock;
            pthread_cond_t  mCondition;
            static int      THREAD_EXIT_STATUS;

            static void * ThreadWrapper(void * ptr);

            void threadLoop(void * ptr);
    };
}
#endif //FFMPEG_DECODER_H


//~ Formatted by Jindent --- http://www.jindent.com
