#include <jni.h>
#include "openslin.h"
#include "opensldevice.h"

/**
 * helper class to visitor OpenSLIn
 * */
class OpenSLInTryPushHelper{
public:
	OpenSLInTryPushHelper(OpenSLIn* in): sl_in_device(in){

	}

	void onCallBack(int when, void* context)const{
		sl_in_device->tryCallBack(when, context);
	}

	void push(OpenSLBuffer* buffer)const{
		sl_in_device->tryPush(buffer);
	}

	OpenSLBuffer* bufferPush() const{
		return sl_in_device->bufferPush();
	}

	int doGoOnRecord(){
		return sl_in_device->goOnRecord();
	}

	OpenSLIn * sl_in_device;
};

/**
 * opensl record call back
 * */
static void sl_callback_recorder(SLAndroidSimpleBufferQueueItf pQueue, void* pContext){
	OpenSLIn* in = (OpenSLIn*)(pContext);
	OpenSLInTryPushHelper pushHelper(in);

	// push record buffer
	OpenSLBuffer* buffer = pushHelper.bufferPush();
	buffer->dataLen = buffer->len;

	// do push
	pushHelper.push(buffer);
	pushHelper.onCallBack(WHEN_QUEUE_PUSH, buffer);

	// go on record
	int lRet = pushHelper.doGoOnRecord();
	if(SL_SUCCESS(lRet)){
		pushHelper.onCallBack(WHEN_QUEUE_GOONRECORD_SUCCESS, pContext);
	}else{
		pushHelper.onCallBack(WHEN_QUEUE_GOONRECORD_FAILED, pContext);
	}
}

/**
 * default constructor
 * */
OpenSLIn::OpenSLIn(OpenSLDevice *device)
: OpenSLObject(device) {
	mName = "OpenSLIn";
	mState = SLIN_STOP;

	mRecorderObj = NULL;
	mRecorder = NULL;
	mRecorderQueue = NULL;
	memset(&mFormat, 0, sizeof(mFormat));
}

/**
 *
 * */
OpenSLIn::~OpenSLIn(){
	mark(IN_DESTRUTCTOR);
	// NB! there are may be pure virtual function call
	// as in stop() function , we will call onCallBack()
	// so you should call stop before object delete
	IF_DO(mState != SLIN_STOP,
			JB_LOGE("openslout should call stop before delete,"
					"leak some event"));
	IF_DO(mState != SLIN_STOP, stopInternal() );
}

/**
 * */
bool OpenSLIn::start(const OpenSLInFormat &format){
	J_ASSERT(mState == SLIN_STOP);

	// check format is valid
	IF_DO(SL_FAILED(mDevice->checkInFormat(this, format)), return false);
	mFormat = format;
	setFormat(format);

	tryCallBack(WHEN_START, &mFormat);
	mState = SLIN_RECORD;

	SLDataSink lDataSink;
	lDataSink.pLocator = (void*)(&format.dataLocator);
	lDataSink.pFormat = (void*)(&format.dataFormat);

	SLDataLocator_IODevice lDataLocatorIn;
	lDataLocatorIn.locatorType = SL_DATALOCATOR_IODEVICE;
	lDataLocatorIn.deviceType = SL_IODEVICE_AUDIOINPUT;
	lDataLocatorIn.deviceID = SL_DEFAULTDEVICEID_AUDIOINPUT;
	lDataLocatorIn.device = NULL;

	SLDataSource lDataSource;
	lDataSource.pLocator = &lDataLocatorIn;
	lDataSource.pFormat = NULL;

	SLresult lRes;
	int lRec;
	const SLuint32 lSoundRecorderIIDCount = 2;
	const SLInterfaceID lSoundRecorderIIDs[] = { SL_IID_RECORD, SL_IID_ANDROIDSIMPLEBUFFERQUEUE };

	SLEngineItf engine = mDevice->engine();
	const SLboolean lSoundRecorderReqs[] = { SL_BOOLEAN_TRUE, SL_BOOLEAN_TRUE };
	(*engine)->CreateAudioRecorder(engine, &mRecorderObj, &lDataSource, &lDataSink,
			lSoundRecorderIIDCount, lSoundRecorderIIDs, lSoundRecorderReqs);

	if (lRes != SL_RESULT_SUCCESS) goto ERROR;
	lRes = (*mRecorderObj)->Realize(mRecorderObj, SL_BOOLEAN_FALSE);
	if (lRes != SL_RESULT_SUCCESS) goto ERROR;

	lRes = (*mRecorderObj)->GetInterface(mRecorderObj, SL_IID_RECORD, &mRecorder);
	if (lRes != SL_RESULT_SUCCESS) goto ERROR;
	lRes = (*mRecorderObj)->GetInterface(mRecorderObj, SL_IID_ANDROIDSIMPLEBUFFERQUEUE, &mRecorderQueue);
	if (lRes != SL_RESULT_SUCCESS) goto ERROR;

	// Registers a record callback.
	lRes = (*mRecorderQueue)->RegisterCallback(mRecorderQueue, sl_callback_recorder, this);
	if (lRes != SL_RESULT_SUCCESS) goto ERROR;
	lRes = (*mRecorder)->SetCallbackEventsMask(mRecorder, SL_RECORDEVENT_BUFFER_FULL);
	if (lRes != SL_RESULT_SUCCESS) goto ERROR;

	// start record
	lRec = record();
	if (SL_FAILED(lRec)) goto ERROR;

	tryCallBack(WHEN_START_END, &mFormat);

	return true;

ERROR:
	JB_LOGE("start record failed");
	stop();
	return false;
}

/**
 * pause the play, return true if current are playing
 * */
bool OpenSLIn::pause(){
	J_ASSERT(mState == SLIN_PAUSE || mState == SLIN_RECORD);

	tryCallBack(WHEN_PAUSE, &mState);

	if(mState == SLIN_PAUSE){
		// goto record
		mState = SLIN_RECORD;
	}else if(mState == SLIN_RECORD){
		// goto pause
		mState = SLIN_PAUSE;
		return true;
	}else {
		J_ASSERT(false);
	}

	return false;
}

/**
 * stop the play
 * */
void OpenSLIn::stop(){
	J_ASSERT(mState != SLIN_STOP);

	tryCallBack(WHEN_STOP, 0);

	mState = SLIN_STOP;
	stopInternal();
	return;
}

/**
 * return current out device state
 * */
OpenSLInState OpenSLIn::curState() const{
	return mState;
}

/**
 * rebegin record
 * */
int OpenSLIn::record(){
	IF_DO(mRecorderObj == NULL, return SLERROR_ERR);

	SLresult lRes;
	SLuint32 lRecorderState;
	(*mRecorderObj)->GetState(mRecorderObj, &lRecorderState);
	if (lRecorderState == SL_OBJECT_STATE_REALIZED) {
		// Stops any current recording.
		lRes = (*mRecorder)->SetRecordState(mRecorder,
				SL_RECORDSTATE_STOPPED);
		if (lRes != SL_RESULT_SUCCESS) goto ERROR;
		lRes = (*mRecorderQueue)->Clear(mRecorderQueue);
		if (lRes != SL_RESULT_SUCCESS) goto ERROR;

		// be sure the buffer is recordSize alloc
		OpenSLBuffer *buffer = bufferPush();
		buffer->alloc(mFormat.recordSize);
		// Provides a buffer for recording.
		lRes = (*mRecorderQueue)->Enqueue(mRecorderQueue,
				buffer->data, buffer->len);
		if (lRes != SL_RESULT_SUCCESS) goto ERROR;
		// do call back
		void* params[] = { (void*)mRecorderQueue, buffer, &buffer->len };
		tryCallBack(WHEN_ENQUEUE_BUFFER, params);

		// Starts recording.
		lRes = (*mRecorder)->SetRecordState(mRecorder,
				SL_RECORDSTATE_RECORDING);
		if (lRes != SL_RESULT_SUCCESS) goto ERROR;
	}
	return SLERROR_OK;

ERROR:
	JB_LOGE("error happend in start record!!");
	return SLERROR_ERR;
}

/**
 * */
int OpenSLIn::goOnRecord(){
	IF_DO(mRecorderObj == NULL, return SLERROR_ERR);

	SLresult lRes;
	SLuint32 lRecorderState;
	(*mRecorderObj)->GetState(mRecorderObj, &lRecorderState);
	if (lRecorderState == SL_OBJECT_STATE_REALIZED) {
		// be sure the buffer is recordSize alloc
		OpenSLBuffer *buffer = bufferPush();
		buffer->alloc(mFormat.recordSize);
		// Provides a buffer for recording.
		lRes = (*mRecorderQueue)->Enqueue(mRecorderQueue,
				buffer->data, buffer->len);
		if (lRes != SL_RESULT_SUCCESS) goto ERROR;
		// do call back
		void* params[] = { (void*)mRecorderQueue, buffer, &buffer->len };
		tryCallBack(WHEN_ENQUEUE_BUFFER, params);
	}
	return SLERROR_OK;

ERROR:
	JB_LOGE("error happend in goon record!!");
	return SLERROR_ERR;
}

/**
 * release the resources
 * */
void OpenSLIn::stopInternal(){
	// base stop
	OpenSLObject::stopInternal();
	// free userdata first
	IF_DO(mUserData != NULL, freeUserData(mUserData));
	// free opensl object
	safe_destroy_slobj(mRecorderObj);
	// clear pointer
	mRecorderQueue = NULL;
	mRecorder = NULL;
}

//--------------------------------------------------------------------------------
// Test Suit
//--------------------------------------------------------------------------------
static OpenSLIn* records[5] = {0};
/**
 * */
bool startRecord(const OpenSLInFormat &format, int idx){
	IF_DO(idx < 0 || idx >= __countof(records), return false);
	OpenSLIn * &in = records[idx];

	if(in == NULL){
		in = new OpenSLIn(slDevice());
	}

	if(in->curState() != SLIN_STOP){
		in->stop();
	}

	return in->start(format);
}

/**
 * */
void stopRecord(int idx){
	IF_DO(idx < 0 || idx >= __countof(records), return);
	OpenSLIn * &in = records[idx];
	IF_DO(in == NULL, return);

	if(in->curState() != SLIN_STOP){
		in->stop();
	}
}
