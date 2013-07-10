#include "stdpre.h"
#include "openslformat.h"

#ifndef __OPENSLDEVICE_H_
#define __OPENSLDEVICE_H_

//-----------------------------------------------------------------------------
class OpenSLOut;
class OpenSLIn;

/**
 * opensl device for audio
 * */
class OpenSLDevice{
public:
	/// default constructor
	OpenSLDevice();
	/// init the device
	bool start();
	/// uninit the device
	void stop();

	/// create a output mix object
	bool createOutputMixObj();
	/// get the unique output mix obj
	SLObjectItf outputMixObj() const;

	/// return the sl engine
	SLEngineItf engine() const;
	SLObjectItf engineObject() const;


	/// check the format is right
	int checkOutFormat(OpenSLOut* out, const OpenSLOutFormat& format) const;
	int checkInFormat(OpenSLIn* in, const OpenSLInFormat& format) const;

private:
	SLObjectItf mEngineObj;
	SLEngineItf mEngineInterface;

	SLObjectItf mOutputMixObj;
};

//----------------------------------------------------------------------
// Global device settings
//----------------------------------------------------------------------

/**
 * */
void slDevice_start();

/**
 * */
void slDevice_stop();

/**
 * */
OpenSLDevice* slDevice();

#endif
