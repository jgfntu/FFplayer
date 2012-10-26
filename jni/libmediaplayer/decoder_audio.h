#ifndef FFMPEG_DECODER_AUDIO_H
#define FFMPEG_DECODER_AUDIO_H

#include "IDecoder.h"
using namespace ffplayer;
typedef void (*AudioDecodingHandler) (int16_t*,int);

class DecoderAudio : public IDecoder
{
public:
    static const int MAX_SKIP_TIME = 5;
    int skipTimes;
    DecoderAudio(AVStream* stream, PacketQueue *queue, Thread *loopThread);
	BuddyType getBuddyType();

    ~DecoderAudio();

    AudioDecodingHandler		onDecode;

private:
    int16_t*                    mSamples;
    int                         mSamplesSize;
    inline int 				bytesPerSample(enum SampleFormat sample_fmt);

    bool                        prepare();
    bool                        decode(void* ptr);
    bool                        process(AVPacket *packet);
};

#endif //FFMPEG_DECODER_AUDIO_H
