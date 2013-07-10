#include "stdpre.h"
#include "openslobject.h"

#ifndef __OPENSLOUT_H_
#define __OPENSLOUT_H_

//--------------------------------------------------------
class OpenSLDevice;
class OpenSLOutTryPollHelper;

/**
 * opensl device for audio out
 * */
class OpenSLOut : public OpenSLObject{
public:
	/// default constructor
	OpenSLOut(OpenSLDevice* device);
	/// default destruct
	virtual ~OpenSLOut();

	///-----------------------------------------------------
	/// do not directy call the pure virtul function
	///-----------------------------------------------------

	/// when the in buffer events are happened
	virtual void onCallBack(int when, void* context){ } ;

	/// every in must imp this interface to export the data to device
	virtual int poll(OpenSLBuffer* buffer){return SLERROR_OK;} ;

	///-----------------------------------------------------
	/// lifecycle: start --- pause ----- pause ------ stop
	//------------------------------------------------------

	/// called when prepare to play the sound
	bool start(const OpenSLOutFormat &format);
	/// pause the play, return true if current are playing
	bool pause();
	/// stop the play
	void stop();

	/// return current out device state
	OpenSLOutState curState() const;

	/// low level interface to handle buffer direct
	void playBuffer(void* buffer, int len, const void* context = NULL);
	void playBuffer(OpenSLBuffer *buffer, const void* context = NULL);

protected:
	/// go on Push
	int goOnPush();
	/// go on poll
	int goOnPoll();
	/// stop it internal : no callback
	void stopInternal();
	/// helper class to visitor protcted members
	friend class OpenSLOutTryPollHelper;

	/// opensl format and state
	OpenSLOutFormat mFormat;
	OpenSLOutState mState;

	/// sl out context
	SLObjectItf mPlayerObj;
	SLPlayItf mPlayer;
    SLBufferQueueItf mPlayerQueue;
    SLAndroidBufferQueueItf mPlayerAndroidQueue;
    SLAndroidSimpleBufferQueueItf mPlayerSimpleAndroidQueue;
    SLSeekItf mPlayerSeek;
};

#endif
