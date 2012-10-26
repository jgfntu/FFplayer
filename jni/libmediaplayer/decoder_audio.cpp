#include <android/log.h>
#include "decoder_audio.h"
#include <unistd.h>
#include "mediaclock.h"
#define TAG "FFMpegAudioDecoder"
int DecoderAudio::skipTimes = 0;
using IDecoder::BuddyType;
using namespace ffplayer;
DecoderAudio::DecoderAudio(AVStream* stream, PacketQueue *queue,
		Thread *loopThread) :
		IDecoder(stream, queue, loopThread) {
}

DecoderAudio::~DecoderAudio() {
	MediaClock::destroy();
}

BuddyType getBuddyType() {
	return BuddyType::INITIATOR;
}

bool DecoderAudio::prepare() {
	mSamplesSize = AVCODEC_MAX_AUDIO_FRAME_SIZE;
	mSamples = (int16_t *) av_malloc(mSamplesSize);

	if (mSamples == NULL) {
		return false;
	}
	MediaClock::instance();
	return true;
}

bool DecoderAudio::process(AVPacket *packet) {
	/* Process some special Events
	 * 1) BOS
	 * 2) EOS
	 */
	if (&BOS_PKT == packet) {
		LOGI("Audio Decoder Received BOS PKT!");
		/*update our clock*/
		packet->pts;
		return true;
	} else if (&EOS_PKT == packet) {
		LOGI("Audio Decoder Received EOS PKT!");
		return false;
	}

	int size = mSamplesSize;
	//for (;;) {
	//TODO: this packet may not be decompressed completely!
	int len = avcodec_decode_audio3(mStream->codec, mSamples, &size, packet);

	if ((len < 0) && (skipTimes++ > MAX_SKIP_TIME)) {
		/* if error, we skip the frame */
		return false;
	}

	LOGD("Audio Decoder Thread Processing");

	//}

	/* if no pts, then compute it */
	AVCodecContext *dec = mStream->codec;
	unsigned long tsMs = packet->pts * av_q2d(mStream->time_base) * 1000;
	unsigned long timeDelta = 0;
	if (packet->pts != AV_NOPTS_VALUE) {
		timeDelta = (tsMs - (MediaClock::instance())->getCurClock());
		unsigned long usecs = timeDelta * 1000;

		if (0 < usecs && usecs < 100000) {
			/* substract the render time diff */
			usleep(usecs - MediaClock::RENDER_DELAY * 1000 * 0.5);
		}
		(MediaClock::instance())->setCurClock(tsMs);

		onDecode(mSamples, size);
		//call handler for posting buffer to os audio driver
	} else {
		timeDelta = 1000
				* ((double) size
						/ (dec->channels * dec->sample_rate
								* bytesPerSample(dec->sample_fmt)));

		onDecode(mSamples, size);
		unsigned long usecs = timeDelta * 1000;

		if (0 < usecs && usecs < 100000) {
			/* substract the render time diff */
			usleep(usecs - MediaClock::RENDER_DELAY * 1000 * 0.5);
		}

		if (timeDelta > 0.0) {
			(MediaClock::instance())->incClockBy(timeDelta);
		}
	}

	return true;
}

int DecoderAudio::bytesPerSample(enum SampleFormat format) {
	switch (format) {
	case SAMPLE_FMT_U8:
		return 1;
	case SAMPLE_FMT_S16:
		return 2;
	case SAMPLE_FMT_S32:
		return 4;
	default:
		return 2;
	}
}

bool DecoderAudio::decode(void* ptr) {
	AVPacket pPacket;
	LOGD("decoding audio");
	MediaClock::instance()->waitOnClock();
	LOGD("AudioQueue get start()");
	if (getPktQueque() && getPktQueque()->get(&pPacket, true) < 0) {
		LOGE("getAudio Packet error, thread exit!");

		return false;
	}LOGD("AudioQueue get end()");

	if (!process(&pPacket)) {
		LOGD("Audio process error, thread exit!");
		return false;
	}
	// Free the packet that was allocated by av_read_frame
	av_free_packet(&pPacket);
	LOGD("decoding audio ended");
	// Free audio samples buffer
	av_free(mSamples);
	return true;
}
