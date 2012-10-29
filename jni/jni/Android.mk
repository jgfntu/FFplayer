LOCAL_PATH := $(call my-dir)
#common shared library
include $(CLEAR_VARS)

IN_NDK := true

LOCAL_CFLAGS := -D__STDC_CONSTANT_MACRO

ifeq ($(IN_NDK),true)	
LOCAL_LDLIBS := -llog
else
LOCAL_PRELINK_MODULE := false
LOCAL_SHARED_LIBRARIES := liblog
endif

LOCAL_SHARED_LIBRARIES := liblog
LOCAL_WHOLE_STATIC_LIBRARIES := libavcodec libavformat libavutil libpostproc libswscale

LOCAL_MODULE := libffmpeg
include $(BUILD_SHARED_LIBRARY)

#default libffmpeg_jni.so version(for Froyo OS)
include $(CLEAR_VARS)

IN_NDK := true

LOCAL_CFLAGS := -D__STDC_CONSTANT_MACROS

WITH_CONVERTOR := true
WITH_PLAYER := true

ifeq ($(WITH_PLAYER),true)
LOCAL_CFLAGS += -DBUILD_WITH_PLAYER
endif

ifeq ($(WITH_CONVERTOR),true)
LOCAL_CFLAGS += -DBUILD_WITH_CONVERTOR
endif

LOCAL_C_INCLUDES += \
    $(LOCAL_PATH)/../libffmpeg \
    $(LOCAL_PATH)/../libmediaplayer \
    $(LOCAL_PATH)/../include

LOCAL_SRC_FILES := \
    onLoad.cpp \
    com_media_ffmpeg_FFMpegAVFrame.cpp \
    com_media_ffmpeg_FFMpegAVInputFormat.c \
    com_media_ffmpeg_FFMpegAVRational.c \
    com_media_ffmpeg_FFMpegAVFormatContext.c \
    com_media_ffmpeg_FFMpegAVCodecContext.cpp \
    com_media_ffmpeg_FFMpegUtils.cpp

ifeq ($(WITH_CONVERTOR),true)
LOCAL_SRC_FILES += \
    com_media_ffmpeg_FFMpeg.c \
    ../libffmpeg/cmdutils.c
endif
		
ifeq ($(WITH_PLAYER),true)
LOCAL_SRC_FILES += \
    com_media_ffmpeg_FFMpegPlayer.cpp
#com_media_ffmpeg_android_FFMpegPlayerAndroid.cpp
endif

ifeq ($(IN_NDK),true)	
LOCAL_LDLIBS := -llog
else
LOCAL_PRELINK_MODULE := false
LOCAL_SHARED_LIBRARIES := liblog
endif

LOCAL_SHARED_LIBRARIES := libjniaudio libjnivideo liblog libffmpeg
LOCAL_STATIC_LIBRARIES := libmediaplayer
LOCAL_MODULE := libffmpeg_jni8
include $(BUILD_SHARED_LIBRARY)

#Another libffmpeg_jni.so version(for GingerBread OS)
include $(CLEAR_VARS)

IN_NDK := true

LOCAL_CFLAGS := -D__STDC_CONSTANT_MACROS

WITH_CONVERTOR := true
WITH_PLAYER := true

ifeq ($(WITH_PLAYER),true)
LOCAL_CFLAGS += -DBUILD_WITH_PLAYER
endif

ifeq ($(WITH_CONVERTOR),true)
LOCAL_CFLAGS += -DBUILD_WITH_CONVERTOR
endif

LOCAL_C_INCLUDES += \
    $(LOCAL_PATH)/../libffmpeg \
    $(LOCAL_PATH)/../libmediaplayer \
    $(LOCAL_PATH)/../include

LOCAL_SRC_FILES := \
    onLoad.cpp \
    com_media_ffmpeg_FFMpegAVFrame.cpp \
    com_media_ffmpeg_FFMpegAVInputFormat.c \
    com_media_ffmpeg_FFMpegAVRational.c \
    com_media_ffmpeg_FFMpegAVFormatContext.c \
    com_media_ffmpeg_FFMpegAVCodecContext.cpp \
    com_media_ffmpeg_FFMpegUtils.cpp

ifeq ($(WITH_CONVERTOR),true)
LOCAL_SRC_FILES += \
    com_media_ffmpeg_FFMpeg.c \
    ../libffmpeg/cmdutils.c
endif
		
ifeq ($(WITH_PLAYER),true)
LOCAL_SRC_FILES += \
    com_media_ffmpeg_FFMpegPlayer.cpp
endif

ifeq ($(IN_NDK),true)	
LOCAL_LDLIBS := -llog
else
LOCAL_PRELINK_MODULE := false
LOCAL_SHARED_LIBRARIES := liblog
endif

LOCAL_SHARED_LIBRARIES := libjniaudio9 libjnivideo9 liblog libffmpeg
LOCAL_STATIC_LIBRARIES := libmediaplayer

LOCAL_MODULE := libffmpeg_jni9
include $(BUILD_SHARED_LIBRARY)

# Another libffmpeg_jni.so version(for ICS OS)
include $(CLEAR_VARS)

IN_NDK := true

LOCAL_CFLAGS := -D__STDC_CONSTANT_MACROS

WITH_CONVERTOR := true
WITH_PLAYER := true

ifeq ($(WITH_PLAYER),true)
LOCAL_CFLAGS += -DBUILD_WITH_PLAYER
endif

ifeq ($(WITH_CONVERTOR),true)
LOCAL_CFLAGS += -DBUILD_WITH_CONVERTOR
endif

LOCAL_C_INCLUDES += \
    $(LOCAL_PATH)/../libffmpeg \
    $(LOCAL_PATH)/../libmediaplayer \
    $(LOCAL_PATH)/../include

LOCAL_SRC_FILES := \
    onLoad.cpp \
    com_media_ffmpeg_FFMpegAVFrame.cpp \
    com_media_ffmpeg_FFMpegAVInputFormat.c \
    com_media_ffmpeg_FFMpegAVRational.c \
    com_media_ffmpeg_FFMpegAVFormatContext.c \
    com_media_ffmpeg_FFMpegAVCodecContext.cpp \
    com_media_ffmpeg_FFMpegUtils.cpp

ifeq ($(WITH_CONVERTOR),true)
LOCAL_SRC_FILES += \
    com_media_ffmpeg_FFMpeg.c \
    ../libffmpeg/cmdutils.c
endif
		
ifeq ($(WITH_PLAYER),true)
LOCAL_SRC_FILES += \
    com_media_ffmpeg_FFMpegPlayer.cpp
#com_media_ffmpeg_android_FFMpegPlayerAndroid.cpp
endif

ifeq ($(IN_NDK),true)	
LOCAL_LDLIBS := -llog
else
LOCAL_PRELINK_MODULE := false
LOCAL_SHARED_LIBRARIES := liblog
endif

LOCAL_SHARED_LIBRARIES := libjniaudio14 libjnivideo14 liblog libffmpeg
LOCAL_STATIC_LIBRARIES := libmediaplayer

LOCAL_MODULE := libffmpeg_jni14
include $(BUILD_SHARED_LIBRARY)