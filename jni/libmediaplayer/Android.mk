LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

LOCAL_CFLAGS := -D__STDC_CONSTANT_MACROS

LOCAL_C_INCLUDES += \
    $(LOCAL_PATH)/../libav \
    $(LOCAL_PATH)/../include \
    $(LOCAL_PATH)/../prebuilt/librender

LOCAL_SRC_FILES += \
    PacketQueue.cpp \
    Output.cpp \
    MediaPlayer.cpp \
    DecoderAudio.cpp \
    DecoderVideo.cpp \
    Thread.cpp
    
LOCAL_MODULE := libmediaplayer

include $(BUILD_STATIC_LIBRARY)