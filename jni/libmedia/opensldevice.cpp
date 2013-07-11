#include "opensldevice.h"
#include "openslformat.h"
#include "openslout.h"
#include "openslin.h"


/**
 * The OpenSL Device Object
 * */
OpenSLDevice::OpenSLDevice():
mEngineObj(NULL), mEngineInterface(NULL), mOutputMixObj(NULL) {
	//
}

/**
 * Start the deice
 * */
bool OpenSLDevice::start(){
	assert(mEngineObj == NULL);
	assert(mEngineInterface == NULL);
	assert(mOutputMixObj == NULL);

	SLresult lRes;
	// config the parameters of device
    const SLuint32      lEngineMixIIDCount = 1;
    const SLInterfaceID lEngineMixIIDs[]={SL_IID_ENGINE};
    const SLboolean lEngineMixReqs[]={SL_BOOLEAN_TRUE};
    const SLuint32 lOutputMixIIDCount=0;
    const SLInterfaceID lOutputMixIIDs[]={};
    const SLboolean lOutputMixReqs[]={};
    SLEngineOption EngineOption[] = { {(SLuint32) SL_ENGINEOPTION_THREADSAFE, (SLuint32) SL_BOOLEAN_TRUE} };

    // crate the engine
    lRes = slCreateEngine(&mEngineObj, 1, EngineOption, lEngineMixIIDCount, lEngineMixIIDs, lEngineMixReqs);
    if (lRes != SL_RESULT_SUCCESS) goto ERROR;

    // do allocate resource for engine
    lRes=(*mEngineObj)->Realize(mEngineObj,SL_BOOLEAN_FALSE);
    if (lRes != SL_RESULT_SUCCESS) goto ERROR;

    // get the interface of engine device
    lRes=(*mEngineObj)->GetInterface(mEngineObj, SL_IID_ENGINE, &mEngineInterface);
    if (lRes != SL_RESULT_SUCCESS) goto ERROR;

    return true;

ERROR:
	stop();
	return false;
}

/**
 * crate the sl device output mix object
 * */
bool OpenSLDevice::createOutputMixObj(){
	assert(mEngineObj != NULL);
	assert(mEngineInterface != NULL);
	assert(mOutputMixObj == NULL);

	// config
	const SLuint32 lOutputMixIIDCount=0;
    const SLInterfaceID lOutputMixIIDs[]={};
    const SLboolean lOutputMixReqs[]={};

    // create mix
	SLresult lRes;
	lRes=(*mEngineInterface)->CreateOutputMix(mEngineInterface, &mOutputMixObj,lOutputMixIIDCount,lOutputMixIIDs, lOutputMixReqs);
    if (lRes != SL_RESULT_SUCCESS) goto ERROR;
	lRes=(*mOutputMixObj)->Realize(mOutputMixObj, SL_BOOLEAN_FALSE);
    if (lRes != SL_RESULT_SUCCESS) goto ERROR;

	return true;

ERROR:
	safe_destroy_slobj(mOutputMixObj);
	return false;
}

/**
 * retutn the unique output mix object
 */
SLObjectItf OpenSLDevice::outputMixObj() const{
	return mOutputMixObj;
}

/**
 * return the sl eingine interface
 * */
SLEngineItf OpenSLDevice::engine() const{
	return mEngineInterface;
}

/**
 * engine object
 * */
SLObjectItf OpenSLDevice::engineObject() const{
	return mEngineObj;
}

/**
 * Stop the device
 * */
void OpenSLDevice::stop() {
	// safe destroy the opensl device
	safe_destroy_slobj(mOutputMixObj);
	safe_destroy_slobj(mEngineObj);

	// clear the engine interface anyway
	mEngineInterface = NULL;
}

//--------------------------------------------------------------------------------------------
// android pensles format check : http://mobilepearls.com/labs/native-android-api/opensles/
// OpenSL ES for Android is designed for multi-threaded applications, and is thread-safe.
// OpenSL ES for Android supports a single engine per application, and up to 32 objects.
//
// 1. PCM Format Data can be used with buffer queues only
//	PCM playback configurations are 8-bit unsigned or 16-bit signed,
//	mono or stereo, little endian byte ordering,
//	and these sample rates: 8000, 11025, 12000, 16000, 22050, 24000, 32000, 44100, or 48000 Hz
//	For recording, the supported configurations are device-dependent,
//	however generally 16000 Hz mono 16-bit signed is usually available.
//
// 2. MIME data format: The MIME data format can be used with URI data locator only,
//	and only for player (not recorder). we recommend that you set the mimeType to NULL
//	and containerType to SL_CONTAINERTYPE_UNSPECIFIED.
//	(except the producere decode adts aac to pcm)
//
// 3. Record : SL_RECORDEVENT_HEADATLIMIT and SL_RECORDEVENT_HEADMOVING events are not supported.
//
// 4. Seek : SetLoop enables whole file looping.
//	The startPos parameter should be zero and the endPos parameter should be SL_TIME_UNKNOWN.
//
// 5. URI data locator :  can be used with MIME data format only,
//	and only for an audio player (not audio recorder).
//	Supported schemes are http: and file:.
//	A missing scheme defaults to the file: scheme.
//	Other schemes such as https:, ftp:, and content: are not supported. rtsp: is not verified.
//
// 6. Buffer queue data locator: An audio player or recorder with buffer queue data
//	locator supports PCM data format only.
//
// 7. Device data locator: The only supported use of an I/O device data locator is
//	when it is specified as the data source for Engine::CreateAudioRecorder.
//
// 8. Android file descriptor data locator: The data format must be MIME.
//
// 9. Android simple buffer queue data locator and interface: are limited to PCM data format.
//
// 10. Decode audio to PCM:  this feature is available at API level 14 and higher.
//
// 11. Decode streaming ADTS AAC to PCM : this feature is available at API level 14 and higher.
// An audio player acts as a streaming decoder
//	if the data source is an Android buffer queue data locator with MIME data format,
//  and the data sink is an Android simple buffer queue data locator with PCM data format.
// The MIME data format should be configured as:
// container: SL_CONTAINERTYPE_RAW
// MIME type string: "audio/vnd.android.aac-adts" (macro SL_ANDROID_MIME_AACADTS)
//
// 12. Decode audio to PCM: this feature is available at API level 14 and higher.
//  as an Android extension, an audio player instead acts as a decoder
//	if the data source is specified as a URI or Android file descriptor data locator with MIME data format,
//	and the data sink is an Android simple buffer queue data locator with PCM data format.
//--------------------------------------------------------------------------------------------

/**
 * Check the format is right
 * */
int OpenSLDevice::checkOutFormat(OpenSLOut* out, const OpenSLOutFormat& format) const{
	OpenSLOutFormat* outFormat = (OpenSLOutFormat*)(&format);
	if(outFormat->dstLocatorType == SL_DATALOCATOR_OUTPUTMIX){
		outFormat->dstDataLocator.dataLocator_OutputMix.outputMix = mOutputMixObj;
	}
	return SLERROR_OK;
}

/**
 * Check the format is right
 * */
int OpenSLDevice::checkInFormat(OpenSLIn* in, const OpenSLInFormat& format) const{
	return SLERROR_OK;
}

static OpenSLDevice* gslDevice;
/**
 * */
void slDevice_start(){
	J_ASSERT(gslDevice == NULL);
	gslDevice = new OpenSLDevice();
	if(!gslDevice->start()){
		JB_LOGE("Failed to start the slDevice");
		goto ERROR;
	}

	if(!gslDevice->createOutputMixObj()){
		JB_LOGE("Failed to createOutputMixObj");
		goto ERROR;
	}

	return ;

ERROR:
	slDevice_stop();
}

/**
 * */
void slDevice_stop(){
	if(gslDevice){
		gslDevice->stop();
		delete gslDevice;
		gslDevice = NULL;
	}
}

/**
 * */
OpenSLDevice* slDevice(){
	return gslDevice;
}
