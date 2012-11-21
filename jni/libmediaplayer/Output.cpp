
#include <android/log.h>
#include "Output.h"

#define TAG "FFMpegOutput"

// -------------------- Audio driver --------------------
int Output::AudioDriver_register()
{
    return AudioTrackWrapper_Register();
}

int Output::AudioDriver_unregister()
{
    return AudioTrackWrapper_Unregister();
}

int Output::AudioDriver_start()
{
    return AudioTrackWrapper_Start();
}

int Output::AudioDriver_set(int      streamType,
                            uint32_t sampleRate,
                            int      format,
                            int      channels)
{
    return AudioTrackWrapper_Set(streamType, sampleRate, format, channels);
}

int Output::AudioDriver_flush()
{
    return AudioTrackWrapper_Flush();
}

int Output::AudioDriver_stop()
{
    return AudioTrackWrapper_Stop();
}

int Output::AudioDriver_reload()
{
    return AudioTrackWrapper_Reload();
}

int Output::AudioDriver_write(void * buffer,
                              int    buffer_size)
{
    return AudioTrackWrapper_Write(buffer, buffer_size);
}

// -------------------- Video driver --------------------
int Output::VideoDriver_register(JNIEnv * env,
                                 jobject  jsurface)
{
    return SurfaceWrapper_Register(env, jsurface);
}

int Output::VideoDriver_unregister()
{
    return SurfaceWrapper_Unregister();
}

int Output::VideoDriver_getPixels(int     width,
                                  int     height,
                                  void ** pixels)
{
    return SurfaceWrapper_GetPixels(width, height, pixels);
}

int Output::VideoDriver_updateSurface(bool autoScale)
{
    return SurfaceWrapper_Update(autoScale);
}


//~ Formatted by Jindent --- http://www.jindent.com
