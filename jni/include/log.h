#ifndef __A_LOG_H__
#define __A_LOG_H__

#include <android/log.h>
#include <assert.h>	// assert

#define TAG "hun-cent-jb-jni"

#define LOGV(...) __android_log_print(ANDROID_LOG_VERBOSE, TAG, __VA_ARGS__)
#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG  , TAG, __VA_ARGS__)
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO   , TAG, __VA_ARGS__)
#define LOGW(...) __android_log_print(ANDROID_LOG_WARN   , TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR  , TAG, __VA_ARGS__)


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

