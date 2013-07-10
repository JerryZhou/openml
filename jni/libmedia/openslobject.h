#include "stdpre.h"
#include "openslformat.h"

#ifndef __OPENSLOBJECT_H_
#define __OPENSLOBJECT_H_

//--------------------------------------------------------
class OpenSLDevice;

/**
 * opensl device for opensl
 * */
class OpenSLObject{
public:
	/// default constructor
	OpenSLObject(OpenSLDevice* device);
	/// default destruct
	virtual ~OpenSLObject();

	/// when the in buffer events are happened
	virtual void onCallBack(int when, void* context) {};

	/// every in must imp this interface to export the data to device
	virtual int push(OpenSLBuffer* buffer)const {return SLERROR_OK; }

	/// every in must imp this interface to export the data to device
	virtual int poll(OpenSLBuffer* buffer) {return SLERROR_OK; }

	/// to extend the out : must keep the data is null when destruct
	void* userData() const;
	void setUserData(void *data);
	void setUserDataConfig(sl_alloc_userData jalloc, sl_free_userData jfree);
	void* allocUserData(void* params) const;
	void freeUserData(void *) const;

protected:
	/// format
	void setFormat(const SLObjectFormat& format);
	/// take log about poll
	int queuePollCount() const;
	/// try poll, the same as call poll, except of take a log
	int tryQueuePoll(OpenSLBuffer* buffer);
	/// take log about push
	int queuePushCount() const;
	/// try push, the same as call push, except of loging
	int tryQueuePush(OpenSLBuffer* buffer);
	/// try call back
	void tryCallBack(int when, void* context);
	/// stop it internal : no callback
	void stopInternal();

	/// internal buffer
	OpenSLBuffer* bufferPoll() const;
	/// internal buffer for push
	OpenSLBuffer* bufferPush() const;
	/// get the record buffer
	void tryPoll(OpenSLBuffer* buffer);
	/// try push
	void tryPush(OpenSLBuffer* buffer);

	/// internal object flag: set flag
	int mark(int flag);
	/// internal object flag: clear flag
	int unmark(int flag);
	/// internal object flag: is flag set
	bool isMark(int flag)const;
	/// internal object flag returned
	int flag()const;

	/// device
	OpenSLDevice *mDevice;

	/// format
	SLObjectFormat mSLObjectFormat;

	 /// user data, should clear it when stop
    void *mUserData;

    /// buffer, if you used it,
    /// you should clearly know, it will used in queue poll thread
    /// internal buffer
    OpenSLBuffer mBufferPoll;
    OpenSLBuffer mBufferPush;

    /// log the count of poll times
    int mQueuePollCount;

    /// push out
    int mQueuePushCount;

    /// flag
    int mFlag;

    /// name
    const char* mName;
};


#endif
