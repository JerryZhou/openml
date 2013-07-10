#include "opensloutasset.h"
#include "opensldevice.h"
#include "asset.h"


/**
 * default constructor
 * */
OpenSLOutAsset::OpenSLOutAsset(OpenSLDevice *device):OpenSLOut(device){
	mEntry = 0;
	mName = "OpenSLOutAsset";
}

/**
 * default destruct
 * */
OpenSLOutAsset::~OpenSLOutAsset(){
	safe_close_asset(mEntry);
}

/**
 * called when prepare to play the sound
 * */
bool OpenSLOutAsset::startPlay(const char* path){
	IF_DO(path == NULL, return false );

	AssetContext context;
	context.path = path;

	OpenSLOutFormat format;
	format.context = &context;

	//get ext
	const char* ptr = strrchr(path, '.');
	if(ptr){
		LOGI("opensloutasset: start play, ext: %s", ptr);
		if(strcasecmp(".PCM", ptr) == 0){

		}else { //if(strcasecmp(".mp3", ptr) == 0){
			AssetDescriptor descriptor = {0};
			if(asset_option(0, ASSET_GET_ANDROID_DESCRIPTOR, &descriptor, (void*)path) == 0){
				LOGI("succesful load asset: %s, fd: %d, offset: %d, length: %d",
						path, descriptor.fd, descriptor.start, descriptor.length);
				format.formatType = SL_DATAFORMAT_MIME;
				format.dataFormat.dataFormat_MIME.formatType 		= SL_DATAFORMAT_MIME;
				format.dataFormat.dataFormat_MIME.mimeType 			= NULL;
				format.dataFormat.dataFormat_MIME.containerType 	= SL_CONTAINERTYPE_UNSPECIFIED;

				format.locatorType = SL_DATALOCATOR_ANDROIDFD;
				format.dataLocator.dataLocator_AndroidFD.locatorType = SL_DATALOCATOR_ANDROIDFD;
		        format.dataLocator.dataLocator_AndroidFD.fd          = descriptor.fd;
		        format.dataLocator.dataLocator_AndroidFD.offset      = descriptor.start;
		        format.dataLocator.dataLocator_AndroidFD.length      = descriptor.length;
			}else{
				LOGE("falied to load mime source: %s", path);
				return false;
			}
		}
	}

	return start(format);
}

/**
 * */
void OpenSLOutAsset::onCallBack(int when, void* context){
	if(when == WHEN_START){
		// load assert
		OpenSLOutFormat* format = (OpenSLOutFormat*)(context);
		IF_DO(format->formatType != SL_DATAFORMAT_PCM, return);

		AssetContext *assetContext = (AssetContext*)(format->context);
		AssetEntry entry = asset_open(assetContext->path);

		LOGI("on start :%s , %d", assetContext->path, entry);
		if(entry != 0){
			safe_close_asset(mEntry);
			mEntry = entry;
		}
	}else if(when == WHEN_START_END){
		OpenSLOutFormat* format = (OpenSLOutFormat*)(context);
		IF_DO(format->formatType != SL_DATAFORMAT_PCM, return);

		// start the buffer in a right length
		if(mEntry != 0){
			int len ;
			asset_option(mEntry, ASSET_GET_LENGTH, &len);
			len = clamp(mFormat.bufferSize, 1024, len);
			mBufferPoll.alloc(len);
		}

		// send the first buffer
		poll(&mBufferPoll);
		playBuffer(&mBufferPoll);
	}else if(when == WHEN_STOP){
		LOGI("on stop: %d", mEntry);
		safe_close_asset(mEntry);
	}else if(when == WHEN_PAUSE){
		// TODO: imp the pause
	}
}

/**
 * */
int OpenSLOutAsset::poll(OpenSLBuffer* buffer){
	IF_DO(mEntry == 0, return SLERROR_FAILED);
	mBufferPoll.dataLen = asset_read(mEntry, mBufferPoll.data, mBufferPoll.len);
	LOGI("poll : %d, %d", mBufferPoll.dataLen, mBufferPoll.len);
	if(mBufferPoll.dataLen > 0){
		// we reach the end of asset
		if(mBufferPoll.dataLen < mBufferPoll.len ){
			// need to support the loop
		}
		return SLERROR_OK;
	}else{
		// need to stop
	}

	return SLERROR_FAILED;
}

//---------------------------------------------------------------------------------
static OpenSLOutAsset* assetOut[5] = {0};
void playAsset(const char* sound, int idx){
	IF_DO(idx < 0 || idx > __countof(assetOut), return);
	OpenSLOutAsset* &asset = assetOut[idx];

	// if no device new a out device
	IF_DO(asset == NULL, asset = new OpenSLOutAsset(slDevice()));

	// stop first
	IF_DO(asset->curState() != SLOUT_STOP, asset->stop());

	// play sound
	asset->startPlay(sound);
}

void stopAsset(int idx){
	IF_DO(idx < 0 || idx > __countof(assetOut), return);
	OpenSLOutAsset* &asset = assetOut[idx];
	IF_DO(asset == NULL, return);

	// stop it
	IF_DO(asset->curState() != SLOUT_STOP, asset->stop());
}
