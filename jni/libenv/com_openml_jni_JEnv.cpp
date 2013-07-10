#include "com_openml_jni_JEnv.h"
#include <asset.h>
#include "../include/log.h"
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <opensldevice.h>
#include <opensloutasset.h>
#include <openslmedia_test.h>

/*
 * Class:     com_openml_jni_JEnv
 * Method:    setupJni
 * Signature: (Ljava/lang/String;Ljava/lang/String;)Z
 */
JNIEXPORT jboolean JNICALL Java_com_openml_jni_JEnv_setupJni
  (JNIEnv *env, jclass obj_class, jstring apkDir, jstring params, jobject jassetMgr){
	// get the utf-8 buffer
	const char *buffer = env->GetStringUTFChars(apkDir, 0);

	// setup the asset manager
	LOGI("setup pkg asset: %s", buffer);
	asset_startup(buffer, AAssetManager_fromJava(env, jassetMgr));
	// setup the opensl-es device
	LOGI("setup opensl-es device");
	slDevice_start();
	// test play pcm
	LOGI("test media");
	test_media_main();

	// release the buffer
	env->ReleaseStringUTFChars(apkDir, buffer );
}
