LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE := libasset 

LOCAL_C_INCLUDES := $(LOCAL_PATH)/../libzip/ $(LOCAL_PATH)/../libpng/
LOCAL_STATIC_LIBRARIES := libzip libpng

LOCAL_LDLIBS := -landroid -llog -lz -lm  
LOCAL_SRC_FILES := asset.cpp


#include $(BUILD_SHARED_LIBRARY)
include $(BUILD_STATIC_LIBRARY)