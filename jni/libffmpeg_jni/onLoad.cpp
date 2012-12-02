#define TAG "ffmpeg_onLoad"

#include <stdlib.h>
#include <android/log.h>
#include "jniUtils.h"

extern int register_android_media_FFMpegPlayerAndroid(JNIEnv *env);
extern int register_android_media_FFMpeg(JNIEnv *env);
static JavaVM *sVm;

/*
 * Throw an exception with the specified class and an optional message.
 */
int jniThrowException(JNIEnv* env, const char* className, const char* msg) {
    jclass exceptionClass = env->FindClass(className);
    if (exceptionClass == NULL) {
        __android_log_print(ANDROID_LOG_ERROR,
			    TAG,
			    "Unable to find exception class %s",
	                    className);
        return -1;
    }

    if (env->ThrowNew(exceptionClass, msg) != JNI_OK) {
        __android_log_print(ANDROID_LOG_ERROR,
			    TAG,
			    "Failed throwing '%s' '%s'",
			    className, msg);
    }
    return 0;
}

void attachCurrentThread()
{

#ifdef ANDROID_SIMULATOR
	    // The simulator runs everything as one process, so any
	    // Binder calls happen on this thread instead of a thread
	    // in another process. We therefore need to make sure that
	    // this thread can do calls into interpreted code.
	    // On the device this is not an issue because the remote
	    // thread will already be set up correctly for this.
	    JavaVM *vm = sVm;
	    if (!vm) {
	    	return;
	    }
	    JNIEnv *env;
	    vm->AttachCurrentThread(&env, NULL);
#endif

}

void detachCurrentThread()
{
#ifdef ANDROID_SIMULATOR
	    // The simulator runs everything as one process, so any
	    // Binder calls happen on this thread instead of a thread
	    // in another process. We therefore need to make sure that
	    // this thread can do calls into interpreted code.
	    // On the device this is not an issue because the remote
	    // thread will already be set up correctly for this.
	    JavaVM *vm = sVm;
	    if (!vm) {
	    	return;
	    }
	    vm->DetachCurrentThread();
#endif
}

JNIEnv* getJNIEnv() {
	/* The calling Thread should AttachCurrentThread */
    __android_log_print(ANDROID_LOG_INFO, TAG, "getJNIEnv() in. . .");
    JNIEnv* env = NULL;
    if (sVm->GetEnv((void**) &env, JNI_VERSION_1_4) != JNI_OK) {
    	__android_log_print(ANDROID_LOG_ERROR,
							TAG,
							"Failed to obtain JNIEnv");
    	return NULL;
    }
    return env;
}

/*
 * Register native JNI-callable methods.
 *
 * "className" looks like "java/lang/String".
 */
int jniRegisterNativeMethods(JNIEnv* env,
                             const char* className,
                             const JNINativeMethod* gMethods,
                             int numMethods)
{
    jclass clazz;

    __android_log_print(ANDROID_LOG_INFO, TAG, "Registering %s natives\n", className);
    clazz = env->FindClass(className);
    if (clazz == NULL) {
        __android_log_print(ANDROID_LOG_ERROR, TAG, "Native registration unable to find class '%s'\n", className);
        return -1;
    }
    if (env->RegisterNatives(clazz, gMethods, numMethods) < 0) {
        __android_log_print(ANDROID_LOG_ERROR, TAG, "RegisterNatives failed for '%s'\n", className);
        return -1;
    }
    return 0;
}

jint JNI_OnLoad(JavaVM* vm, void* reserved) {
    JNIEnv* env = NULL;
    jint result = JNI_ERR;
	sVm = vm;

    if (vm->GetEnv((void**) &env, JNI_VERSION_1_4) != JNI_OK) {
        __android_log_print(ANDROID_LOG_ERROR, TAG, "GetEnv failed!");
        return result;
    }

    __android_log_print(ANDROID_LOG_INFO, TAG, "JNI_OnLoad loading . . .");

    if(register_android_media_FFMpegPlayerAndroid(env) != JNI_OK) {
    	__android_log_print(ANDROID_LOG_ERROR, TAG, "can't load android_media_FFMpegPlayerAndroid");
    	goto end;
    }

    if(register_android_media_FFMpeg(env) != JNI_OK) {
    	__android_log_print(ANDROID_LOG_ERROR, TAG, "can't load android_media_FFMpegPlayerAndroid");
    	goto end;
    }

    __android_log_print(ANDROID_LOG_INFO, TAG, "loaded");

    result = JNI_VERSION_1_4;

end:
    return result;
}
