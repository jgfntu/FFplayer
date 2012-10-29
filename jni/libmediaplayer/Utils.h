#ifndef __UTILS_H
#define __UTILS_H
#include <android/log.h>
#define min(a, b) (((a) > (b)) ? (b) : (a))
//helper macro to issue a function is invoked
#define ISSUE_FUNCTION_IN() \
    do { \
        LOGD("Inside function %s()\n", __FUNCTION__); \
    } while(0)

//helper macro to test a pointer in a function returning void
#define CHECK_POINTER_VOID(pointer) \
    do { \
        if (NULL == pointer) {\
            LOGD("%s is NULL on line:%d of file: \"%s\"\n", #pointer, __LINE__, __FILE__); \
            return; \
        } \
    } while (0)

//helper macro to test a pointer in a function returning int
#define CHECK_POINTER_INT(pointer, ret) \
    do { \
        if (NULL == pointer) {\
            LOGD("%s is NULL on line:%d of file: \"%s\"\n", #pointer, __LINE__, __FILE__); \
            return ret; \
        } \
    } while (0)

//helper macro to test a pointer in a function returning bool
#define CHECK_POINTER_BOOL(pointer) \
    do { \
        if (NULL == pointer) {\
            LOGD("%s is NULL on line:%d of file: \"%s\"\n", #pointer, __LINE__, __FILE__); \
            return false; \
        } \
    } while (0)
#endif// __UTILS_H
