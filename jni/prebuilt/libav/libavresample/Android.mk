LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)
LOCAL_MODULE := libavresample
LOCAL_SRC_FILES := libavresample.a
include $(PREBUILT_STATIC_LIBRARY)

