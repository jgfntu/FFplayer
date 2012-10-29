#ifndef _JNI_UTILS_H_
#define _JNI_UTILS_H_

#include <stdlib.h>
#include <jni.h>

#ifndef ANDROID_SIMULATOR
#define ANDROID_SIMULATOR
#endif

#ifdef __cplusplus
extern "C"
{
#endif

int jniThrowException(JNIEnv* env, const char* className, const char* msg);

JNIEnv* getJNIEnv();
void attachCurrentThread();
void detachCurrentThread();
int jniRegisterNativeMethods(JNIEnv* env,
                             const char* className,
                             const JNINativeMethod* gMethods,
                             int numMethods);

#ifdef __cplusplus
}
#endif

#endif /* _JNI_UTILS_H_ */
