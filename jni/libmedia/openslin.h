#include "stdpre.h"
#include "openslobject.h"

#ifndef __OPENSLIN_H_
#define __OPENSLIN_H_

//--------------------------------------------------------
class OpenSLDevice;
class OpenSLInTryPushHelper;

/**
 * opensl device for audio in
 * */
class OpenSLIn : public OpenSLObject{
public:
	/// default constructor
	OpenSLIn(OpenSLDevice* device);
	/// default destruct
	virtual ~OpenSLIn();

	/// when the in buffer events are happened
	virtual void onCallBack(int when, void* context) const {};

	/// every in must imp this interface to export the data to device
	virtual int push(OpenSLBuffer* buffer)const {};

	/// called when prepare to play the sound
	bool start(const OpenSLInFormat &format);
	/// pause the play, return true if current are playing
	bool pause();
	/// stop the play
	void stop();

	/// return current out device state
	OpenSLInState curState() const;

protected:
	/// freind class to visitor the protected class
	friend class OpenSLInTryPushHelper;
	/// rebegin record sound to
	int record();
	/// go on record
	int goOnRecord();
	/// release all the resource
	void stopInternal();

	/// device format and state
	OpenSLInFormat mFormat;
	OpenSLInState mState;

	/// opensl object and interface
	SLObjectItf mRecorderObj;
	SLRecordItf mRecorder;
    SLAndroidSimpleBufferQueueItf mRecorderQueue;
};

//--------------------------------------------------------------------------------
// Test Suit
//--------------------------------------------------------------------------------
bool startRecord(const OpenSLInFormat &format, int idx = 0);

void stopRecord(int idx = 0);


#endif
