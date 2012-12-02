LOCAL_PATH := $(call my-dir)
    
include $(CLEAR_VARS)

LOCAL_CFLAGS := -D__STDC_CONSTANT_MACROS

LOCAL_C_INCLUDES += \
    $(LOCAL_PATH)/../libav \
    $(LOCAL_PATH)/../libmediaplayer \
    $(LOCAL_PATH)/../include

LOCAL_SRC_FILES := \
    onLoad.cpp \
    com_media_ffmpeg_FFMpegPlayer.cpp

LOCAL_PRELINK_MODULE := false

LOCAL_LDLIBS := -llog -lz

LOCAL_SHARED_LIBRARIES := libjniaudio \
    libjnivideo

LOCAL_WHOLE_STATIC_LIBRARIES := \
    libavcodec \
    libavformat \
    libavutil \
    libswscale \
    libmediaplayer 

LOCAL_MODULE := libffmpeg_jni

include $(BUILD_SHARED_LIBRARY)
