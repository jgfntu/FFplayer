#ifndef __RUNNABLE_H__
namespace ffplayer {
class Runnable {
public:
	/**
	 * flags to indicate whether this object should cleanup by it self or
	 * its owner(mainly the thread host object)
	 */
	enum {
		GC_BY_OWNER,
		GC_BY_SELF
	};
	virtual long getTimer() = 0;
	virtual int getGCFlags() = 0;
	virtual void run(void *ptr) = 0;
	/**
	 *  called on the first access to this object
	 */
	virtual void onInitialize() = 0;
	/**
	 *  called on the last access of this object
	 *  client should do remember to release some internal resources
	 *  in this callback
	 */
	virtual void onFinalize() = 0;
};
}
#endif
