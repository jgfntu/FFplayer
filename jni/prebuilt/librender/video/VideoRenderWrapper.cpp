#include <surfaceflinger/Surface.h>
#define TAG "VideoRenderWrapper"
#include <utils/Log.h>
#include <SkBitmap.h>
#include <SkCanvas.h>
#include <jni.h>

#include "VideoRenderWrapper.h"

using namespace android;
#define CHECK_NATIVE_SURFACE(surface) \
    do { \
        if(NULL == surface) \
        return ANDROID_SURFACE_RESULT_JNI_EXCEPTION; \
    } while (0)

/**
 * Internal helper functions declaration
 */
static void InitBitmap(SkBitmap *bitmap, int format, int width, int height);
static Surface* GetNativeSurface(JNIEnv* env, jobject jsurface);
static void UpdateSurface(bool autoscale);
/**
 * Internal data structure definitions
 */
static Surface*         surface = NULL;
static Surface::SurfaceInfo surfaceInfo;

static SkBitmap         bmpClient;
static SkBitmap         bmpSurface;
static SkRect           rect_bmpClient;
static SkRect           rect_bmpSurface;
static SkMatrix         matrix;

static
void InitBitmap(SkBitmap *bitmap, int format, int width, int height) {
    switch (format) {
        case PIXEL_FORMAT_RGBA_8888:
            bitmap->setConfig(SkBitmap::kARGB_8888_Config,
                              width, height);
            break;

        case PIXEL_FORMAT_RGBA_4444:
            bitmap->setConfig(SkBitmap::kARGB_4444_Config,
                              width, height);
            break;

        case PIXEL_FORMAT_RGB_565:
            bitmap->setConfig(SkBitmap::kRGB_565_Config,
                              width, height);
            break;

        case PIXEL_FORMAT_A_8:
            bitmap->setConfig(SkBitmap::kA8_Config,
                              width, height);
            break;

        default:
            bitmap->setConfig(SkBitmap::kNo_Config,
                              width, height);
            break;
    }
}

static
Surface* GetNativeSurface(JNIEnv* env, jobject jsurface) {
    jclass clazz = env->FindClass("android/view/Surface");
    if (clazz == NULL) {
        __android_log_print(ANDROID_LOG_INFO, TAG, "find class android.view.Surface failed!");
    }
    const char * surface_field_names[] = {
        "mSurface", // for android 2.2.3 below version
        "mNativeSurface", // for android other versions
    };
    jfieldID field_surface = NULL;
    for (unsigned int i = 0; i < sizeof(surface_field_names)/sizeof(surface_field_names[0]); i++) {
        field_surface = env->GetFieldID(clazz, surface_field_names[i], "I");
        if(field_surface != NULL) {
            break;
        }
    }
    if (field_surface == NULL) {
        __android_log_print(ANDROID_LOG_INFO, TAG, "find surface field of class android.view.Surface failed!");
        return NULL;
    }
    return (Surface *) env->GetIntField(jsurface, field_surface);
}

int SurfaceWrapper_Register(JNIEnv* env, jobject jsurface) {
    __android_log_print(ANDROID_LOG_INFO, TAG, "Registering video surface");
    if (NULL == surface) {
        surface = GetNativeSurface(env, jsurface);
        CHECK_NATIVE_SURFACE(surface);
        __android_log_print(ANDROID_LOG_INFO, TAG, "Surface registered");
        return ANDROID_SURFACE_RESULT_SUCCESS;
    } else {
        return ANDROID_SURFACE_RESULT_JNI_EXCEPTION;
    }
}

int SurfaceWrapper_GetPixels(int width, int height, void** pixels) {
    __android_log_print(ANDROID_LOG_INFO, TAG, "getting surface's pixels %ix%i", width, height);
    CHECK_NATIVE_SURFACE(surface);

    InitBitmap(&bmpClient, PIXEL_FORMAT_RGB_565, width, height);
    bmpClient.setIsOpaque(true);
    //-- alloc array of pixels
    if(!bmpClient.allocPixels()) {
        return ANDROID_SURFACE_RESULT_COULDNT_INIT_BITMAP_CLIENT;
    }
    *pixels = bmpClient.getPixels();
    __android_log_print(ANDROID_LOG_INFO, TAG, "surface's pixels getted");
    return ANDROID_SURFACE_RESULT_SUCCESS;
}

static
void UpdateSurface(bool autoscale) {
    SkCanvas canvas(bmpSurface);

    if(autoscale)
    {
        rect_bmpSurface.set(0, 0, bmpSurface.width(), bmpSurface.height());
        rect_bmpClient.set(0, 0, bmpClient.width(), bmpClient.height());
        matrix.setRectToRect(rect_bmpClient, rect_bmpSurface, SkMatrix::kFill_ScaleToFit);
        canvas.drawBitmapMatrix(bmpClient, matrix);
    }
    else
    {
        canvas.drawBitmap(bmpClient, 0, 0);
    }
}

int SurfaceWrapper_Update(bool autoscale) {
    CHECK_NATIVE_SURFACE(surface);
    if (!surface->isValid()) {
        return ANDROID_SURFACE_RESULT_NOT_VALID;
    }
    if (surface->lock(&surfaceInfo) < 0) {
        return ANDROID_SURFACE_RESULT_COULDNT_LOCK;
    }

    /* create surface bitmap with pixels of surface */
    if(bmpSurface.width() != (int)surfaceInfo.w ||
       bmpSurface.height() != (int)surfaceInfo.h)
    {
        InitBitmap(&bmpSurface, surfaceInfo.format,
                   surfaceInfo.w, surfaceInfo.h);
    }
    bmpSurface.setPixels(surfaceInfo.bits);

    /* redraw surface screen */
    UpdateSurface(autoscale);

    if (surface->unlockAndPost() < 0) {
        return ANDROID_SURFACE_RESULT_COULDNT_UNLOCK_AND_POST;
    }
    return ANDROID_SURFACE_RESULT_SUCCESS;
}

int SurfaceWrapper_Unregister() {
    __android_log_print(ANDROID_LOG_INFO, TAG, "unregistering video surface");
    surface = NULL;
    return ANDROID_SURFACE_RESULT_SUCCESS;
}
