#ifndef MEDIA_CLOCK_H
#define MEDIA_CLOCK_H
#include "thread.h"

class MediaClock
{
public:
	enum {
		RENDER_DELAY = 10
	};
	void incClockBy(double delta);
	void decClockBy(double delta);
	double getCurClock();
	void setCurClock(double time);
	static MediaClock* instance();
protected:

private :
	MediaClock();
	~MediaClock();
       pthread_mutex_t     		mClockMutex;
	long 					mMasterClock;
	static MediaClock * sInstance;

};

#endif //MEDIA_CLOCK_H
