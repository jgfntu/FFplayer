/*
 * mediaplayer.cpp
 */

//#define LOG_NDEBUG 0
#define TAG "Native MediaPlayer"
#define TAG_SOURCE "Native Source Thread"
#define TAG_AUDIO "Native AudioDec Thread"
#define TAG_VIDEO "Native VideoDec Thread"


#include <sys/types.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include "mediaclock.h"

extern "C" {
#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"
#include "libavutil/avutil.h"
#include "libswscale/swscale.h"

#include "libavutil/log.h"
} // end of extern C


#include <android/log.h>

#include "mediaplayer.h"
#include "output.h"

#ifndef INT64_MIN
#define INT64_MIN       (-0x7fffffff - 1)
#endif

#ifndef INT64_MAX
#define INT64_MAX      (0x7fffffff)
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
		mPlayerThread(NULL),
		mListener(NULL),
		mCookie(NULL),
		mDuration(-1),
		mStreamType(MUSIC),
		mCurrentPosition(-1),
		mSeekPosition(-1),
		mCurrentState(MEDIA_PLAYER_IDLE),
		mPrepareSync(false),
		mPrepareStatus(NO_ERROR),
	    mLoop(false),
	    mJustSeeked(false),
	    mNeedToSeek(false)
{
    pthread_mutex_init(&mLock, NULL);
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
	__android_log_print(ANDROID_LOG_INFO, TAG, "~MediaPlayer() called!");
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
	__android_log_print(ANDROID_LOG_INFO, TAG, "prepareAudio");
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
	__android_log_print(ANDROID_LOG_INFO, TAG, "prepareVideo");
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
		return INVALID_OPERATION;
	}

	// Open codec
	if (avcodec_open(codec_ctx, codec) < 0) {
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
	mCurrentState = MEDIA_PLAYER_PREPARING;
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

    	__android_log_print(ANDROID_LOG_INFO, TAG, "The Duration of the media is %dms", mDuration);
	return NO_ERROR;
}

status_t MediaPlayer::setListener(MediaPlayerListener* listener)
{
    __android_log_print(ANDROID_LOG_INFO, TAG, "setListener");
    mListener = listener;
    return NO_ERROR;
}

status_t MediaPlayer::setDataSource(const char *url)
{
    __android_log_print(ANDROID_LOG_INFO, TAG, "setDataSource(%s)", url);
    status_t err = BAD_VALUE;
	// Open video file
    int res = av_open_input_file(&mMovieFile, url, NULL, 0, NULL);
    __android_log_print(ANDROID_LOG_INFO, TAG, "av_open_input_file(res:%d)", res);
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
	__android_log_print(ANDROID_LOG_INFO, TAG, "suspend");

	mCurrentState = MEDIA_PLAYER_STOPPED;
	if(mDecoderAudio != NULL) {
		mDecoderAudio->stop();
	}
	if(mDecoderVideo != NULL) {
		mDecoderVideo->stop();
	}

	if(pthread_join(mPlayerThread, NULL) != 0) {
		__android_log_print(ANDROID_LOG_ERROR, TAG, "Couldn't cancel player thread");
	}

	// Close the codec
	free(mDecoderAudio);
	free(mDecoderVideo);

	// Close the video file
	av_close_input_file(mMovieFile);

	//close OS drivers
	Output::AudioDriver_unregister();
	Output::VideoDriver_unregister();

	__android_log_print(ANDROID_LOG_ERROR, TAG, "suspended");

    return NO_ERROR;
}

status_t MediaPlayer::resume() {
	pthread_mutex_lock(&mLock);
	mCurrentState = MEDIA_PLAYER_STARTED;
	MediaClock::instance()->setCurClockStatus(MediaClock::CLOCK_KICKING);
	pthread_mutex_unlock(&mLock);
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
			 ((mCurrentState == MEDIA_PLAYER_DECODED || mCurrentState == MEDIA_PLAYER_STARTED)
			  && queue->size() == 0));
}

void MediaPlayer::decode(AVFrame* frame, double pts)
{
	if(FPS_DEBUGGING) {
		timeval pTime;
		static int frames = 0;
		static double t1 = -1;
		static double t2 = -1;

		gettimeofday(&pTime, NULL);
		t2 = pTime.tv_sec + (pTime.tv_usec / 1000000.0);
		if (t1 == -1 || t2 > t1 + 1) {
			__android_log_print(ANDROID_LOG_INFO, TAG, "Video fps:%i", frames);
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

void MediaPlayer::decode(int16_t* buffer, int buffer_size)
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
			__android_log_print(ANDROID_LOG_INFO, TAG, "Audio fps:%i", frames);
			//sPlayer->notify(MEDIA_INFO_FRAMERATE_AUDIO, frames, -1);
			t1 = t2;
			frames = 0;
		}
		frames++;
	}

	if(Output::AudioDriver_write(buffer, buffer_size) <= 0) {
		__android_log_print(ANDROID_LOG_ERROR, TAG, "Couldn't write samples to audio track, buffer:%p, size:%d", buffer, buffer_size);
	}
}

status_t MediaPlayer::seekTo_l(int msec) {
	__android_log_print(ANDROID_LOG_INFO, TAG, "seekTo_l %d", msec);
	int ret;
        if (mMovieFile->start_time != AV_NOPTS_VALUE)
            msec += mMovieFile->start_time;

        msec = msec * AV_TIME_BASE / 1000 ;
    	__android_log_print(ANDROID_LOG_INFO, TAG, "The final seek ts:%d", msec);

        ret = avformat_seek_file(mMovieFile, -1, INT64_MIN, msec, INT64_MAX, 0);

    	__android_log_print(ANDROID_LOG_INFO, TAG, "The ret of avformat_seek_file (%d)", ret);

        if (ret < 0) {
            __android_log_print(ANDROID_LOG_INFO, TAG, "could not seek to position %0.3f\n",
                    (double)msec);
            return UNKNOWN_ERROR;
        }

    	__android_log_print(ANDROID_LOG_INFO, TAG, "Ready to flush Video queque");

        BOS_PKT.pts = msec;

        mDecoderVideo->flushDataQueue();
		mDecoderVideo->enqueue(&BOS_PKT);


    	__android_log_print(ANDROID_LOG_INFO, TAG, "Finished flush Video queque");

    	__android_log_print(ANDROID_LOG_INFO, TAG, "Ready to flush Audio queque");

    	/*TODO: first flush the dataQueue, then push some silent data to it, so as to feed the AudioTrack continously*/
        mDecoderAudio->flushDataQueue();
        mDecoderAudio->enqueue(&BOS_PKT);

    	//__android_log_print(ANDROID_LOG_INFO, TAG, "Finished flush Audio queque");

        mJustSeeked = true;
       return NO_ERROR;
}

status_t MediaPlayer::seekTo(int msec)
{
	__android_log_print(ANDROID_LOG_INFO, TAG, "seekTo %d", msec);
    if ((sPlayer != 0) && ( mCurrentState & ( MEDIA_PLAYER_STARTED | MEDIA_PLAYER_PREPARED | MEDIA_PLAYER_PAUSED |  MEDIA_PLAYER_PLAYBACK_COMPLETE) ) ) {
        if ( msec < 0 ) {
            __android_log_print(ANDROID_LOG_INFO, TAG, "Attempt to seek to invalid position: %d", msec);
            msec = 0;
        } else if ((mDuration > 0) && (msec > mDuration)) {
            __android_log_print(ANDROID_LOG_INFO, TAG, "Attempt to seek to past end of file: request = %d, EOF = %d", msec, mDuration);
            msec = mDuration;
        }
        // cache duration
        mCurrentPosition = msec;
        if (mSeekPosition < 0) {
            mSeekPosition = msec;
            //return sPlayer->seekTo_l(msec);
        	__android_log_print(ANDROID_LOG_INFO, TAG, "setting mNeedToSeek to true!");
            mNeedToSeek = true;
            return NO_ERROR;
        }
        else {
            __android_log_print(ANDROID_LOG_INFO, TAG, "Seek in progress - queue up seekTo[%d]", msec);
            return NO_ERROR;
        }
    }
    return INVALID_OPERATION;
}

void MediaPlayer::decodeMovie(void* ptr)
{
	AVPacket pPacket;

	AVStream* stream_audio = mMovieFile->streams[mAudioStreamIndex];
	mDecoderAudio = new DecoderAudio(stream_audio);
	mDecoderAudio->onDecode = decode;
	mDecoderAudio->startAsync();

	AVStream* stream_video = mMovieFile->streams[mVideoStreamIndex];
	mDecoderVideo = new DecoderVideo(stream_video);
	mDecoderVideo->onDecode = decode;
	mDecoderVideo->startAsync();

	mCurrentState = MEDIA_PLAYER_STARTED;
	__android_log_print(ANDROID_LOG_INFO, TAG, "playing %ix%i", mVideoWidth, mVideoHeight);
	int eof = 0;

	while (mCurrentState != MEDIA_PLAYER_DECODED && mCurrentState != MEDIA_PLAYER_STOPPED &&
		   mCurrentState != MEDIA_PLAYER_STATE_ERROR)
	{
		MediaClock::instance()->waitOnClock();

		if (mDecoderVideo->packets() > FFMPEG_PLAYER_MAX_QUEUE_SIZE &&
				mDecoderAudio->packets() > 2 * FFMPEG_PLAYER_MAX_QUEUE_SIZE) {
			usleep(200);
			continue;
		}

		if (mNeedToSeek) {
			sPlayer->seekTo_l(mSeekPosition);
			Output::AudioDriver_flush();
			//Output::AudioDriver_reload();
			mNeedToSeek = false;
		}

		int ret;
		ret = av_read_frame(mMovieFile, &pPacket);
		__android_log_print(ANDROID_LOG_INFO, TAG_SOURCE, "av_read_frame ret:%d", ret);

		if (ret < 0) {
			if (ret == AVERROR_EOF || url_feof(mMovieFile->pb))
				eof = 1;
			if (mMovieFile->pb && mMovieFile->pb->error)
				eof = 1;
			if (eof) {
				mCurrentState = MEDIA_PLAYER_PLAYBACK_COMPLETE;

				__android_log_print(ANDROID_LOG_INFO, TAG_SOURCE, "enqueue EOS_PKT");
				mDecoderVideo->enqueue(&EOS_PKT);
		        mDecoderAudio->enqueue(&EOS_PKT);
				notify(MEDIA_PLAYBACK_COMPLETE);
				break;
			}
		}

		if (mJustSeeked) {
			__android_log_print(ANDROID_LOG_INFO, TAG_SOURCE, "We have JustSeeked!");
			int64_t fakeNPT = pPacket.pts;
			if (AV_NOPTS_VALUE == fakeNPT) {
				fakeNPT = pPacket.dts;
			}
			unsigned long keyTS = 1000 * fakeNPT * av_q2d(mMovieFile->streams[pPacket.stream_index]->time_base);
			__android_log_print(ANDROID_LOG_INFO, TAG_SOURCE, "set the MediaClock to %ld!", keyTS);
			MediaClock::instance()->setCurClock(keyTS);
			mJustSeeked = false;
			notify(MEDIA_SEEK_COMPLETE);
		}

		long tmppts = 1000 * pPacket.pts * av_q2d(mMovieFile->streams[pPacket.stream_index]->time_base);

		// Is this a packet from the video stream?
		if (pPacket.stream_index == mVideoStreamIndex) {
			mDecoderVideo->enqueue(&pPacket);
			__android_log_print(ANDROID_LOG_INFO, TAG_SOURCE, "read out A video packet: pts(%ld ms)", tmppts);

		}
		else if (pPacket.stream_index == mAudioStreamIndex) {
			mDecoderAudio->enqueue(&pPacket);
			__android_log_print(ANDROID_LOG_INFO, TAG_SOURCE, "read out A audio packet: pts(%ld ms)", tmppts);
		}
		else {
			// Free the packet that was allocated by av_read_frame
			av_free_packet(&pPacket);
		}
	}

	//waits on end of video thread
	__android_log_print(ANDROID_LOG_ERROR, TAG_SOURCE, "waiting on video thread");
	int ret = -1;
	if((ret = mDecoderVideo->wait()) != 0) {
		__android_log_print(ANDROID_LOG_ERROR, TAG_SOURCE, "Couldn't cancel video thread: %i", ret);
	}

	__android_log_print(ANDROID_LOG_ERROR, TAG, "waiting on audio thread");
	if((ret = mDecoderAudio->wait()) != 0) {
		__android_log_print(ANDROID_LOG_ERROR, TAG_SOURCE, "Couldn't cancel audio thread: %i", ret);
	}

	if(mCurrentState == MEDIA_PLAYER_STATE_ERROR) {
		__android_log_print(ANDROID_LOG_INFO, TAG_SOURCE, "playing err");
	}
	mCurrentState = MEDIA_PLAYER_PLAYBACK_COMPLETE;
	__android_log_print(ANDROID_LOG_INFO, TAG_SOURCE, "end of playing");
}

void* MediaPlayer::startPlayer(void* ptr)
{
    __android_log_print(ANDROID_LOG_INFO, TAG, "starting main player thread");
	attachCurrentThread();
    sPlayer->decodeMovie(ptr);
	detachCurrentThread();
}

status_t MediaPlayer::start()
{
	if (mCurrentState != MEDIA_PLAYER_PREPARED) {
		return INVALID_OPERATION;
	}
	mPlayerThread =
	pthread_create(&mPlayerThread, NULL, startPlayer, NULL);
	return NO_ERROR;
}

status_t MediaPlayer::stop()
{
	pthread_mutex_lock(&mLock);
	mCurrentState = MEDIA_PLAYER_STOPPED;
	pthread_mutex_unlock(&mLock);
    return NO_ERROR;
}

status_t MediaPlayer::pause()
{
	pthread_mutex_lock(&mLock);
	__android_log_print(ANDROID_LOG_INFO, TAG, "setting the status of MediaPlayer to %d", mCurrentState);
	mCurrentState = MEDIA_PLAYER_PAUSED;
	MediaClock::instance()->setCurClockStatus(MediaClock::CLOCK_PAUSED);
	pthread_mutex_unlock(&mLock);
	return NO_ERROR;
}

bool MediaPlayer::isPlaying()
{
    return mCurrentState == MEDIA_PLAYER_STARTED ||
		mCurrentState == MEDIA_PLAYER_DECODED;
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
	if (mCurrentState < MEDIA_PLAYER_PREPARED) {
		return INVALID_OPERATION;
	}
	*h = mVideoHeight;
    return NO_ERROR;
}

status_t MediaPlayer::getCurrentPosition(int *msec)
{
	if (mCurrentState < MEDIA_PLAYER_PREPARED) {
		return INVALID_OPERATION;
	}

	*msec = (MediaClock::instance())->getCurClock();/*av_gettime()*/;
	//__android_log_print(ANDROID_LOG_INFO, TAG, "position %i", *msec);
	return NO_ERROR;
}

status_t MediaPlayer::getDuration(int *msec)
{
	if (mCurrentState < MEDIA_PLAYER_PREPARED) {
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
			__android_log_print(ANDROID_LOG_ERROR, TAG, "AV_LOG_PANIC: %s", fmt);
			//sPlayer->mCurrentState = MEDIA_PLAYER_STATE_ERROR;
			break;

			/**
			 * Something went wrong and recovery is not possible.
			 * For example, no header was found for a format which depends
			 * on headers or an illegal combination of parameters is used.
			 */
		case AV_LOG_FATAL:
			__android_log_print(ANDROID_LOG_ERROR, TAG, "AV_LOG_FATAL: %s", fmt);
			//sPlayer->mCurrentState = MEDIA_PLAYER_STATE_ERROR;
			break;

			/**
			 * Something went wrong and cannot losslessly be recovered.
			 * However, not all future data is affected.
			 */
		case AV_LOG_ERROR:
			__android_log_print(ANDROID_LOG_ERROR, TAG, "AV_LOG_ERROR: %s", fmt);
			//sPlayer->mCurrentState = MEDIA_PLAYER_STATE_ERROR;
			break;

			/**
			 * Something somehow does not look correct. This may or may not
			 * lead to problems. An example would be the use of '-vstrict -2'.
			 */
		case AV_LOG_WARNING:
			__android_log_print(ANDROID_LOG_ERROR, TAG, "AV_LOG_WARNING: %s", fmt);
			break;

		case AV_LOG_INFO:
			__android_log_print(ANDROID_LOG_INFO, TAG, "%s", fmt);
			break;

		case AV_LOG_DEBUG:
			__android_log_print(ANDROID_LOG_DEBUG, TAG, "%s", fmt);
			break;

	}
}

void MediaPlayer::notify(int msg, int ext1, int ext2)
{
    __android_log_print(ANDROID_LOG_INFO, TAG, "message received msg=%d, ext1=%d, ext2=%d", msg, ext1, ext2);
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
            __android_log_print(ANDROID_LOG_INFO, TAG, "signal application thread");
            mPrepareSync = false;
            mPrepareStatus = NO_ERROR;
            mSignal.signal();
        }
        break;
    #endif

    case MEDIA_PLAYBACK_COMPLETE:
        __android_log_print(ANDROID_LOG_INFO, TAG, "playback complete");
        if (mCurrentState == MEDIA_PLAYER_IDLE) {
            __android_log_print(ANDROID_LOG_INFO, TAG, "playback complete in idle state");
        }
        if (!mLoop) {
            mCurrentState = MEDIA_PLAYER_PLAYBACK_COMPLETE;
        }
        break;
    case MEDIA_ERROR:
        // Always log errors.
        // ext1: Media framework error code.
        // ext2: Implementation dependant error code.
        __android_log_print(ANDROID_LOG_INFO, TAG, "error (%d, %d)", ext1, ext2);
        mCurrentState = MEDIA_PLAYER_STATE_ERROR;
        if (mPrepareSync)
        {
            __android_log_print(ANDROID_LOG_INFO, TAG, "signal application thread");
            mPrepareSync = false;
            mPrepareStatus = ext1;
            //mSignal.signal();
            send = false;
        }
        break;
    case MEDIA_INFO:
        // ext1: Media framework error code.
        // ext2: Implementation dependant error code.
        __android_log_print(ANDROID_LOG_INFO, TAG, "info/warning (%d, %d)", ext1, ext2);
        break;

    case MEDIA_SEEK_COMPLETE:
        __android_log_print(ANDROID_LOG_INFO, TAG, "Received seek complete");
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
        __android_log_print(ANDROID_LOG_INFO, TAG, "buffering %d", ext1);
        break;
    case MEDIA_SET_VIDEO_SIZE:
        __android_log_print(ANDROID_LOG_INFO, TAG, "New video size %d x %d", ext1, ext2);
        mVideoWidth = ext1;
        mVideoHeight = ext2;
        break;
    default:
        __android_log_print(ANDROID_LOG_INFO, TAG, "unrecognized message: (%d, %d, %d)", msg, ext1, ext2);
        break;
    }

    if ((mListener != 0) && send) {
       //__android_log_print(ANDROID_LOG_INFO, TAG, "callback application");
    	/*Debug!!!*/
    	//return;
        __android_log_print(ANDROID_LOG_INFO, TAG, "send the msg:%d to our listener", msg);

       mListener->notify(msg, ext1, ext2);
       //__android_log_print(ANDROID_LOG_INFO, TAG, "back from callback");
    }
}
