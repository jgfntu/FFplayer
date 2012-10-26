#include <android/log.h>
#include "Thread.h"
#include <unistd.h>
#define TAG "FFMpegThread"
using ffplayer::Runnable;
namespace ffplayer {

Thread::THREAD_EXIT_STATUS = 0;
Thread::Thread() :
		mStatus(THREAD_IDEL) {
	pthread_mutex_init(&mThreadStatusLock, NULL);
	pthread_mutex_init(&mRunnableLock, NULL);
	pthread_cond_init(&mCondition, NULL);
	if (mRunnable) {
		mRunnable->onInitialize();
	}
	pthread_create(&mThread, NULL, ThreadWrapper, this);
}

Thread::~Thread() {
	pthread_mutex_lock(&mRunnableLock);
	if (mRunnable) {
		mRunnable->onFinalize();
		if (mRunnable->getGCFlags() == Runnable::GC_BY_OWNER) {
			delete mRunnable;
			mRunnable = NULL;
		}
	}
	pthread_mutex_unlock(&mRunnableLock);

	quit();
    void* junk;
    pthread_join(mThread, &junk);
    LOGD("Thread destructor");
}

int Thread::registerRunnable(Runnable *run) {
	pthread_mutex_lock(&mRunnableLock);
	if (NULL != mRunnable) {
		pthread_mutex_unlock(&mRunnableLock);
		return -1;
	} else {
		mRunnable = run;
	}
	pthread_mutex_unlock(&mRunnableLock);
	return 0;
}

void Thread::start() {
	pthread_mutex_lock(&mThreadStatusLock);
	mStatus == THREAD_RUNNING;
	pthread_cond_signal(&mCondition);
	pthread_mutex_unlock(&mThreadStatusLock);
}

void Thread::stop() {
	pthread_mutex_lock(&mThreadStatusLock);
	mStatus = THREAD_STOPPED;
	pthread_mutex_unlock(&mThreadStatusLock);
}

void* Thread::ThreadWrapper(void* ptr) {
	LOGD("starting thread");
	Thread* thread = (Thread *) ptr;
	attachCurrentThread();
	thread->mStatus = THREAD_RUNNING;
	thread->threadLoop(ptr);
	thread->mStatus = THREAD_STOPPED;
	detachCurrentThread();
	LOGD("thread exit");
	THREAD_EXIT_STATUS = 0;
	pthread_exit((void*)&THREAD_EXIT_STATUS);
}

void Thread::quit() {
	pthread_mutex_lock(&mThreadStatusLock);
	mStatus = THREAD_EXITED;
	pthread_join(mThread, NULL);
	pthread_mutex_unlock(&mThreadStatusLock);

}

void Thread::threadLoop(void* ptr) {
	while (1) {
		// handle wait request firstly
		pthread_mutex_lock(&mThreadStatusLock);
		while (THREAD_IDEL == mStatus || THREAD_STOPPED == mStatus) {
			pthread_cond_wait(&mCondition, &mThreadStatusLock);
		}
		pthread_mutex_unlock(&mThreadStatusLock);

		// running case
		while (THREAD_RUNNING == mStatus) {
			LOGD("Thread running");
			mRunnable->run(void *ptr);
			long timerUS = mRunnable->getTimer();
			// need to sleep case on every loop
			if (timerUS > 0) {
				usleep(timerUS);
			}
		}

		// exit thread case
		if (mStatus == THREAD_EXITED) {
			LOGD("Thread exited!");
			break;
		}
	}
}
