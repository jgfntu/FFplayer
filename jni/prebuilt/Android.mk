LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)
LOCAL_MODULE := libjnivideo
LOCAL_SRC_FILES := libjnivideo.so
include $(PREBUILT_SHARED_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE := libjniaudio
LOCAL_SRC_FILES := libjniaudio.so
include $(PREBUILT_SHARED_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE := libjnivideo9
LOCAL_SRC_FILES := libjnivideo9.so
include $(PREBUILT_SHARED_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE := libjniaudio9
LOCAL_SRC_FILES := libjniaudio9.so
include $(PREBUILT_SHARED_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE := libjnivideo14
LOCAL_SRC_FILES := libjnivideo14.so
include $(PREBUILT_SHARED_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE := libjniaudio14
LOCAL_SRC_FILES := libjniaudio14.so
include $(PREBUILT_SHARED_LIBRARY)