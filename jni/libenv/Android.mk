LOCAL_PATH := $(call my-dir)

# ---------------------------------------------------------------------------
# lib env
include $(CLEAR_VARS)

LOCAL_MODULE := libenv 

LOCAL_C_INCLUDES := $(LOCAL_PATH)/../libzip/ $(LOCAL_PATH)/../libasset/ $(LOCAL_PATH)/../libmedia/
# becareful about the sequence
LOCAL_STATIC_LIBRARIES := libmedia libasset libzip

#-Wno-psabi to remove warning about GCC 4.4 va_list warning
LOCAL_CFLAGS := -DANDROID_NDK -Wno-psabi

LOCAL_DEFAULT_CPP_EXTENSION := cpp 

LOCAL_SRC_FILES := com_openml_jni_JEnv.cpp 

LOCAL_LDLIBS := -landroid -llog -lz -lm -lEGL -lGLESv1_CM -lOpenSLES

include $(BUILD_SHARED_LIBRARY)
#include $(BUILD_STATIC_LIBRARY)


