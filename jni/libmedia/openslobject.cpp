#include <jni.h>
#include "openslobject.h"
#include "opensldevice.h"

/**
 * default constructor
 * */
OpenSLObject::OpenSLObject(OpenSLDevice *device)
: mDevice(device)
, mFlag(0)
, mQueuePollCount(0)
, mQueuePushCount(0)
, mName("OpenSLObject"){
	J_ASSERT(mDevice != NULL);

	mUserData = NULL;
	memset(&mSLObjectFormat, 0, sizeof(mSLObjectFormat));
}

/**
 *
 * */
OpenSLObject::~OpenSLObject(){
	// mark
	mark(IN_DESTRUTCTOR);
	// free userdata first
	IF_DO(mUserData != NULL, freeUserData(mUserData));

	// clear device ref
	mDevice = NULL;
}

/**
 * */
void OpenSLObject::setFormat(const SLObjectFormat& format){
	mSLObjectFormat = format;
}

/**
 * */
OpenSLBuffer* OpenSLObject::bufferPoll() const{
	return (OpenSLBuffer*)(&mBufferPoll);
}

/**
 * internal buffer for push
 * */
OpenSLBuffer* OpenSLObject::bufferPush() const{
	return (OpenSLBuffer*)(&mBufferPush);
}

/**
 * get the record buffer
 * */
void OpenSLObject::tryPoll(OpenSLBuffer* buffer){
	IF_DO(isMark(IN_DESTRUTCTOR), return );
	int params[2] = { mQueuePollCount, (int)buffer};
	tryCallBack(WHEN_QUEUE_PUSH_COUNT, params);

	if(mSLObjectFormat.slPoll != NULL){
		if( SL_SUCCESS( mSLObjectFormat.slPoll((void*)this, buffer, 0) ) ){
			return ;
		}
	}

	poll(buffer);
}

/**
 * try push
 * */
void OpenSLObject::tryPush(OpenSLBuffer* buffer){
	IF_DO(isMark(IN_DESTRUTCTOR), return );
	int params[2] = { mQueuePushCount, (int)buffer};
	tryCallBack(WHEN_QUEUE_PUSH_COUNT, params);

	if(mSLObjectFormat.slPush != NULL){
		mSLObjectFormat.slPush((void*)this, buffer);
	}

	push(buffer);
}

/**
 * queue poll count since start
 * */
int OpenSLObject::queuePollCount() const{
	return mQueuePollCount;
}

/**
 * delegate to poll
 * */
int OpenSLObject::tryQueuePoll(OpenSLBuffer* buffer){
	IF_DO(isMark(IN_DESTRUTCTOR), return SLERROR_ERR);
	int params[2] = { mQueuePollCount, (int)buffer};
	tryCallBack(WHEN_QUEUE_POLL_COUNT, params);

	if(mSLObjectFormat.slPoll){
		return mSLObjectFormat.slPoll(this, buffer, 0);
	}

	return poll(buffer);
}

/**
 * take log about push
 * */
int OpenSLObject::queuePushCount() const{
	return mQueuePushCount;
}

/**
 * try push, the same as call push, except of loging
 * */
int OpenSLObject::tryQueuePush(OpenSLBuffer* buffer){
	IF_DO(isMark(IN_DESTRUTCTOR), return SLERROR_ERR);
	int params[2] = { mQueuePushCount, (int)buffer};
	tryCallBack(WHEN_QUEUE_PUSH_COUNT, params);

	if(mSLObjectFormat.slPush){
		mSLObjectFormat.slPush(this, buffer);
	}

	return push(buffer);
}

/**
 * try call back
 * */
void OpenSLObject::tryCallBack(int when, void* context){
	if(mSLObjectFormat.slCallBack){
		mSLObjectFormat.slCallBack(this, when, context);
	}

	onCallBack(when, context);
}

/**
 * clear internal data
 * */
void OpenSLObject::stopInternal(){
	mQueuePushCount = 0;
	mQueuePollCount = 0;
}

/**
 * */
void* OpenSLObject::userData() const{
	return mUserData;
}

/**
 * */
void OpenSLObject::setUserData(void *data){
	IF_DO(mSLObjectFormat.slAllocUserData != NULL, mSLObjectFormat.slAllocUserData(this, mUserData));
	mUserData = data;
}

/**
 * */
void OpenSLObject::setUserDataConfig(sl_alloc_userData jalloc, sl_free_userData jfree){
	mSLObjectFormat.slAllocUserData = jalloc;
	mSLObjectFormat.slFreeUserData = jfree;
}

/**
 * */
void* OpenSLObject::allocUserData(void* params) const{
	IF_DO(mSLObjectFormat.slAllocUserData == NULL, return NULL);
	return mSLObjectFormat.slAllocUserData((void*)this, params);
}

/**
 * */
void OpenSLObject::freeUserData(void * data) const{
	IF_DO(mSLObjectFormat.slFreeUserData == NULL, return);
	mSLObjectFormat.slFreeUserData((void*)this, data);
}


/**
 * */
int OpenSLObject::mark(int flag){
	mFlag |= flag;
}

/**
 * */
int OpenSLObject::unmark(int flag){
	mFlag &= ~flag;
}

/**
 * */
bool OpenSLObject::isMark(int flag)const{
	return (((mFlag & flag) ^ flag) == 0);
}

/**
 * */
int OpenSLObject::flag()const{
	return mFlag;
}
