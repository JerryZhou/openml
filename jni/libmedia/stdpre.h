#ifndef _STDPRE_H_
#define _STDPRE_H_

#include <jni.h>	// jni
#include <cstddef>	// NULL
#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/time.h>
#include <fcntl.h>
#include <pthread.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

#include <string>
#include <list>

// opensles
#include <SLES/OpenSLES.h>
#include <SLES/OpenSLES_Android.h>
#include <SLES/OpenSLES_AndroidConfiguration.h>

// log
#include "../include/log.h"
// mutex
#include "../include/mutex.h"

#if DEBUG
inline void safe_destroy_slobj(SLObjectItf &obj){
	if(obj != NULL){
		(*obj)->Destroy(obj);
		obj = NULL;
	}
}
#else
#define safe_destroy_slobj(obj) if(obj != NULL){ (*obj)->Destroy(obj); obj = NULL; }
#endif
#if DEBUG
inline void slCheckErrorWithStatus(SLresult state, const char* description, SLresult& out){
	if(state != SL_RESULT_SUCCESS) {
		JB_LOGE(description);
		out = SL_RESULT_SUCCESS;
	}
}
#else
#define slCheckErrorWithStatus(state, description, out) \
	if(state != SL_RESULT_SUCCESS) { JB_LOGE(description); out = SL_RESULT_SUCCESS; }
#endif
#endif
