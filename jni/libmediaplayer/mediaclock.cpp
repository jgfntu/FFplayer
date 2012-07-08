#include "mediaclock.h"

MediaClock * MediaClock::sInstance = NULL;
MediaClock* MediaClock::instance() {
	if (NULL == sInstance) {
		sInstance = new MediaClock();
	}
	return sInstance;
}
MediaClock::MediaClock() {
	pthread_mutex_init(&mClockMutex, NULL);
	mMasterClock = 0.0;
}

void MediaClock::incClockBy(double delta) {
	pthread_mutex_lock(&mClockMutex);
	mMasterClock += delta;
	pthread_mutex_unlock(&mClockMutex);
}

void MediaClock::decClockBy(double delta) {
	pthread_mutex_lock(&mClockMutex);
	mMasterClock -= delta;
	pthread_mutex_unlock(&mClockMutex);
}

double MediaClock::getCurClock() {
	return mMasterClock;
}

void MediaClock::setCurClock(double time) {
	pthread_mutex_lock(&mClockMutex);
	mMasterClock = time;
	pthread_mutex_unlock(&mClockMutex);
}

