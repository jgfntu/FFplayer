LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)
# our source files
#
LOCAL_SRC_FILES:= \
	VideoRenderWrapper.cpp

LOCAL_SHARED_LIBRARIES := \
	libskia \
    	libsurfaceflinger_client \
    	libutils \
	liblog \
	libgui

LOCAL_C_INCLUDES += \
	$(JNI_H_INCLUDE) \
	external/skia/src/core \
	external/skia/include/core \
	frameworks/base/include \
	frameworks/base/native/include \
	system/core/include \
	hardware/libhardware/include \
	dalvik/libnativehelper/include/nativehelper

# Optional tag would mean it doesn't get installed by default
LOCAL_MODULE_TAGS := optional

LOCAL_PRELINK_MODULE := false
LOCAL_ARM_MODE := arm

LOCAL_MODULE:= libjnivideo

include $(BUILD_SHARED_LIBRARY)
