#ifndef __A_LOG_H__
#define __A_LOG_H__

#include <android/log.h>
#include <assert.h>	// assert

#define HUNCENT_TAG "hun-cent-jb-jni"

#define JB_LOGV(...) __android_log_print(ANDROID_LOG_VERBOSE, HUNCENT_TAG, __VA_ARGS__)
#define JB_LOGD(...) __android_log_print(ANDROID_LOG_DEBUG  , HUNCENT_TAG, __VA_ARGS__)
#define JB_LOGI(...) __android_log_print(ANDROID_LOG_INFO   , HUNCENT_TAG, __VA_ARGS__)
#define JB_LOGW(...) __android_log_print(ANDROID_LOG_WARN   , HUNCENT_TAG, __VA_ARGS__)
#define JB_LOGE(...) __android_log_print(ANDROID_LOG_ERROR  , HUNCENT_TAG, __VA_ARGS__)


#define IF_DO(con, doit ) do { if(con) { doit; } }while(false)
#define IF_DO_THEN_DO(con, doit, thendoit ) do { if(con) { doit; } else { thendoit; } }while(false)

#define J_ASSERT(con) assert(con)

/**
 * clamp the v to [min, max]
 * */
inline int clamp(int v, int min, int max){
	IF_DO(v > max, return max);
	IF_DO(v < min, return min);
	return v;
}

/**
 * get array size
 * */
#ifndef __countof
#define __countof(array) (sizeof(array)/sizeof(array[0]))
#endif

#endif

