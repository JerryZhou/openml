#include "stdpre.h"
#include "openslout.h"
#include <asset.h>

#ifndef __OPENSLOUTASSET_H_
#define __OPENSLOUTASSET_H_

//--------------------------------------------------------
class OpenSLDevice;

/**
 * opensl device for audio out
 * */
class OpenSLOutAsset : public OpenSLOut{
public:
	/// default constructor
	OpenSLOutAsset(OpenSLDevice* device);
	/// default destruct
	virtual ~OpenSLOutAsset();

	/// when the in buffer events are happened
	virtual void onCallBack(int when, void* context) ;

	/// every in must imp this interface to export the data to device
	virtual int poll(OpenSLBuffer* buffer) ;

	/// start play assert
	bool startPlay(const char* path);

protected:
	AssetEntry mEntry;
};

//--------------------------------------------------------------------------------
// Test Suit
//--------------------------------------------------------------------------------
void playAsset(const char* sound, int idx = 0);
void stopAsset(int idx = 0);


#endif
