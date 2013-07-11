#include "openslout.h"
#include "opensldevice.h"

/**
 * helper class to hide imp of OpenSLOut
 * */
class OpenSLOutTryPollHelper{
public:
	OpenSLOutTryPollHelper(OpenSLOut* out): sl_out_device(out){

	}

	int poll(OpenSLBuffer* outBuffer){
		// empty buff
		outBuffer->dataLen = 0;
		// queue poll
		return sl_out_device->tryQueuePoll(outBuffer);
	}

	int push(OpenSLBuffer* inBuffer){
		return sl_out_device->tryQueuePush(inBuffer);
	}

	void onCallBack(int when, void* context){
		sl_out_device->tryCallBack(when, context);
	}

	OpenSLBuffer* bufferPoll() const{
		return sl_out_device->bufferPoll();
	}

	OpenSLBuffer* bufferPush() const{
		return sl_out_device->bufferPush();
	}

	int doGoOnPush(){
		return sl_out_device->goOnPush();
	}

	OpenSLOut* sl_out_device;
};

/**
 * call back
 * */
void sl_callback_queue_context(SLBufferQueueItf pBufferQueue, void* context){
	OpenSLOut *out = (OpenSLOut*)(context);
	OpenSLOutTryPollHelper pollHelper(out);

	// do callback first
	pollHelper.onCallBack(WHEN_QUEUE_POLL, context);

	// do poll then
	OpenSLBuffer* outBuffer = pollHelper.bufferPoll();
	int lret = pollHelper.poll(outBuffer);
	if(SL_SUCCESS(lret)){
		// do call back
		pollHelper.onCallBack(WHEN_QUEUE_POLL_SUCCESS, context);
		// keep the data is not empty
		if(outBuffer->dataLen >= 0){
			out->playBuffer(outBuffer, pBufferQueue);
		}
		// we reach the end of asset
		if(outBuffer->dataLen < outBuffer->len ){
			// may do some buffering
			int params[2] = {outBuffer->dataLen, outBuffer->len };
			pollHelper.onCallBack(WHEN_QUEUE_POLL_INEQUACY, params);
		}
	}else{
		// do call back, may be do buffering
		pollHelper.onCallBack(WHEN_QUEUE_POLL_FAILED, context);
	}
}

//-----------------------------------------------------------------
void sl_callback_simpleandroidqueue_context(SLAndroidSimpleBufferQueueItf pBufferQueue, void* context){
	OpenSLOut *out = (OpenSLOut*)(context);
	OpenSLOutTryPollHelper pollHelper(out);

	// do callback first
	pollHelper.onCallBack(WHEN_QUEUE_POLL, context);

	// do poll then
	OpenSLBuffer* outBuffer = pollHelper.bufferPoll();
	int lret = pollHelper.poll(outBuffer);
	if(SL_SUCCESS(lret)){
		// do call back
		pollHelper.onCallBack(WHEN_QUEUE_POLL_SUCCESS, context);
		// keep the data is not empty
		if(outBuffer->dataLen >= 0){
			out->playBuffer(outBuffer, pBufferQueue);
		}
		// we reach the end of asset
		if(outBuffer->dataLen < outBuffer->len ){
			// may do some buffering
			int params[2] = {outBuffer->dataLen, outBuffer->len };
			pollHelper.onCallBack(WHEN_QUEUE_POLL_INEQUACY, params);
		}
	}else{
		// do call back, may be do buffering
		pollHelper.onCallBack(WHEN_QUEUE_POLL_FAILED, context);
	}
}

//-----------------------------------------------------------------
void sl_callback_simpleandroidqueue_push_context(SLAndroidSimpleBufferQueueItf pBufferQueue, void* context){
	OpenSLOut *out = (OpenSLOut*)(context);
	OpenSLOutTryPollHelper pushHelper(out);

	// push record buffer
	OpenSLBuffer* buffer = pushHelper.bufferPush();
	buffer->dataLen = buffer->len;

	// do push
	pushHelper.push(buffer);
	pushHelper.onCallBack(WHEN_QUEUE_PUSH, buffer);

	// go on record
	int lRet = pushHelper.doGoOnPush();
	if(lRet){
		pushHelper.onCallBack(WHEN_QUEUE_GOONRECORD_SUCCESS, context);
	}else{
		pushHelper.onCallBack(WHEN_QUEUE_GOONRECORD_FAILED, context);
	}
}

//-----------------------------------------------------------------
/* Callback for AndroidBufferQueueItf through which we supply ADTS buffers */
SLresult sl_callback_androidqueue_context(
		SLAndroidBufferQueueItf caller,
		void *pCallbackContext,        /* input */
		void *pBufferContext,          /* input */
		void *pBufferData,             /* input */
		SLuint32 dataSize,             /* input */
		SLuint32 dataUsed,             /* input */
		const SLAndroidBufferItem *pItems,/* input */
		SLuint32 itemsLength           /* input */){
	OpenSLOut *out = (OpenSLOut*)(pCallbackContext);
	OpenSLOutTryPollHelper pollHelper(out);

	// do callback first
	pollHelper.onCallBack(WHEN_QUEUE_POLL, pCallbackContext);

	// do poll then
	OpenSLBuffer* outBuffer = pollHelper.bufferPoll();
	int lret = pollHelper.poll(outBuffer);
	if(SL_SUCCESS(lret)){
		// do call back
		pollHelper.onCallBack(WHEN_QUEUE_POLL_SUCCESS, pCallbackContext);
		// keep the data is not empty
		if(outBuffer->dataLen >= 0){
			out->playBuffer(outBuffer, caller);
		}
		// we reach the end of asset
		if(outBuffer->dataLen < outBuffer->len ){
			// may do some buffering
			int params[2] = {outBuffer->dataLen, outBuffer->len };
			pollHelper.onCallBack(WHEN_QUEUE_POLL_INEQUACY, params);
		}
	}else{
		// do call back, may be do buffering
		pollHelper.onCallBack(WHEN_QUEUE_POLL_FAILED, pCallbackContext);
	}

	return SL_RESULT_SUCCESS;
}

/**
 * default constructor
 * */
OpenSLOut::OpenSLOut(OpenSLDevice *device)
: OpenSLObject(device) {
	mName = "OpenSLOut";
	mState = SLOUT_STOP;
	memset(&mFormat, 0, sizeof(mFormat));

	mPlayerObj = NULL;
	mPlayer = NULL;
	mPlayerQueue = NULL;
	mPlayerAndroidQueue = NULL;
	mPlayerSimpleAndroidQueue = NULL;
}

/**
 * default destruct
 * */
OpenSLOut::~OpenSLOut(){
	// mark we are in destructor
	mark(IN_DESTRUTCTOR);
	// NB! there are may be pure virtual function call
	// as in stop() function , we will call onCallBack()
	// so you should call stop before object delete
	IF_DO(mState != SLOUT_STOP,
			JB_LOGE("openslout should call stop before delete,"
					"leak some event"));
	IF_DO(mState != SLOUT_STOP, stopInternal() );
}

/**
 * called when prepare to play the sound
 * */
bool OpenSLOut::start(const OpenSLOutFormat &format){
	J_ASSERT(mState == SLOUT_STOP);

	// check the format
	IF_DO( SL_FAILED(mDevice->checkOutFormat(this, format)), return false );
	mFormat = format;
	setFormat(format);

	// change the state
	tryCallBack(WHEN_START, &mFormat);
	mState = SLOUT_PLAYING;
	mBufferPoll.alloc(mFormat.bufferSize);

    // SL Data source
    SLDataSource lDataSource;
    lDataSource.pLocator = (void*)(&format.dataLocator);
    lDataSource.pFormat = (void*)(&format.dataFormat);

    // data sink
    SLDataSink lDataSink;
    lDataSink.pLocator = (void*)(&format.dstDataLocator);
    lDataSink.pFormat = format.dstFormatType == 0 ? NULL : (void*)(&format.dstDataFormat);

	SLresult lRes;
    if(format.locatorType == SL_DATALOCATOR_ANDROIDFD){
    	const SLuint32 lBGMPlayerIIDCount = 2;
        const SLInterfaceID lBGMPlayerIIDs[] =
            { SL_IID_PLAY, SL_IID_SEEK };
        const SLboolean lBGMPlayerReqs[] =
            { SL_BOOLEAN_TRUE, SL_BOOLEAN_TRUE };

	    SLEngineItf engine = mDevice->engine();
        lRes = (*engine)->CreateAudioPlayer(engine,
            &mPlayerObj, &lDataSource, &lDataSink,
            lBGMPlayerIIDCount, lBGMPlayerIIDs, lBGMPlayerReqs);
        if (lRes != SL_RESULT_SUCCESS) goto ERROR;

        lRes = (*mPlayerObj)->Realize(mPlayerObj, SL_BOOLEAN_FALSE);
        if (lRes != SL_RESULT_SUCCESS) goto ERROR;

        lRes = (*mPlayerObj)->GetInterface(mPlayerObj, SL_IID_PLAY, &mPlayer);
        if (lRes != SL_RESULT_SUCCESS) goto ERROR;
        lRes = (*mPlayerObj)->GetInterface(mPlayerObj, SL_IID_SEEK, &mPlayerSeek);
        if (lRes != SL_RESULT_SUCCESS) goto ERROR;

        // Enables looping and starts playing.
        lRes = (*mPlayerSeek)->SetLoop(mPlayerSeek, format.loopCount > 1 ? SL_BOOLEAN_TRUE : SL_BOOLEAN_FALSE, 0, SL_TIME_UNKNOWN);
        if (lRes != SL_RESULT_SUCCESS) goto ERROR;

    }else if(format.locatorType == SL_DATALOCATOR_ANDROIDBUFFERQUEUE){
	    SLuint32 lSoundPlayerIIDCount = 2;
	    const SLInterfaceID lSoundPlayerIIDs[] =
	        { SL_IID_PLAY, SL_IID_ANDROIDSIMPLEBUFFERQUEUE, SL_IID_ANDROIDBUFFERQUEUESOURCE };
	    const SLboolean lSoundPlayerReqs[] =
	        { SL_BOOLEAN_TRUE, SL_BOOLEAN_TRUE, SL_BOOLEAN_TRUE };
	    if(format.dstLocatorType == SL_DATALOCATOR_ANDROIDSIMPLEBUFFERQUEUE){
	    	lSoundPlayerIIDCount += 1;
	    }

	    SLEngineItf engine = mDevice->engine();
	    lRes = (*engine)->CreateAudioPlayer(engine, &mPlayerObj,
	        &lDataSource, &lDataSink, lSoundPlayerIIDCount,
	        lSoundPlayerIIDs, lSoundPlayerReqs);
	    if (lRes != SL_RESULT_SUCCESS) goto ERROR;

	    lRes = (*mPlayerObj)->Realize(mPlayerObj, SL_BOOLEAN_FALSE);
	    if (lRes != SL_RESULT_SUCCESS) goto ERROR;

	    lRes = (*mPlayerObj)->GetInterface(mPlayerObj, SL_IID_PLAY, &mPlayer);
	    if (lRes != SL_RESULT_SUCCESS) goto ERROR;

	    lRes = (*mPlayer)->SetCallbackEventsMask(mPlayer, SL_PLAYEVENT_HEADATEND);
	    slCheckErrorWithStatus(lRes, "Problem registering player callback mask (Error %d).", lRes);

	    if(format.dstLocatorType == SL_DATALOCATOR_ANDROIDSIMPLEBUFFERQUEUE){
	    	// decode result
	    	lRes = (*mPlayerObj)->GetInterface(mPlayerObj, SL_IID_ANDROIDSIMPLEBUFFERQUEUE, &mPlayerSimpleAndroidQueue);
	    	// Registers a callback called when sound is finished.
	    	lRes = (*mPlayerSimpleAndroidQueue)->RegisterCallback(mPlayerSimpleAndroidQueue, sl_callback_simpleandroidqueue_push_context, this);
	    	slCheckErrorWithStatus(lRes, "Problem registering player callback (Error %d).", lRes);
	    	// push empty buffer
	    	goOnPush();
	    }

	    lRes = (*mPlayerObj)->GetInterface(mPlayerObj, SL_IID_ANDROIDBUFFERQUEUESOURCE, &mPlayerAndroidQueue);
	    if (lRes != SL_RESULT_SUCCESS) goto ERROR;

	    // Registers a callback called when sound is finished.
	    lRes = (*mPlayerAndroidQueue)->RegisterCallback(mPlayerAndroidQueue, sl_callback_androidqueue_context, this);
	    slCheckErrorWithStatus(lRes, "Problem registering player callback (Error %d).", lRes);
    }else if(format.locatorType == SL_DATALOCATOR_ANDROIDSIMPLEBUFFERQUEUE){
    	const SLuint32 lSoundPlayerIIDCount = 2;
    	const SLInterfaceID lSoundPlayerIIDs[] =
    	{ SL_IID_PLAY, SL_IID_ANDROIDSIMPLEBUFFERQUEUE };
    	const SLboolean lSoundPlayerReqs[] =
    	{ SL_BOOLEAN_TRUE, SL_BOOLEAN_TRUE };

    	SLEngineItf engine = mDevice->engine();
    	lRes = (*engine)->CreateAudioPlayer(engine, &mPlayerObj,
    			&lDataSource, &lDataSink, lSoundPlayerIIDCount,
    			lSoundPlayerIIDs, lSoundPlayerReqs);
    	if (lRes != SL_RESULT_SUCCESS) goto ERROR;

    	lRes = (*mPlayerObj)->Realize(mPlayerObj, SL_BOOLEAN_FALSE);
    	if (lRes != SL_RESULT_SUCCESS) goto ERROR;

    	lRes = (*mPlayerObj)->GetInterface(mPlayerObj, SL_IID_PLAY, &mPlayer);
    	if (lRes != SL_RESULT_SUCCESS) goto ERROR;

    	lRes = (*mPlayerObj)->GetInterface(mPlayerObj, SL_IID_ANDROIDSIMPLEBUFFERQUEUE, &mPlayerSimpleAndroidQueue);
    	if (lRes != SL_RESULT_SUCCESS) goto ERROR;

    	// Registers a callback called when sound is finished.
    	lRes = (*mPlayerSimpleAndroidQueue)->RegisterCallback(mPlayerSimpleAndroidQueue, sl_callback_simpleandroidqueue_context, this);
    	slCheckErrorWithStatus(lRes, "Problem registering player callback (Error %d).", lRes);
    	lRes = (*mPlayer)->SetCallbackEventsMask(mPlayer, SL_PLAYEVENT_HEADATEND);
    	slCheckErrorWithStatus(lRes, "Problem registering player callback mask (Error %d).", lRes);
    }
	else {
    	JB_LOGE("do not support locatorType: %d", format.locatorType);
    	goto ERROR;
    }

    JB_LOGE("opensl out setpalying");
    lRes = (*mPlayer)->SetPlayState(mPlayer, SL_PLAYSTATE_PLAYING);
    if (lRes != SL_RESULT_SUCCESS) goto ERROR;

    tryCallBack(WHEN_START_END, &mFormat);

    return true;

ERROR:
	JB_LOGE("opensl out error: %d", lRes);
    stop();
    return false;
}

/**
 *  pause the play
 * */
bool OpenSLOut::pause(){
	J_ASSERT(mState == SLOUT_PAUSE || mState == SLOUT_PLAYING);

	tryCallBack(WHEN_PAUSE, &mState);

	if(mState == SLOUT_PAUSE){
		// goto playing
		mState = SLOUT_PLAYING;
	}else if(mState == SLOUT_PLAYING){
		// goto pause
		mState = SLOUT_PAUSE;
		return true;
	}else {
		J_ASSERT(false);
	}

	return false;
}

/**
 * stop the play
 * */
void OpenSLOut::stop(){
	J_ASSERT(mState != SLOUT_STOP);

	tryCallBack(WHEN_STOP, 0);

	mState = SLOUT_STOP;
	stopInternal();
}

/**
 * return current state
 * */
OpenSLOutState OpenSLOut::curState() const{
	return mState;
}

/**
 *	buffer, len : size given in bytes
 * */
void OpenSLOut::playBuffer(void* buffer, int len, const void* context){
	J_ASSERT(mPlayerObj != NULL);
	IF_DO(len <= 0 , return );

	SLresult lRes;
    SLuint32 lPlayerState;
    (*mPlayerObj)->GetState(mPlayerObj, &lPlayerState);

#define QUEUE_IS_CON(queue, con) ( (con == NULL && queue != NULL) || (con != NULL && con == queue) )

    if (lPlayerState == SL_OBJECT_STATE_REALIZED) {
    	if(QUEUE_IS_CON(mPlayerQueue, context)){
    		// Removes any sound from the queue.
    		lRes = (*mPlayerQueue)->Clear(mPlayerQueue);
    		if (lRes != SL_RESULT_SUCCESS) goto ERROR;
    		// Plays the new sound.
    		lRes = (*mPlayerQueue)->Enqueue(mPlayerQueue, buffer, len);
    		if (lRes != SL_RESULT_SUCCESS) goto ERROR;
    		// do call back
			void* params[] = { (void*)mPlayerQueue, buffer, &len };
			tryCallBack(WHEN_ENQUEUE_BUFFER, params);
    	}else if(QUEUE_IS_CON(mPlayerAndroidQueue, context)){
    		// Removes any sound from the queue.
    		// lRes = (*mPlayerQueue)->Clear(mPlayerQueue);
    		// if (lRes != SL_RESULT_SUCCESS) goto ERROR;
    		JB_LOGE("android queue enqueue buffer %d", len);
    		// Plays the new sound.
    		lRes = (*mPlayerAndroidQueue)->Enqueue(mPlayerAndroidQueue, NULL /*pBufferContext*/,
    				buffer, len, NULL, 0);
    		// do call back
			void* params[] = { (void*)mPlayerAndroidQueue, buffer, &len };
			tryCallBack(WHEN_ENQUEUE_BUFFER, params);
    		if (lRes != SL_RESULT_SUCCESS) goto ERROR;
    	}else if(QUEUE_IS_CON(mPlayerSimpleAndroidQueue, context)){
    		// Removes any sound from the queue.
    		lRes = (*mPlayerSimpleAndroidQueue)->Clear(mPlayerSimpleAndroidQueue);
    		if (lRes != SL_RESULT_SUCCESS) goto ERROR;
    		// Plays the new sound.
    		lRes = (*mPlayerSimpleAndroidQueue)->Enqueue(mPlayerSimpleAndroidQueue, buffer, len);
    		if (lRes != SL_RESULT_SUCCESS) goto ERROR;
			// do call back
			void* params[] = { (void*)mPlayerSimpleAndroidQueue, buffer, &len };
			tryCallBack(WHEN_ENQUEUE_BUFFER, params);
    	}
   }

   return;

ERROR:
   JB_LOGE("Error trying to play sound: %d", lRes);
}

/**
 * play buffer
 * */
void OpenSLOut::playBuffer(OpenSLBuffer *buffer, const void* context){
	playBuffer(buffer->data, buffer->dataLen, context);
}

/**
 * go on push
 * */
int OpenSLOut::goOnPush(){
	IF_DO(mPlayerObj == NULL, return SLERROR_ERR);
	IF_DO(mPlayerSimpleAndroidQueue == NULL, return SLERROR_ERR);

	SLresult lRes;
	SLuint32 lRecorderState;
	(*mPlayerObj)->GetState(mPlayerObj, &lRecorderState);
	if (lRecorderState == SL_OBJECT_STATE_REALIZED) {
		// be sure the buffer is recordSize alloc
		mBufferPush.alloc(mFormat.recordSize);
		// Provides a buffer for recording.
		lRes = (*mPlayerSimpleAndroidQueue)->Enqueue(mPlayerSimpleAndroidQueue,
				mBufferPush.data, mBufferPush.len);
		if (lRes != SL_RESULT_SUCCESS) goto ERROR;
		// do call back
		void* params[] = { (void*)mPlayerSimpleAndroidQueue, &mBufferPush, &mBufferPush.len };
		tryCallBack(WHEN_ENQUEUE_BUFFER, params);
	}
	return SLERROR_OK;

ERROR:
	JB_LOGE("go on push error");
	return SLERROR_ERR;
}

/**
 * go on poll
 * */
int OpenSLOut::goOnPoll(){
	IF_DO(mPlayerObj == NULL, return SLERROR_ERR);
	IF_DO(mPlayerAndroidQueue == NULL, return SLERROR_ERR);

	SLresult lRes;
	SLuint32 lRecorderState;
	(*mPlayerObj)->GetState(mPlayerObj, &lRecorderState);
	if (lRecorderState == SL_OBJECT_STATE_REALIZED) {
		// be sure the buffer is recordSize alloc
		mBufferPoll.alloc(mFormat.bufferSize);
	    int pollRet = tryQueuePoll(&mBufferPoll);
	    if(SL_SUCCESS(pollRet)){
	    	playBuffer(&mBufferPoll, mPlayerAndroidQueue);
	    }else{
	    	goto ERROR;
	    }
	}
	return SLERROR_OK;

ERROR:
	JB_LOGE("go on poll error");
	return SLERROR_ERR;
}
/**
 * there are no state change, no callback will execute
 * */
void OpenSLOut::stopInternal(){
	// base class stop
	OpenSLObject::stopInternal();
	// free userdata first
	IF_DO(mUserData != NULL, freeUserData(mUserData));
	// free opensl object
	safe_destroy_slobj(mPlayerObj);
	// clear pointer
	mPlayerQueue = NULL;
	mPlayer = NULL;
	mPlayerSeek = NULL;
	mPlayerAndroidQueue = NULL;
	mPlayerSimpleAndroidQueue = NULL;
}
