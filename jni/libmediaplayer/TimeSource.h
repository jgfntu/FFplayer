#ifndef FFPLAYER_TIMESOURCE_H
#define FFPLAYER_TIMESOURCE_H
class TimeSource {
	virtual long getRealTimeMS() = 0;
	virtual void setRealTimeMS(long newMS) = 0;
};
#endif
