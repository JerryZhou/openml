#include "stdpre.h"
#include "openslout.h"
#include <asset.h>

#ifndef __OPENSLOUTSTREAM_H_
#define __OPENSLOUTSTREAM_H_

//--------------------------------------------------------
class OpenSLDevice;

/**
 * opensl device for audio out
 * */
class OpenSLOutStream : public OpenSLOut{
public:
	/// default constructor
	OpenSLOutStream(OpenSLDevice* device);
	/// default destruct
	virtual ~OpenSLOutStream();

	/// when the in buffer events are happened
	virtual void onCallBack(int when, void* context) ;

	///-----------------------------------------------------
	/// lifecycle: startStream  -- play --- pause ----- pause ------ stop
	//------------------------------------------------------

	/// start stream
	bool startStream(const OpenSLStreamConfig &streamConfig);

	/// play
	bool play();

protected:
	OpenSLStreamConfig mStreamConfig;
};

//--------------------------------------------------------------------------------
// Test Suit
//--------------------------------------------------------------------------------
bool startStream(const OpenSLStreamConfig &streamConfig, int idx = 0);
void stopStream(int idx = 0);
bool playStream(int idx = 0);


#endif
