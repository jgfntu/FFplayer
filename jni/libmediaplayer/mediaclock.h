#ifndef MEDIA_CLOCK_H
#define MEDIA_CLOCK_H
#include "thread.h"
#include <android/log.h>
class MediaClock
{
public:
	enum {
		RENDER_DELAY = 0,
		CLOCK_PAUSED,
		CLOCK_KICKING
	};
	void incClockBy(unsigned long delta);
	void decClockBy(unsigned long delta);
	unsigned long getCurClock();
	void setCurClock(unsigned long time);
	void setCurClockStatus(int newStatus);
	int getCurClockStatus();
	void waitOnClock();

	static MediaClock* instance();
	static  void destroy();

protected:

private :
	MediaClock();
	~MediaClock();
       pthread_mutex_t     		mClockMutex;
    unsigned long 				mMasterClock;
    int 						mStatus;
	static MediaClock * sInstance;
	pthread_mutex_t     mStatusMutex;
	pthread_cond_t		mStatusCondition;

};

#endif //MEDIA_CLOCK_H
