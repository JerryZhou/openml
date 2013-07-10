#include "stdpre.h"

#ifndef __OPENSLFORMAT_H_
#define __OPENSLFORMAT_H_

/**
 * */
enum OpenSLOutState{
	SLOUT_STOP,
	SLOUT_PAUSE,
	SLOUT_PLAYING,
};

/**
 * */
enum OpenSLErrorCode{
	SLERROR_OK = 0,
	SLERROR_ERR = 1,
	SLERROR_FAILED = 1,
};

/**
 * */
enum OpenSLObjectFlag{
	IN_CONSTRUCTOR = 0x01,
	IN_DESTRUTCTOR = 0x02,
};

/**
 * */
#define SL_SUCCESS(err) ((err) == SLERROR_OK)
#define SL_FAILED(err) (!(SL_SUCCESS(err)))

/**
 * */
enum SLCallBackWhen{
	WHEN_START,		// 0
	WHEN_START_END,		// 1
	WHEN_PAUSE, // 2
	WHEN_STOP, // 3

	WHEN_QUEUE_POLL, // 4
	WHEN_QUEUE_POLL_COUNT, // 5
	WHEN_QUEUE_POLL_FAILED, // 6
	WHEN_QUEUE_POLL_SUCCESS, // 7
	WHEN_QUEUE_POLL_INEQUACY, // 8

	WHEN_QUEUE_PUSH, // 9
	WHEN_QUEUE_PUSH_COUNT, // 10
	WHEN_QUEUE_GOONRECORD_SUCCESS, // 11
	WHEN_QUEUE_GOONRECORD_FAILED, // 12

	WHEN_ENQUEUE_BUFFER, // 13
};

/**
 * */
struct OpenSLBuffer{
	void * data; // data
	int len; // data buffer memory len
	int dataLen; // data content len

	OpenSLBuffer(): data(NULL), len(0), dataLen(0){

	}

	OpenSLBuffer(int size):  data(NULL), len(0), dataLen(0){
		alloc(size);
	}

	OpenSLBuffer(OpenSLBuffer& other): data(NULL), len(0), dataLen(0){
		*this = other;
	}

	OpenSLBuffer& operator=(const OpenSLBuffer& other){
		IF_DO(this == &other, return *this );

		alloc(other.len);
		IF_DO(other.dataLen > 0, memcpy(this->data, other.data, other.dataLen) );
		this->dataLen = other.dataLen;

		return *this;
	}

	~OpenSLBuffer(){
		if(data != NULL) free(data);
	}

	int alloc(int ilen){
		IF_DO(len == ilen, return ilen);
		IF_DO(ilen <= 0, return ilen);

		if(data != NULL) free(data);
		data = malloc(ilen);
		len = ilen;

		return len;
	}
};

/**
 * if you use the userData in openslobject, you should imp the next function
 * */
typedef void* (*sl_alloc_userData)(void* slThis, void* params);

/**
 * free user data
 * */
typedef void (*sl_free_userData)(void* slThis, void* userData);

/**
 * @param when, see SLOutCallBackWhen
 * */
typedef void (*sl_stream_call_back)(void* out, int when, void* context);

/**
 * @param out, this pointer
 * @return SLERROR_OK if successful
 * */
typedef int (*sl_stream_poll)(void* out, OpenSLBuffer* buffer,  void* context);

/**
 * @param when, see SLOutCallBackWhen
 * */
typedef void (*sl_in_call_back)(void* in, int when, void* context);

/**
 * @param in, this pointer
 * */
typedef void (*sl_in_push)(void* in, OpenSLBuffer* buffer);

/**
 * sl object call backs
 * */
struct SLObjectFormat{
	sl_alloc_userData slAllocUserData;
	sl_free_userData slFreeUserData;
	sl_stream_call_back slCallBack;
	sl_stream_poll slPoll;
	sl_in_push slPush;

	SLObjectFormat(): slAllocUserData(NULL)
	, slFreeUserData(NULL)
	, slCallBack(NULL)
	, slPoll(NULL)
	, slPush(NULL) {

	}
};

/**
 * */
struct OpenSLOutFormat : public SLObjectFormat{
	// format
	int formatType;
	union{
		SLDataFormat_PCM dataFormat_PCM;	// stream
		SLDataFormat_MIME dataFormat_MIME;	// android_fd
	}dataFormat;

	// locator
	int locatorType;
	union{
		SLDataLocator_AndroidBufferQueue dataLocator_AndroidBufferQueue;
		SLDataLocator_AndroidSimpleBufferQueue dataLocator_AndroidSimpleBufferQueue;
		SLDataLocator_AndroidFD dataLocator_AndroidFD;
		// reserved
		SLDataLocator_URI dataLocator_URI;
		SLDataLocator_IODevice dataLocator_IODevice;
		SLDataLocator_BufferQueue dataLocator_BufferQueue;
	}dataLocator;

	// dst format
	int dstFormatType;
	union{
		SLDataFormat_PCM dataFormat_PCM;
	}dstDataFormat;

	// dst locator
	int dstLocatorType;
	union{
		SLDataLocator_OutputMix dataLocator_OutputMix;
		SLDataLocator_BufferQueue dataLocator_BufferQueue;
		SLDataLocator_AndroidBufferQueue dataLocator_AndroidBufferQueue;
		SLDataLocator_AndroidSimpleBufferQueue dataLocator_AndroidSimpleBufferQueue;
	}dstDataLocator;

	// control
	int bufferSize;
	int recordSize;
	int callBackWhen;
	int loopCount;

	// userdata
	void* context;

	/// default out format
	OpenSLOutFormat ():
			bufferSize(10240),
			recordSize(10240),
			callBackWhen(0),
			loopCount(1),
			context(0){
		// source format
		formatType = SL_DATAFORMAT_PCM;
		dataFormat.dataFormat_PCM.formatType = SL_DATAFORMAT_PCM;
		dataFormat.dataFormat_PCM.numChannels = 1;
		dataFormat.dataFormat_PCM.samplesPerSec = SL_SAMPLINGRATE_44_1;
		dataFormat.dataFormat_PCM.bitsPerSample = SL_PCMSAMPLEFORMAT_FIXED_16;
		dataFormat.dataFormat_PCM.containerSize = SL_PCMSAMPLEFORMAT_FIXED_16;
		dataFormat.dataFormat_PCM.bitsPerSample = SL_PCMSAMPLEFORMAT_FIXED_16;
		dataFormat.dataFormat_PCM.channelMask = SL_SPEAKER_FRONT_CENTER;
		dataFormat.dataFormat_PCM.endianness = SL_BYTEORDER_LITTLEENDIAN;

		// source locator
		locatorType = SL_DATALOCATOR_ANDROIDSIMPLEBUFFERQUEUE;
		dataLocator.dataLocator_AndroidSimpleBufferQueue.locatorType =
				SL_DATALOCATOR_ANDROIDSIMPLEBUFFERQUEUE;
		dataLocator.dataLocator_AndroidSimpleBufferQueue.numBuffers = 1;

		//dst format
		dstFormatType = 0;
		memset(&dstDataFormat, 0, sizeof(dstDataFormat));

		//dst locator
		dstLocatorType = SL_DATALOCATOR_OUTPUTMIX;
		dstDataLocator.dataLocator_OutputMix.locatorType = SL_DATALOCATOR_OUTPUTMIX;
		dstDataLocator.dataLocator_OutputMix.outputMix = NULL;// set by device
	}
};

/**
 * */
struct AssetContext{
	const char* path;
};

/**
 * stream config
 * @param streamType, according to the androidSound: RING, MUSIC, ...
 * */
struct OpenSLStreamConfig : public OpenSLOutFormat{
	int streamType;

	OpenSLStreamConfig()
	: streamType(0) {
		;
	}
};

/**
 * in format, used to record sound
 * */
struct OpenSLInFormat : public SLObjectFormat{
	// format
	int formatType;
	union{
		SLDataFormat_PCM dataFormat_PCM;	// stream
		SLDataFormat_MIME dataFormat_MIME;	// android_fd
	}dataFormat;

	// locator
	int locatorType;
	union{
		SLDataLocator_AndroidSimpleBufferQueue dataLocator_AndroidSimpleBufferQueue;
		SLDataLocator_AndroidFD dataLocator_AndroidFD;
	}dataLocator;

	// control
	int recordSize;

	OpenSLInFormat()
	: recordSize(8820) {
		formatType = SL_DATAFORMAT_PCM;
		dataFormat.dataFormat_PCM.formatType = SL_DATAFORMAT_PCM;
		dataFormat.dataFormat_PCM.numChannels = 1;
		dataFormat.dataFormat_PCM.samplesPerSec = SL_SAMPLINGRATE_44_1;
		dataFormat.dataFormat_PCM.bitsPerSample = SL_PCMSAMPLEFORMAT_FIXED_16;
		dataFormat.dataFormat_PCM.containerSize = SL_PCMSAMPLEFORMAT_FIXED_16;
		dataFormat.dataFormat_PCM.bitsPerSample = SL_PCMSAMPLEFORMAT_FIXED_16;
		dataFormat.dataFormat_PCM.channelMask = SL_SPEAKER_FRONT_CENTER;
		dataFormat.dataFormat_PCM.endianness = SL_BYTEORDER_LITTLEENDIAN;

		locatorType = SL_DATALOCATOR_ANDROIDSIMPLEBUFFERQUEUE;
		dataLocator.dataLocator_AndroidSimpleBufferQueue.locatorType =
				SL_DATALOCATOR_ANDROIDSIMPLEBUFFERQUEUE;
		dataLocator.dataLocator_AndroidSimpleBufferQueue.numBuffers = 1;
	}
};

/**
 * in state
 * */
enum OpenSLInState{
	SLIN_STOP,
	SLIN_PAUSE,
	SLIN_RECORD,
};

#endif
