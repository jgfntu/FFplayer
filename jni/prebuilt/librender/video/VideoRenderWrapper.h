#ifndef FFPLAYER_PREBUILT_VIDEO_RENDER_WRAPPER_H
#define FFPLAYER_PREBUILT_VIDEO_RENDER_WRAPPER_H
// operation results
#define ANDROID_SURFACE_RESULT_SUCCESS                          0
#define ANDROID_SURFACE_RESULT_NOT_VALID                        -1
#define ANDROID_SURFACE_RESULT_COULDNT_LOCK                     -2
#define ANDROID_SURFACE_RESULT_COULDNT_UNLOCK_AND_POST          -3
#define ANDROID_SURFACE_RESULT_COULDNT_INIT_BITMAP_SURFACE      -4
#define ANDROID_SURFACE_RESULT_COULDNT_INIT_BITMAP_CLIENT       -5
#define ANDROID_SURFACE_RESULT_JNI_EXCEPTION                    -6

#ifdef __cplusplus
extern "C" {
#endif
/**
 * Register the surface passed in from Java layer to our
 * internal structure so as to make the render image content possible
 */
int SurfaceWrapper_Register(JNIEnv* env, jobject jsurface);

/**
 * Get the raw pixel buffer pointer by the given width & height parameters,
 * implicit "surface" which created priorly when calling Surface_Register
 */
int SurfaceWrapper_GetPixels(int width, int height, void** pixels);

/**
 * Update the content of the previously registerred surface
 */
int SurfaceWrapper_Update(bool autoscale);

/**
 * Unregister the surface previously registerred
 */
int SurfaceWrapper_Unregister();
#ifdef __cplusplus
}
#endif
#endif
