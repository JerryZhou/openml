
# build env lib
LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE := libmedia 

LOCAL_C_INCLUDES := $(LOCAL_PATH)/../libzip/ \
	$(LOCAL_PATH)/../libpng/ \
	$(LOCAL_PATH)/../libasset/
LOCAL_STATIC_LIBRARIES := libzip libpng libasset

#-Wno-psabi to remove warning about GCC 4.4 va_list warning
LOCAL_CFLAGS := -DANDROID_NDK -Wno-psabi

# link the opengl, opensl
LOCAL_LDLIBS := -landroid -llog -lz -lm -lOpenSLES

LOCAL_SRC_FILES := opensldevice.cpp openslobject.cpp \
	openslout.cpp opensloutasset.cpp opensloutstream.cpp \
	openslin.cpp \
	openslmedia_test.cpp \
	slesTestDecodeAac.cpp \
	codec.cpp

#include $(BUILD_SHARED_LIBRARY)
include $(BUILD_STATIC_LIBRARY)


