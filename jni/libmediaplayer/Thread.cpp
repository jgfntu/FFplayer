#define LOG_TAG "Thread"
#include <common/logwrapper.h>
#include "Thread.h"
#include <unistd.h>
#include "BuddyRunnable.h"
namespace ffplayer {
int Thread::THREAD_EXIT_STATUS = 0;

Thread::Thread() :
		mStatus(THREAD_IDEL), mRunnable(NULL) {
	pthread_mutex_init(&mThreadStatusLock, NULL);
	pthread_mutex_init(&mRunnableLock, NULL);
	pthread_cond_init(&mCondition, NULL);
	LOGD("create Thread step 1");

	pthread_attr_t attr;
	pthread_attr_init(&attr);
	pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
	LOGD("create Thread step 3");
	pthread_create(&mThread, &attr, ThreadWrapper, this);
	LOGD("create Thread step 4");
}

Thread::~Thread() {
	pthread_mutex_lock(&mRunnableLock);
	if (mRunnable) {
		mRunnable->onFinalize();
		if (mRunnable->getGCFlags() == BuddyRunnable::GC_BY_EXTERNAL) {
			delete mRunnable;
			mRunnable = NULL;
		}
	}
	pthread_mutex_unlock(&mRunnableLock);

	quit();
	LOGD("Thread destructor");
}

int Thread::registerRunnable(BuddyRunnable *run) {
	if (!run) {
		return -1;
	}
	pthread_mutex_lock(&mRunnableLock);
	if (mRunnable) {
		pthread_mutex_unlock(&mRunnableLock);
		return -1;
	} else if (BuddyRunnable::INITIATOR == run->getBuddyType()) {
		LOGD("The BuddyRunnable:%p registerred successfully!", run);
		mRunnable = run;
		mRunnable->onInitialize();
	} else {
		pthread_mutex_unlock(&mRunnableLock);
		return -1;
	}
	pthread_mutex_unlock(&mRunnableLock);
	return 0;
}

void Thread::start() {
	pthread_mutex_lock(&mThreadStatusLock);
	mStatus = THREAD_RUNNING;
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
	pthread_exit((void*) &THREAD_EXIT_STATUS);
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
			mRunnable->run(ptr);
		}

		// exit thread case
		if (mStatus == THREAD_EXITED) {
			LOGD("Thread exited!");
			break;
		}
	}
}
}
