LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE := libjnivideo

LOCAL_SRC_FILES := libjnivideo.so

include $(PREBUILT_SHARED_LIBRARY)