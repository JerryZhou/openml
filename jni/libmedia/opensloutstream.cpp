#include "opensloutstream.h"
#include "opensldevice.h"


/**
 * default constructor
 * */
OpenSLOutStream::OpenSLOutStream(OpenSLDevice *device):OpenSLOut(device){
	mName = "OpenSLOutStream";
	memset(&mStreamConfig, 0, sizeof(mStreamConfig));
}

/**
 * default destruct
 * */
OpenSLOutStream::~OpenSLOutStream(){
}

/**
 * called when prepare to play the sound
 * */
bool OpenSLOutStream::startStream(const OpenSLStreamConfig &streamConfig){
	IF_DO(streamConfig.slPoll == NULL, return false );

	mStreamConfig = streamConfig;
	return start(mStreamConfig);
}

/**
 * */
void OpenSLOutStream::onCallBack(int when, void* context){
	if(when == WHEN_PAUSE){
		// TODO: imp the pause
	}
}

/**
 * play the stream
 * */
bool OpenSLOutStream::play(){
	mBufferPoll.alloc(mFormat.bufferSize);
	int pollRet = tryQueuePoll(&mBufferPoll);
	if(SL_SUCCESS(pollRet)){
		playBuffer(&mBufferPoll);
		return true;
	}
	return false;
}

//---------------------------------------------------------------------------------
static OpenSLOutStream* streamOut[5] = {0};
bool startStream(const OpenSLStreamConfig &streamConfig, int idx){
	IF_DO(idx < 0 || idx > __countof(streamOut), return false);
	OpenSLOutStream * &stream = streamOut[idx];

	if(stream == NULL){
		stream = new OpenSLOutStream(slDevice());
	}

	if(stream->curState() != SLOUT_STOP){
		stream->stop();
	}

	return stream->startStream(streamConfig);
}

void stopStream(int idx){
	IF_DO(idx < 0 || idx > __countof(streamOut), return);
	OpenSLOutStream * stream = streamOut[idx];

	if(stream && stream->curState() != SLOUT_STOP){
		stream->stop();
	}
}

bool playStream(int idx){
	IF_DO(idx < 0 || idx > __countof(streamOut), return false);
	OpenSLOutStream * stream = streamOut[idx];
	IF_DO(stream == NULL, return false);
	IF_DO(stream->curState() == SLOUT_STOP, return false);

	stream->play();
	return true;
}
