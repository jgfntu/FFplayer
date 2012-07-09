#include "mediaclock.h"
#define TAG "MediaClock"

MediaClock * MediaClock::sInstance = NULL;
MediaClock* MediaClock::instance() {
	if (NULL == sInstance) {
		sInstance = new MediaClock();
	}
	return sInstance;
}
MediaClock::MediaClock() {
	pthread_mutex_init(&mClockMutex, NULL);
	pthread_mutex_init(&mStatusMutex, NULL);
	pthread_cond_init(&mStatusCondition, NULL);

	mMasterClock = 0;
	mStatus = CLOCK_KICKING;
}
void MediaClock::destroy()
{
	if (NULL != sInstance) {
		delete sInstance;
		sInstance = NULL;
	}
}

MediaClock::~MediaClock()
{
	pthread_mutex_destroy(&mClockMutex);
	pthread_mutex_destroy(&mStatusMutex);
	pthread_cond_destroy(&mStatusCondition);
}


void MediaClock::incClockBy(unsigned long delta) {
	pthread_mutex_lock(&mClockMutex);
	mMasterClock += delta;
	pthread_mutex_unlock(&mClockMutex);
}

void MediaClock::decClockBy(unsigned long delta) {
	pthread_mutex_lock(&mClockMutex);
	mMasterClock -= delta;

	if (mMasterClock < 0)
	{
		mMasterClock = 0;
	}

	pthread_mutex_unlock(&mClockMutex);
}

unsigned long MediaClock::getCurClock() {
	return mMasterClock;
}

int MediaClock::getCurClockStatus() {
	return mStatus;
}

void MediaClock::setCurClockStatus(int newStatus) {
	pthread_mutex_lock(&mStatusMutex);
	if ((CLOCK_KICKING == newStatus) && (CLOCK_PAUSED == mStatus))
	{
		//pthread_cond_signal(&mStatusCondition);
		pthread_cond_broadcast(&mStatusCondition);
	}
	__android_log_print(ANDROID_LOG_INFO, TAG, "setCurClockStatus to %d!", newStatus);

	mStatus = newStatus;
	pthread_mutex_unlock(&mStatusMutex);
}

void MediaClock::setCurClock(unsigned long time) {
	pthread_mutex_lock(&mClockMutex);
	mMasterClock = time;
	pthread_mutex_unlock(&mClockMutex);
}

void MediaClock::waitOnClock() {
	pthread_mutex_lock(&mStatusMutex);
	while (CLOCK_PAUSED == mStatus) {
		pthread_cond_wait(&mStatusCondition, &mStatusMutex);
	}
	pthread_mutex_unlock(&mStatusMutex);
}



