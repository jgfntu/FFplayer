#ifndef __BUDDY_RUNNABLE_H__
#define __BUDDY_RUNNABLE_H__
namespace ffplayer {
class BuddyRunnable {
public:
	/**
	 * flags to indicate whether this object should cleanup by it self or
	 * its owner(mainly the thread host object)
	 */
	enum {
		GC_BY_EXTERNAL, GC_BY_SELF
	};

	typedef enum BuddyType {
		INITIATOR, NEUTRAL, FOLLOWER
	} BuddyType;

    typedef enum EventType {
		AV_SYNC,
	} EventType;

	typedef struct BuddyEvent {
		EventType type;
		union {
			void *pData;
			int iData;
			double dData;
		} data;
	} BuddyEvent;

	virtual BuddyType getBuddyType() = 0;
	virtual int onBuddyEvent(BuddyRunnable *buddy, BuddyEvent evt) = 0;
	virtual int bindBuddy(BuddyRunnable *buddy) = 0;
	virtual int getGCFlags() = 0;
	virtual void run(void *ptr) = 0;
	virtual void onInitialize() = 0;
	virtual void onFinalize() = 0;
};
}
#endif //__BUDDY_RUNNABLE_H__
