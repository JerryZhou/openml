#include "openslmedia_test.h"
#include "opensldevice.h"
#include "opensloutstream.h"
#include "opensloutasset.h"
#include "openslin.h"

#include "codec.h"


ThreadMutex bufferMutex;
std::list<OpenSLBuffer*> records;

void initRecordBuffer(){
	LOGE("init mutex and buffer");
}

//--------------------------------------------------------------------------
void recordInPush(void* in, OpenSLBuffer* buffer){
	OpenSLIn * recordIn = (OpenSLIn*)(in);

	do{
		AutoLock autoLock(bufferMutex);
		if(buffer->dataLen > 0){
			// copy data to record
			OpenSLBuffer *addToRecord = new OpenSLBuffer(*buffer);
			records.push_back(addToRecord);
			LOGE("record push: %d", buffer->dataLen);
		}
	}while(false);
}

//--------------------------------------------------------------------------
int pollRecordCount = 0;
int streamPollFromRecordWithAutoFill(void* out, OpenSLBuffer* buffer,  void* context){
	OpenSLOutStream* outStream = (OpenSLOutStream*)(out);

	do{
		AutoLock autoLock(bufferMutex);
		if(records.size() > 0){
			pollRecordCount = 0;
			// copy data to buffer
			OpenSLBuffer * recordFront = records.front();
			*buffer = *recordFront;
			LOGE("stream poll: %d", buffer->dataLen);
			records.pop_front();
			delete recordFront;
		}else{
			if(pollRecordCount > 10){
				return SLERROR_ERR;
			}
			pollRecordCount++;
			buffer->dataLen = buffer->len;
			memset(buffer->data, 0, buffer->len);
			LOGE("stream poll-fill empty voice: %d", buffer->len);
		}
	}while(false);

	return SLERROR_OK;
}


AssetEntry assetToAAC = 0;
static int fd = 0;
static void *ptr;
static unsigned char *frame;
static size_t filelen;
static int pollCount = 0;
int streamPollFromAssetWithAutoFill(void* out, OpenSLBuffer* buffer, void* context){
	OpenSLOutStream* outStream = (OpenSLOutStream*)(out);
	LOGE("streamPollFromAssetWithAutoFill");
	if(assetToAAC == 0){
		//fd = open("sdcard/example2.aac", O_RDONLY);
		assetToAAC = asset_open("assets/000001.mp3");
		LOGE("streamPollFromAssetWithAutoFill: %d", assetToAAC);
		IF_DO(assetToAAC == 0, return assetToAAC);
	}

	++pollCount;

	do{
		// read 9 bytes
		int readLen = asset_read(assetToAAC, buffer->data, adts_len_no_crc);
		if(readLen == adts_len_no_crc){
			AdtsHeader adts = {0};
			serialFrom(&adts, buffer->data);
			logAdts(adts);

			readLen = asset_read(assetToAAC, (char*)(buffer->data) + adts_len_no_crc, adts.frameLength - adts_len_no_crc);
			if(readLen == (adts.frameLength - adts_len_no_crc)){
				buffer->dataLen = adts.frameLength;
			}
		}else{
			safe_close_asset(assetToAAC);
		}
		LOGE("stream poll data: %d-%d", buffer->len, buffer->dataLen);
	}while(false);

	return SLERROR_OK;
}

//---------------------------------------------------------------------------------
//------------------------------- example------------------------------------------
//---------------------------------------------------------------------------------
void example_record_to_play(){
	// record
	LOGI("init record");
	OpenSLInFormat inFormat;
	inFormat.slPush = recordInPush;
	inFormat.recordSize = 4410;
	startRecord(inFormat, 0);

	LOGI("init stream player");
	OpenSLStreamConfig streamPlayRecordFormat;
	streamPlayRecordFormat.slPoll = streamPollFromRecordWithAutoFill;
	startStream(streamPlayRecordFormat, 0);
	playStream(0);
}

void example_play_asset(){
	playAsset("assets/000001.mp3");
}

void example_play_aac_stream_with_harddecode(){
	// format
	SLDataFormat_PCM sourceFormat;
	sourceFormat.formatType = SL_DATAFORMAT_PCM;
	sourceFormat.numChannels = 2;
	sourceFormat.samplesPerSec = SL_SAMPLINGRATE_32;
	sourceFormat.bitsPerSample = SL_PCMSAMPLEFORMAT_FIXED_16;
	sourceFormat.containerSize = SL_PCMSAMPLEFORMAT_FIXED_16;
	sourceFormat.bitsPerSample = SL_PCMSAMPLEFORMAT_FIXED_16;
	sourceFormat.channelMask = channelCountToMask(2);
	sourceFormat.endianness = SL_BYTEORDER_LITTLEENDIAN;

	// decode adts aac to pcm
	LOGI("init aac decode");
	OpenSLStreamConfig streamConfig;
	streamConfig.streamType = 0;
	streamConfig.slPoll = streamPollFromAssetWithAutoFill;
	streamConfig.slPush = recordInPush;

	streamConfig.dstFormatType = SL_DATAFORMAT_PCM;
	streamConfig.dstDataFormat.dataFormat_PCM = sourceFormat;// use the default pcm format

	streamConfig.dstLocatorType = SL_DATALOCATOR_ANDROIDSIMPLEBUFFERQUEUE;
	streamConfig.dstDataLocator.dataLocator_AndroidSimpleBufferQueue.locatorType = SL_DATALOCATOR_ANDROIDSIMPLEBUFFERQUEUE;
	streamConfig.dstDataLocator.dataLocator_AndroidSimpleBufferQueue.numBuffers = 1;

	streamConfig.formatType = SL_DATAFORMAT_MIME;
	streamConfig.dataFormat.dataFormat_MIME.formatType 		= SL_DATAFORMAT_MIME;
	streamConfig.dataFormat.dataFormat_MIME.mimeType 			= SL_ANDROID_MIME_AACADTS;
	streamConfig.dataFormat.dataFormat_MIME.containerType 	= SL_CONTAINERTYPE_RAW;

	streamConfig.locatorType = SL_DATALOCATOR_ANDROIDBUFFERQUEUE;
	streamConfig.dataLocator.dataLocator_AndroidBufferQueue.locatorType = SL_DATALOCATOR_ANDROIDBUFFERQUEUE;
	streamConfig.dataLocator.dataLocator_AndroidBufferQueue.numBuffers = 1;

#define SAMPLES_PER_AAC_FRAME 1024
#define BUFFER_SIZE_IN_BYTES   (2*sizeof(short)*SAMPLES_PER_AAC_FRAME)
#define NB_BUFFERS_IN_PCM_QUEUE 2
	streamConfig.recordSize = BUFFER_SIZE_IN_BYTES * NB_BUFFERS_IN_PCM_QUEUE;

	// start decode
	startStream(streamConfig, 0);
	playStream(0);

	// play the decode aac stream
	LOGI("init stream player");
	OpenSLStreamConfig streamPlayRecordFormat;
	streamPlayRecordFormat.dataFormat.dataFormat_PCM = sourceFormat;
	streamPlayRecordFormat.slPoll = streamPollFromRecordWithAutoFill;
	startStream(streamPlayRecordFormat, 1);
	playStream(1);
}

extern int playAACDecode();
void example_decode_aac_stream(){
	// play aac decode
	playAACDecode();
}

//--------------------------------------------------------------------------
// Test Media
//--------------------------------------------------------------------------
void test_media_main(){
	initRecordBuffer();

	// 1
	//example_record_to_play();
	// 2
	//example_play_asset();
	// 3
	//example_play_aac_stream_with_harddecode();
	// 4
	//example_decode_aac_stream();
}

