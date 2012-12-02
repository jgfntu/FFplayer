
#ifndef __MEDIAPLAYER_H
#define __MEDIAPLAYER_H

#include <pthread.h>
#include <jni.h>
#include <android/Errors.h>
#include "../libffmpeg_jni/jniUtils.h"
#include "DecoderVideo.h"
#include "DecoderAudio.h"

#define FFMPEG_PLAYER_MAX_QUEUE_SIZE 20
using namespace android;
using namespace ffplayer;

enum media_event_type
{
    MEDIA_NOP = 0, MEDIA_PREPARED = 1, MEDIA_PLAYBACK_COMPLETE = 2, MEDIA_BUFFERING_UPDATE = 3,
    MEDIA_SEEK_COMPLETE = 4, MEDIA_SET_VIDEO_SIZE = 5, MEDIA_ERROR = 100, MEDIA_INFO = 200,
};

// Generic error codes for the media player framework.  Errors are fatal, the
// playback must abort.
//
// Errors are communicated back to the client using the
// MediaPlayerListener::notify method defined below.
// In this situation, 'notify' is invoked with the following:
// 'msg' is set to MEDIA_ERROR.
// 'ext1' should be a value from the enum media_error_type.
// 'ext2' contains an implementation dependant error code to provide
// more details. Should default to 0 when not used.
//
// The codes are distributed as follow:
// 0xx: Reserved
// 1xx: Android Player errors. Something went wrong inside the MediaPlayer.
// 2xx: Media errors (e.g Codec not supported). There is a problem with the
// media itself.
// 3xx: Runtime errors. Some extraordinary condition arose making the playback
// impossible.
//
enum media_error_type
{
    // 0xx
    MEDIA_ERROR_UNKNOWN = 1,

    // 1xx
    MEDIA_ERROR_SERVER_DIED = 100,

    // 2xx
    MEDIA_ERROR_NOT_VALID_FOR_PROGRESSIVE_PLAYBACK = 200,

    // 3xx
};

// Info and warning codes for the media player framework.  These are non fatal,
// the playback is going on but there might be some user visible issues.
//
// Info and warning messages are communicated back to the client using the
// MediaPlayerListener::notify method defined below.  In this situation,
// 'notify' is invoked with the following:
// 'msg' is set to MEDIA_INFO.
// 'ext1' should be a value from the enum media_info_type.
// 'ext2' contains an implementation dependant info code to provide
// more details. Should default to 0 when not used.
//
// The codes are distributed as follow:
// 0xx: Reserved
// 7xx: Android Player info/warning (e.g player lagging behind.)
// 8xx: Media info/warning (e.g media badly interleaved.)
//
enum media_info_type
{
    // 0xx
    MEDIA_INFO_UNKNOWN = 1,

    // 7xx
    // The video is too complex for the decoder: it can't decode frames fast
    // enough. Possibly only the audio plays fine at this stage.
    MEDIA_INFO_VIDEO_TRACK_LAGGING = 700,

    // 8xx
    // Bad interleaving means that a media has been improperly interleaved or not
    // interleaved at all, e.g has all the video samples first then all the audio
    // ones. Video is playing but a lot of disk seek may be happening.
    MEDIA_INFO_BAD_INTERLEAVING = 800,

    // The media is not seekable (e.g live stream).
    MEDIA_INFO_NOT_SEEKABLE = 801,

    // New media metadata is available.
    MEDIA_INFO_METADATA_UPDATE = 802, MEDIA_INFO_FRAMERATE_VIDEO = 900, MEDIA_INFO_FRAMERATE_AUDIO,
};

enum media_player_states
{
    MEDIA_PLAYER_STATE_ERROR = 0, MEDIA_PLAYER_IDLE = 1 << 0, MEDIA_PLAYER_INITIALIZED = 1 << 1,
    MEDIA_PLAYER_PREPARED = 1 << 2, MEDIA_PLAYER_RUNNING = 1 << 3, MEDIA_PLAYER_PAUSED = 1 << 4,
    MEDIA_PLAYER_STOPPED = 1 << 5, MEDIA_PLAYER_PLAYBACK_COMPLETE = 1 << 6
};

// ----------------------------------------------------------------------------
// ref-counted object for callbacks
class MediaPlayerListener
{
    public:
        virtual void notify(int msg,
                            int ext1,
                            int ext2) = 0;
};


class MediaPlayer:
    public PacketQueue::Observer
{
    public:
        MediaPlayer();

        virtual ~MediaPlayer();

        status_t setDataSource(const char * url);

        status_t setVideoSurface(JNIEnv * env,
                                 jobject  jsurface);

        status_t setListener(MediaPlayerListener * listener);

        status_t start();

        status_t stop();

        status_t pause();

        bool isPlaying();

        status_t getVideoWidth(int * w);

        status_t getVideoHeight(int * h);

        status_t seekTo(int msec);

        status_t getCurrentPosition(int * msec);

        status_t getDuration(int * msec);

        status_t reset();

        status_t setAudioStreamType(int type);

        status_t prepare();

        void notify(int msg,
                    int ext1 = 0,
                    int ext2 = 0);

        status_t seekTo_l(int msec);

        status_t suspend();

        status_t resume();

        // implement PacketQueue::Observer
        void onDataChanged(int evt);

    private:
        inline bool validStatus()
        {
            return ((mCurrentState != MEDIA_PLAYER_STOPPED) && (mCurrentState != MEDIA_PLAYER_STATE_ERROR));
        }

        inline bool resSufficient()
        {
            return (mHasVideo ? mVideoQueue && (mVideoQueue -> size() > FFMPEG_PLAYER_MAX_QUEUE_SIZE) : true)
                   && (mHasAudio ? mAudioQueue && (mAudioQueue -> size() > 2 * FFMPEG_PLAYER_MAX_QUEUE_SIZE) : true);
        }

        status_t prepareAudio();

        status_t prepareVideo();

        bool shouldCancel(PacketQueue * queue);

        static void ffmpegNotify(void *       ptr,
                                 int          level,
                                 const char * fmt,
                                 va_list      vl);

        static void * PlayerThreadWrapper(void * ptr);

        static void render(AVFrame * frame);

        static void render(int16_t * buffer,
                           int       buffer_size);

        void decodeMovie(void * ptr);

        void clear_l();

        double          mTime;
        pthread_mutex_t mQueueCondLock;
        pthread_cond_t  mQueueCond;
        pthread_t       mMainThread;
        Thread *        mDecoderThread;
        PacketQueue *   mVideoQueue;
        PacketQueue *   mAudioQueue;

        // Mutex                       mNotifyLock;
        // Condition                   mSignal;
        MediaPlayerListener * mListener;
        AVFormatContext *     mFormatCtx;
        int                   mAudioStreamIndex;
        int                   mVideoStreamIndex;
        DecoderAudio *        mDecoderAudio;
        DecoderVideo *        mDecoderVideo;
        AVFrame *             mFrame;

        struct SwsContext *mConvertCtx;


        bool                mHasVideo;
        bool                mHasAudio;
        void *              mCookie;
        media_player_states mCurrentState;
        int                 mDuration;
        int                 mCurrentPosition;
        int                 mSeekPosition;
        bool                mPrepareSync;
        bool                mJustSeeked;
        bool                mNeedToSeek;
        status_t            mPrepareStatus;
        int                 mStreamType;
        bool                mLoop;
        float               mLeftVolume;
        float               mRightVolume;
        int                 mVideoWidth;
        int                 mVideoHeight;
};
#endif // __MEDIAPLAYER_H


//~ Formatted by Jindent --- http://www.jindent.com
