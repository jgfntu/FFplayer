/*
 * MediaPlayer.cpp
 */

#include <sys/types.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
extern "C" {
#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"
#include "libavutil/avutil.h"
#include "libswscale/swscale.h"
#include "libavutil/log.h"
} // extern C

#define LOG_TAG "MediaPlayer"
#include <common/logwrapper.h>
#include "Thread.h"
#include "MediaPlayer.h"
#include "output.h"
using namespace ffplayer;
#ifndef INT64_MIN
#define INT64_MIN       (-0x7fffffff - 1)
#endif

#ifndef INT64_MAX
#define INT64_MAX      (0x7fffffff)
#endif

#if defined(free)
#undef free
#endif

#define FPS_DEBUGGING false

static MediaPlayer* sPlayer;
AVPacket BOS_PKT;
AVPacket EOS_PKT;
#define DELETE(obj)     \
	do {                \
		if (obj) {      \
			delete obj; \
			obj = NULL; \
		}               \
	} while (0);

MediaPlayer::MediaPlayer():
		mMainThread(NULL),
		mListener(NULL),
		mCookie(NULL),
		mDuration(-1),
		mAudioStreamIndex(-1),
		mVideoStreamIndex(-1),
		mStreamType(MUSIC),
		mCurrentPosition(-1),
		mSeekPosition(-1),
		mCurrentState(MEDIA_PLAYER_IDLE),
		mPrepareSync(false),
		mPrepareStatus(NO_ERROR),
	    mLoop(false),
	    mJustSeeked(false),
	    mNeedToSeek(false),
	    mVideoQueue(NULL),
	    mAudioQueue(NULL),
	    mHasAudio(false),
	    mHasVideo(false)
{
    pthread_mutex_init(&mQueueCondLock, NULL);
    pthread_cond_init(&mQueueCond, NULL);
    mLeftVolume = mRightVolume = 1.0;
    mVideoWidth = mVideoHeight = 0;
    sPlayer = this;
    av_init_packet(&BOS_PKT);
    BOS_PKT.data= reinterpret_cast<uint8_t *> (const_cast<char *> ("BOS_PACKET"));
    av_init_packet(&EOS_PKT);
    EOS_PKT.data= reinterpret_cast<uint8_t *> (const_cast<char *> ("BOS_PACKET"));
}


MediaPlayer::~MediaPlayer()
{
	if(mListener != NULL) {
		free(mListener);
	}
	DELETE(mDecoderVideo);
	DELETE(mDecoderAudio);
	pthread_mutex_destroy(&mQueueCondLock);
	pthread_cond_destroy(&mQueueCond);
	LOGI("~MediaPlayer() called!");
}

void MediaPlayer::clear_l()
{
    mDuration = -1;
    mCurrentPosition = -1;
    mSeekPosition = -1;
    mVideoWidth = mVideoHeight = 0;
}

status_t MediaPlayer::prepareAudio()
{
	LOGD("TAG, prepareAudio");
	mAudioStreamIndex = -1;
	for (int i = 0; i < mMovieFile->nb_streams; i++) {
		if (mMovieFile->streams[i]->codec->codec_type == CODEC_TYPE_AUDIO) {
			mAudioStreamIndex = i;
			break;
		}
	}

	if (mAudioStreamIndex == -1) {
		return INVALID_OPERATION;
	}

	AVStream* stream = mMovieFile->streams[mAudioStreamIndex];
	// Get a pointer to the codec context for the video stream
	AVCodecContext* codec_ctx = stream->codec;
	AVCodec* codec = avcodec_find_decoder(codec_ctx->codec_id);
	if (codec == NULL) {
		return INVALID_OPERATION;
	}

	// Open codec
	if (avcodec_open(codec_ctx, codec) < 0) {
		return INVALID_OPERATION;
	}

	// prepare os output
	if (Output::AudioDriver_set(MUSIC,
								stream->codec->sample_rate,
								PCM_16_BIT,
								(stream->codec->channels == 2) ? CHANNEL_OUT_STEREO
										: CHANNEL_OUT_MONO) != ANDROID_AUDIOTRACK_RESULT_SUCCESS) {
		return INVALID_OPERATION;
	}

	if (Output::AudioDriver_start() != ANDROID_AUDIOTRACK_RESULT_SUCCESS) {
		return INVALID_OPERATION;
	}

	return NO_ERROR;
}

status_t MediaPlayer::prepareVideo()
{
	LOGD("prepareVideo");
	// Find the first video stream
	mVideoStreamIndex = -1;
	for (int i = 0; i < mMovieFile->nb_streams; i++) {
		if (mMovieFile->streams[i]->codec->codec_type == CODEC_TYPE_VIDEO) {
			mVideoStreamIndex = i;
			break;
		}
	}

	if (mVideoStreamIndex == -1) {
		return INVALID_OPERATION;
	}

	AVStream* stream = mMovieFile->streams[mVideoStreamIndex];
	// Get a pointer to the codec context for the video stream
	AVCodecContext* codec_ctx = stream->codec;
	AVCodec* codec = avcodec_find_decoder(codec_ctx->codec_id);
	if (codec == NULL) {
		LOGE("Video codec cannot be found!");
		return INVALID_OPERATION;
	}

	// Open codec
	if (avcodec_open(codec_ctx, codec) < 0) {
		LOGE("Video codec cannot be open success!!");
		return INVALID_OPERATION;
	}

	mVideoWidth = codec_ctx->width;
	mVideoHeight = codec_ctx->height;
	mDuration =  mMovieFile->duration / AV_TIME_BASE * 1000;

	mConvertCtx = sws_getContext(stream->codec->width,
								 stream->codec->height,
								 stream->codec->pix_fmt,
								 stream->codec->width,
								 stream->codec->height,
								 PIX_FMT_RGB565,
								 SWS_POINT,
								 NULL,
								 NULL,
								 NULL);

	if (mConvertCtx == NULL) {
		return INVALID_OPERATION;
	}

	void*		pixels;
	if (Output::VideoDriver_getPixels(stream->codec->width,
									  stream->codec->height,
									  &pixels) != ANDROID_SURFACE_RESULT_SUCCESS) {
		return INVALID_OPERATION;
	}

	mFrame = avcodec_alloc_frame();
	if (mFrame == NULL) {
		return INVALID_OPERATION;
	}
	// Assign appropriate parts of buffer to image planes in pFrameRGB
	// Note that pFrameRGB is an AVFrame, but AVFrame is a superset
	// of AVPicture
	avpicture_fill((AVPicture *) mFrame,
				   (uint8_t *) pixels,
				   PIX_FMT_RGB565,
				   stream->codec->width,
				   stream->codec->height);

	return NO_ERROR;
}

status_t MediaPlayer::prepare()
{
	status_t ret;
	av_log_set_callback(ffmpegNotify);
	if ((ret = prepareVideo()) != NO_ERROR) {
		mCurrentState = MEDIA_PLAYER_STATE_ERROR;
		return ret;
	}
	if ((ret = prepareAudio()) != NO_ERROR) {
		mCurrentState = MEDIA_PLAYER_STATE_ERROR;
		return ret;
	}
	mCurrentState = MEDIA_PLAYER_PREPARED;
	/* in the unit of ms */
	mDuration = mMovieFile->duration / AV_TIME_BASE * 1000;
	AVStream *stream = mMovieFile->streams[mVideoStreamIndex];
	int duration = stream->duration * av_q2d(stream->time_base) * 1000;
	mDuration = mDuration > duration ? mDuration : duration;
	stream = mMovieFile->streams[mAudioStreamIndex];
	duration = stream->duration * av_q2d(stream->time_base) * 1000;
	mDuration = mDuration > duration ? mDuration : duration;

    LOGD("The Duration of the media is %dms", mDuration);
	return NO_ERROR;
}

status_t MediaPlayer::setListener(MediaPlayerListener* listener)
{
	LOGD("setListener");
    mListener = listener;
    return NO_ERROR;
}

status_t MediaPlayer::setDataSource(const char *url)
{
	LOGD("setDataSource(%s)", url);
    status_t err = BAD_VALUE;
	// Open video file
    int res = av_open_input_file(&mMovieFile, url, NULL, 0, NULL);
	LOGD("av_open_input_file(res:%d)", res);

	if(res != 0) {
		return INVALID_OPERATION;
	}
	// Retrieve stream information
	if(av_find_stream_info(mMovieFile) < 0) {
		return INVALID_OPERATION;
	}

	mCurrentState = MEDIA_PLAYER_INITIALIZED;
    return NO_ERROR;
}

status_t MediaPlayer::suspend() {
	LOGD("suspend");

	pthread_mutex_lock(&mQueueCondLock);
	mCurrentState = MEDIA_PLAYER_STOPPED;
	pthread_cond_signal(&mQueueCond);
	pthread_mutex_unlock(&mQueueCondLock);
	if(mDecoderAudio != NULL) {
		mDecoderAudio->stop();
	}
	if(mDecoderVideo != NULL) {
		mDecoderVideo->stop();
	}

	if(pthread_join(mMainThread, NULL) != 0) {
		LOGE("Couldn't cancel player thread");
	}

	// Close the codec
	free(mDecoderAudio);
	free(mDecoderVideo);

	// Close the video file
	av_close_input_file(mMovieFile);

	//close OS drivers
	Output::AudioDriver_unregister();
	Output::VideoDriver_unregister();
	LOGD("suspended");
    return NO_ERROR;
}

status_t MediaPlayer::resume() {
	mCurrentState = MEDIA_PLAYER_RUNNING;
    return NO_ERROR;
}

status_t MediaPlayer::setVideoSurface(JNIEnv* env, jobject jsurface)
{
	if(Output::VideoDriver_register(env, jsurface) != ANDROID_SURFACE_RESULT_SUCCESS) {
		return INVALID_OPERATION;
	}
	if(Output::AudioDriver_register() != ANDROID_AUDIOTRACK_RESULT_SUCCESS) {
		return INVALID_OPERATION;
	}
    return NO_ERROR;
}

bool MediaPlayer::shouldCancel(PacketQueue* queue)
{
	return (mCurrentState == MEDIA_PLAYER_STATE_ERROR || mCurrentState == MEDIA_PLAYER_STOPPED ||
			 ((mCurrentState == MEDIA_PLAYER_RUNNING)
			  && queue->size() == 0));
}

void MediaPlayer::render(AVFrame* frame)
{
	if(FPS_DEBUGGING) {
		timeval pTime;
		static int frames = 0;
		static double t1 = -1;
		static double t2 = -1;

		gettimeofday(&pTime, NULL);
		t2 = pTime.tv_sec + (pTime.tv_usec / 1000000.0);
		if (t1 == -1 || t2 > t1 + 1) {
			LOGI("Video fps:%i", frames);
			//sPlayer->notify(MEDIA_INFO_FRAMERATE_VIDEO, frames, -1);
			t1 = t2;
			frames = 0;
		}
		frames++;
	}

	// Convert the image from its native format to RGB
	sws_scale(sPlayer->mConvertCtx,
		      frame->data,
		      frame->linesize,
			  0,
			  sPlayer->mVideoHeight,
			  sPlayer->mFrame->data,
			  sPlayer->mFrame->linesize);

	Output::VideoDriver_updateSurface();
}

void MediaPlayer::render(int16_t* buffer, int buffer_size)
{
	if (0 == buffer_size || NULL == buffer) {
		return;
	}

	if(FPS_DEBUGGING) {
		timeval pTime;
		static int frames = 0;
		static double t1 = -1;
		static double t2 = -1;

		gettimeofday(&pTime, NULL);
		t2 = pTime.tv_sec + (pTime.tv_usec / 1000000.0);
		if (t1 == -1 || t2 > t1 + 1) {
			LOGD("Audio fps:%i", frames);
			//sPlayer->notify(MEDIA_INFO_FRAMERATE_AUDIO, frames, -1);
			t1 = t2;
			frames = 0;
		}
		frames++;
	}

	if(Output::AudioDriver_write(buffer, buffer_size) <= 0) {
		LOGE("Couldn't write samples to audio track, buffer:%p, size:%d", buffer, buffer_size);
	}
}

status_t MediaPlayer::seekTo_l(int msec) {
	LOGD("seekTo_l %d", msec);
	int ret;
        if (mMovieFile->start_time != AV_NOPTS_VALUE)
            msec += mMovieFile->start_time;

        msec = msec * AV_TIME_BASE / 1000 ;
    	LOGD("The final seek ts:%d", msec);

        ret = avformat_seek_file(mMovieFile, -1, INT64_MIN, msec, INT64_MAX, 0);

    	LOGD("The ret of avformat_seek_file (%d)", ret);

        if (ret < 0) {
            LOGE("could not seek to position %0.3f\n",
                    (double)msec);
            return UNKNOWN_ERROR;
        }

    	LOGD("Ready to flush Video queque");

        BOS_PKT.pts = msec;
        if (mDecoderVideo)
        mDecoderVideo->flush();
		mVideoQueue->put(&BOS_PKT);
    	LOGD("Finished flush Video queque");
    	LOGD("Ready to flush Audio queque");
    	/*TODO: first flush the dataQueue, then push some silent data to it, so as to feed the AudioTrack continously*/
        mDecoderAudio->flush();
        mAudioQueue->put(&BOS_PKT);

    	LOGD("Finished flush Audio queque");
        mJustSeeked = true;
        mSeekPosition = -1;
       return NO_ERROR;
}

status_t MediaPlayer::seekTo(int msec)
{
	LOGE("MediaPlayer::seekTo %d", msec);
    if (mCurrentState == MEDIA_PLAYER_RUNNING || mCurrentState == MEDIA_PLAYER_PAUSED) {
        if (msec < 0) {
            LOGE("Attempt to seek to invalid position: %d", msec);
            msec = 0;
        } else if ((mDuration > 0) && (msec > mDuration)) {
            LOGD("Attempt to seek to past end of file: request = %d, EOF = %d", msec, mDuration);
            msec = mDuration;
        }
        // cache duration
        mCurrentPosition = msec;
        if (mSeekPosition < 0) {
            mSeekPosition = msec;
            //return sPlayer->seekTo_l(msec);
        	LOGD("setting mNeedToSeek to true!");
            mNeedToSeek = true;
            return NO_ERROR;
        }
        else {
            LOGD("Seek in progress - queue up seekTo[%d]", msec);
            return NO_ERROR;
        }
    }
    return INVALID_OPERATION;
}

// through this may be have some concurrent problem,
// but it's accurate for media playback need
void MediaPlayer::onDataChanged(int evt) {
	if (evt == PacketQueue::Observer::DATA_CONSUMED) {
		pthread_mutex_lock(&mQueueCondLock);
		pthread_cond_signal(&mQueueCond);
		pthread_mutex_unlock(&mQueueCondLock);
	}
}

void MediaPlayer::decodeMovie(void* ptr)
{
	AVPacket pPacket;

	if (-1 == mAudioStreamIndex) {
		return;
	}
	mHasAudio = true;

	AVStream* stream_audio = mMovieFile->streams[mAudioStreamIndex];
	mDecoderThread = new Thread();

	mDecoderAudio = new DecoderAudio(stream_audio, mAudioQueue = new PacketQueue(this), mDecoderThread);

	mDecoderAudio->rendorHook = render;
	if (-1 != mVideoStreamIndex) {
		mHasVideo = true;
		AVStream* stream_video = mMovieFile->streams[mVideoStreamIndex];
		mDecoderVideo = new DecoderVideo(stream_video, mVideoQueue = new PacketQueue(this), NULL);
		mDecoderVideo->rendorHook = render;
		mDecoderAudio->bindBuddy(mDecoderVideo);
		mDecoderVideo->start();
	}
	mDecoderAudio->start();

	mCurrentState = MEDIA_PLAYER_RUNNING;
	LOGD("playing dimensions%ix%i", mVideoWidth, mVideoHeight);
	int eof = 0;
	while (validStatus())
	{
		pthread_mutex_lock(&mQueueCondLock);
		// when mCurrentState set to stopped, need to quit the Main PlayerThread
		// so cancel wait operation
		while ((mCurrentState != MEDIA_PLAYER_STOPPED) && resSufficient()) {
			pthread_cond_wait(&mQueueCond, &mQueueCondLock);
		}
		pthread_mutex_unlock(&mQueueCondLock);

		if (mNeedToSeek) {
			seekTo_l(mSeekPosition);
			Output::AudioDriver_flush();
			//Output::AudioDriver_reload();
			mNeedToSeek = false;
		}

		int ret;
		ret = av_read_frame(mMovieFile, &pPacket);
		LOGD("av_read_frame ret:%d", ret);

		if (ret < 0) {
			if (ret == AVERROR_EOF || url_feof(mMovieFile->pb))
				eof = 1;
			if (mMovieFile->pb && mMovieFile->pb->error)
				eof = 1;
			if (eof) {
				mCurrentState = MEDIA_PLAYER_PLAYBACK_COMPLETE;
				LOGD("enqueue EOS_PKT");
				if (mAudioQueue)
					mAudioQueue->put(&EOS_PKT);
				if (mVideoQueue)
					mVideoQueue->put(&EOS_PKT);
				notify(MEDIA_PLAYBACK_COMPLETE);
				break;
			}
		}

		if (mJustSeeked) {
			LOGD("We have JustSeeked!");
			int64_t newNPT = pPacket.pts;
			if (AV_NOPTS_VALUE == newNPT) {
				newNPT = pPacket.dts;
			}
			if (AV_NOPTS_VALUE == newNPT) {
				mJustSeeked = false;
				continue;
			}
			unsigned long keyTS = 1000 * newNPT * av_q2d(mMovieFile->streams[pPacket.stream_index]->time_base);
			if (mDecoderAudio) {
				mDecoderAudio->setRealTimeMS(keyTS);
			}
			mJustSeeked = false;
			notify(MEDIA_SEEK_COMPLETE);
		}

		long tmppts = 1000 * pPacket.pts * av_q2d(mMovieFile->streams[pPacket.stream_index]->time_base);
		// Is this a packet from the video stream?
		if (pPacket.stream_index == mVideoStreamIndex && mVideoQueue) {
			mVideoQueue->put(&pPacket);
			LOGD("read out A video packet: pts(%ld ms)", tmppts);
		}
		else if (pPacket.stream_index == mAudioStreamIndex && mAudioQueue) {
			mAudioQueue->put(&pPacket);
			LOGD("read out A audio packet: pts(%ld ms)", tmppts);
		}
		else {
			// Free the packet that was allocated by av_read_frame
			av_free_packet(&pPacket);
		}
	}
}

void* MediaPlayer::PlayerThreadWrapper(void* ptr)
{
    LOGD("starting main player thread");
	attachCurrentThread();
    sPlayer->decodeMovie(ptr);
	detachCurrentThread();
}

status_t MediaPlayer::start()
{
	if (MEDIA_PLAYER_STOPPED == mCurrentState || MEDIA_PLAYER_PAUSED == mCurrentState) {
		if (mDecoderAudio) {
			mDecoderAudio->start();
			pthread_mutex_lock(&mQueueCondLock);
			mCurrentState = MEDIA_PLAYER_RUNNING;
			pthread_cond_signal(&mQueueCond);
			pthread_mutex_unlock(&mQueueCondLock);
		}
	}
	if (mCurrentState != MEDIA_PLAYER_PREPARED) {
		return INVALID_OPERATION;
	}
	return pthread_create(&mMainThread, NULL, PlayerThreadWrapper, NULL);
}

status_t MediaPlayer::stop()
{
	pthread_mutex_lock(&mQueueCondLock);
	mCurrentState = MEDIA_PLAYER_STOPPED;
	pthread_cond_signal(&mQueueCond);
	pthread_mutex_unlock(&mQueueCondLock);
    return NO_ERROR;
}

status_t MediaPlayer::pause()
{
	pthread_mutex_lock(&mQueueCondLock);
	if (mDecoderAudio) {
		mDecoderAudio->pause();
		mCurrentState = MEDIA_PLAYER_PAUSED;
	}
	pthread_cond_signal(&mQueueCond);
	pthread_mutex_unlock(&mQueueCondLock);
	return NO_ERROR;
}

bool MediaPlayer::isPlaying()
{
    return mCurrentState == MEDIA_PLAYER_RUNNING;
}

status_t MediaPlayer::getVideoWidth(int *w)
{
	if (mCurrentState < MEDIA_PLAYER_PREPARED) {
		return INVALID_OPERATION;
	}
	*w = mVideoWidth;
    return NO_ERROR;
}

status_t MediaPlayer::getVideoHeight(int *h)
{
	if (mCurrentState < MEDIA_PLAYER_PREPARED || !h) {
		return INVALID_OPERATION;
	}
	*h = mVideoHeight;
    return NO_ERROR;
}

status_t MediaPlayer::getCurrentPosition(int *msec)
{
	if (mCurrentState < MEDIA_PLAYER_PREPARED || !msec || !mDecoderAudio) {
		return INVALID_OPERATION;
	}

	*msec = mDecoderAudio->getRealTimeMS();/*av_gettime()*/;
	return NO_ERROR;
}

status_t MediaPlayer::getDuration(int *msec)
{
	if (mCurrentState < MEDIA_PLAYER_PREPARED || !msec) {
		return INVALID_OPERATION;
	}
	*msec = mDuration;
    return NO_ERROR;
}

status_t MediaPlayer::reset()
{
    return INVALID_OPERATION;
}

status_t MediaPlayer::setAudioStreamType(int type)
{
	return NO_ERROR;
}

void MediaPlayer::ffmpegNotify(void* ptr, int level, const char* fmt, va_list vl) {

	switch(level) {
			/**
			 * Something went really wrong and we will crash now.
			 */
		case AV_LOG_PANIC:
			LOGE("AV_LOG_PANIC: %s", fmt);
			//sPlayer->mCurrentState = MEDIA_PLAYER_STATE_ERROR;
			break;

			/**
			 * Something went wrong and recovery is not possible.
			 * For example, no header was found for a format which depends
			 * on headers or an illegal combination of parameters is used.
			 */
		case AV_LOG_FATAL:
			LOGE("AV_LOG_FATAL: %s", fmt);
			//sPlayer->mCurrentState = MEDIA_PLAYER_STATE_ERROR;
			break;

			/**
			 * Something went wrong and cannot losslessly be recovered.
			 * However, not all future data is affected.
			 */
		case AV_LOG_ERROR:
			LOGE("AV_LOG_ERROR: %s", fmt);
			//sPlayer->mCurrentState = MEDIA_PLAYER_STATE_ERROR;
			break;

			/**
			 * Something somehow does not look correct. This may or may not
			 * lead to problems. An example would be the use of '-vstrict -2'.
			 */
		case AV_LOG_WARNING:
			LOGE("AV_LOG_WARNING: %s", fmt);
			break;

		case AV_LOG_INFO:
			LOGI("%s", fmt);
			break;

		case AV_LOG_DEBUG:
			LOGD("%s", fmt);
			break;
	}
}

void MediaPlayer::notify(int msg, int ext1, int ext2)
{
    LOGD("message received msg=%d, ext1=%d, ext2=%d", msg, ext1, ext2);
    bool send = true;
    bool locked = false;

    switch (msg) {
    #if 0
    case MEDIA_NOP: // interface test message
        break;
    case MEDIA_PREPARED:
        __android_log_print(ANDROID_LOG_INFO, TAG, "prepared");
        mCurrentState = MEDIA_PLAYER_PREPARED;
        if (mPrepareSync) {
            LOGD("signal application thread");
            mPrepareSync = false;
            mPrepareStatus = NO_ERROR;
            mSignal.signal();
        }
        break;
    #endif

    case MEDIA_PLAYBACK_COMPLETE:
        LOGD("playback complete");
        if (mCurrentState == MEDIA_PLAYER_IDLE) {
            LOGD("playback complete in idle state");
        }
        if (!mLoop) {
            mCurrentState = MEDIA_PLAYER_PLAYBACK_COMPLETE;
        }
        break;
    case MEDIA_ERROR:
        // Always log errors.
        // ext1: Media framework error code.
        // ext2: Implementation dependant error code.
        LOGD("error (%d, %d)", ext1, ext2);
        mCurrentState = MEDIA_PLAYER_STATE_ERROR;
        if (mPrepareSync)
        {
            LOGD("signal application thread");
            mPrepareSync = false;
            mPrepareStatus = ext1;
            //mSignal.signal();
            send = false;
        }
        break;
    case MEDIA_INFO:
        // ext1: Media framework error code.
        // ext2: Implementation dependant error code.
        LOGD("info/warning (%d, %d)", ext1, ext2);
        break;

    case MEDIA_SEEK_COMPLETE:
        LOGD("Received seek complete");
        {
        	/*
			float drift = abs(mSeekPosition - mCurrentPosition) / mDuration;
			__android_log_print(
					ANDROID_LOG_INFO,
					TAG,
					"last seek drift:%f", drift);
			if (drift > 0.5) {
				__android_log_print(ANDROID_LOG_INFO, TAG,
						"Executing queued seekTo(%d) drift(%d)", mSeekPosition,
						drift);
				seekTo(mSeekPosition);
			} else {
				__android_log_print(
						ANDROID_LOG_INFO,
						TAG,
						"All seeks complete - return to regularly scheduled program");
				mCurrentPosition = mSeekPosition = -1;
			}
			*/
		}
		mCurrentPosition = mSeekPosition = -1;
        break;
    case MEDIA_BUFFERING_UPDATE:
        LOGD("buffering %d", ext1);
        break;
    case MEDIA_SET_VIDEO_SIZE:
        LOGD("New video size %d x %d", ext1, ext2);
        mVideoWidth = ext1;
        mVideoHeight = ext2;
        break;
    default:
        LOGD("unrecognized message: (%d, %d, %d)", msg, ext1, ext2);
        break;
    }

    if ((mListener != 0) && send) {
       //__android_log_print(ANDROID_LOG_INFO, TAG, "callback application");
    	/*Debug!!!*/
    	//return;
        LOGD("send the msg:%d to our listener", msg);
       mListener->notify(msg, ext1, ext2);
       //__android_log_print(ANDROID_LOG_INFO, TAG, "back from callback");
    }
}
