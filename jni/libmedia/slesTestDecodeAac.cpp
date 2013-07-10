/*
 * Copyright (C) 2011 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/* AAC ADTS Decode Test

First run the program from shell:
  # slesTestDecodeAac /sdcard/myFile.adts

Expected output:
  OpenSL ES test slesTestDecodeAac: decodes a file containing AAC ADTS data
  Player created
  Player realized
  Enqueueing initial empty buffers to receive decoded PCM data 0 1
  Enqueueing initial full buffers of encoded ADTS data 0 1
  Starting to decode
  Frame counters: encoded=4579 decoded=4579

These use adb on host to retrieve the decoded file:
  % adb pull /sdcard/myFile.adts.raw myFile.raw

How to examine the output with Audacity:
 Project / Import raw data
 Select myFile.raw file, then click Open button
 Choose these options:
  Signed 16-bit PCM
  Little-endian
  1 Channel (Mono) / 2 Channels (Stereo) based on the PCM information obtained when decoding
  Sample rate based on the PCM information obtained when decoding
 Click Import button

*/

// #define QUERY_METADATA

#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/time.h>
#include <fcntl.h>
#include <pthread.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

#include <SLES/OpenSLES.h>
#include <SLES/OpenSLES_Android.h>

#include "./opensldevice.h"
#include "./codec.h"

/* Explicitly requesting SL_IID_ANDROIDBUFFERQUEUE and SL_IID_ANDROIDSIMPLEBUFFERQUEUE
 * on the AudioPlayer object for decoding, and
 * SL_IID_METADATAEXTRACTION for retrieving the format of the decoded audio.
 */
#define NUM_EXPLICIT_INTERFACES_FOR_PLAYER 4

/* Number of decoded samples produced by one AAC frame; defined by the standard */
#define SAMPLES_PER_AAC_FRAME 1024
/* Size of the encoded AAC ADTS buffer queue */
#define NB_BUFFERS_IN_ADTS_QUEUE 1 // 2 to 4 is typical

/* Size of the decoded PCM buffer queue */
#define NB_BUFFERS_IN_PCM_QUEUE 1  // 2 to 4 is typical
/* Size of each PCM buffer in the queue */
#define BUFFER_SIZE_IN_BYTES   (2*sizeof(short)*SAMPLES_PER_AAC_FRAME)

/* Local storage for decoded PCM audio data */
int8_t pcmData[NB_BUFFERS_IN_PCM_QUEUE * BUFFER_SIZE_IN_BYTES];

/* destination for decoded data */
static FILE* outputFp;

/* to signal to the test app that the end of the encoded ADTS stream has been reached */
bool eos = false;
bool endOfEncodedStream = false;

void *ptr;
unsigned char *frame;
size_t filelen;
size_t encodedFrames = 0;
size_t encodedSamples = 0;
size_t decodedFrames = 0;
size_t decodedSamples = 0;
size_t totalEncodeCompletions = 0;     // number of Enqueue completions received
size_t pauseFrame = 0;              // pause after this many decoded frames, zero means don't pause
SLboolean createRaw = SL_BOOLEAN_TRUE; // whether to create a .raw file containing PCM data

/* constant to identify a buffer context which is the end of the stream to decode */
static const int kEosBufferCntxt = 1980; // a magic value we can compare against

/* protects shared variables */
pthread_mutex_t eosLock = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t eosCondition = PTHREAD_COND_INITIALIZER;

// These are extensions to OpenMAX AL 1.0.1 values

#define PREFETCHSTATUS_UNKNOWN ((SLuint32) 0)
#define PREFETCHSTATUS_ERROR   ((SLuint32) (-1))

// Mutex and condition shared with main program to protect prefetch_status

static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t cond = PTHREAD_COND_INITIALIZER;
SLuint32 prefetch_status = PREFETCHSTATUS_UNKNOWN;

/* used to detect errors likely to have occured when the OpenSL ES framework fails to open
 * a resource, for instance because a file URI is invalid, or an HTTP server doesn't respond.
 */
#define PREFETCHEVENT_ERROR_CANDIDATE \
        (SL_PREFETCHEVENT_STATUSCHANGE | SL_PREFETCHEVENT_FILLLEVELCHANGE)

//-----------------------------------------------------------------
/* Exits the application if an error is encountered */
#define ExitOnError(x) ExitOnErrorFunc(x,__LINE__)

void ExitOnErrorFunc( SLresult result , int line)
{
    if (SL_RESULT_SUCCESS != result) {
        fprintf(stderr, "Error code %u encountered at line %d, exiting\n", result, line);
        exit(EXIT_FAILURE);
    }
}

//-----------------------------------------------------------------
/* Structure for passing information to callback function */
typedef struct CallbackCntxt_ {
    SLPlayItf playItf;
    SLint8*   pDataBase;    // Base address of local audio data storage
    SLint8*   pData;        // Current address of local audio data storage
} CallbackCntxt;

// used to notify when SL_PLAYEVENT_HEADATEND event is received
static pthread_mutex_t head_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t head_cond = PTHREAD_COND_INITIALIZER;
static SLboolean head_atend = SL_BOOLEAN_FALSE;

//-----------------------------------------------------------------
/* Callback for AndroidBufferQueueItf through which we supply ADTS buffers */
SLresult AndroidBufferQueueCallback(
        SLAndroidBufferQueueItf caller,
        void *pCallbackContext,        /* input */
        void *pBufferContext,          /* input */
        void *pBufferData,             /* input */
        SLuint32 dataSize,             /* input */
        SLuint32 dataUsed,             /* input */
        const SLAndroidBufferItem *pItems,/* input */
        SLuint32 itemsLength           /* input */)
{
    // mutex on all global variables
    pthread_mutex_lock(&eosLock);
    SLresult res;

    // for demonstration purposes:
    // verify what type of information was enclosed in the processed buffer
    if (NULL != pBufferContext) {
        if (&kEosBufferCntxt == pBufferContext) {
            fprintf(stdout, "EOS was processed\n");
        }
    }

    ++totalEncodeCompletions;
    if (endOfEncodedStream) {
        // we continue to receive acknowledgement after each buffer was processed
        if (pBufferContext == (void *) &kEosBufferCntxt) {
            LOGE("Received EOS completion after EOS\n");
        } else if (pBufferContext == NULL) {
            LOGE("Received ADTS completion after EOS\n");
        } else {
            fprintf(stderr, "Received acknowledgement after EOS with unexpected context %p\n",
                    pBufferContext);
        }
    } else if (filelen == 0 || totalEncodeCompletions == 5) {
        // signal EOS to the decoder rather than just starving it
        LOGE("Enqueue EOS: encoded frames=%u, decoded frames=%u\n", encodedFrames, decodedFrames);
        LOGE("You should now see %u ADTS completion%s followed by 1 EOS completion\n",
                NB_BUFFERS_IN_ADTS_QUEUE - 1, NB_BUFFERS_IN_ADTS_QUEUE != 2 ? "s" : "");
        SLAndroidBufferItem msgEos;
        msgEos.itemKey = SL_ANDROID_ITEMKEY_EOS;
        msgEos.itemSize = 0;
        // EOS message has no parameters, so the total size of the message is the size of the key
        //   plus the size of itemSize, both SLuint32
        res = (*caller)->Enqueue(caller, (void *)&kEosBufferCntxt /*pBufferContext*/,
                NULL /*pData*/, 0 /*dataLength*/,
                &msgEos /*pMsg*/,
                sizeof(SLuint32)*2 /*msgLength*/);
        ExitOnError(res);
        endOfEncodedStream = true;
    // verify that we are at start of an ADTS frame
    } else if (!(filelen < 7 || frame[0] != 0xFF || (frame[1] & 0xF0) != 0xF0)) {
        if (pBufferContext != NULL) {
            fprintf(stderr, "Received acknowledgement before EOS with unexpected context %p\n",
                    pBufferContext);
        }
        AdtsHeader adts = {0};
		serialFrom(&adts, frame);
		logAdts(adts);

        unsigned framelen = ((frame[3] & 3) << 11) | (frame[4] << 3) | (frame[5] >> 5);
        if (framelen <= filelen) {
            // push more data to the queue
            res = (*caller)->Enqueue(caller, NULL /*pBufferContext*/,
                    frame, framelen, NULL, 0);
            ExitOnError(res);
            frame += framelen;
            filelen -= framelen;
            ++encodedFrames;
            encodedSamples += SAMPLES_PER_AAC_FRAME;
        } else {
            fprintf(stderr,
                    "partial ADTS frame at EOF discarded; offset=%u, framelen=%u, filelen=%u\n",
                    frame - (unsigned char *) ptr, framelen, filelen);
            frame += filelen;
            filelen = 0;
        }
    } else {
        fprintf(stderr, "corrupt ADTS frame encountered; offset=%u, filelen=%u\n",
                frame - (unsigned char *) ptr, filelen);
        frame += filelen;
        filelen = 0;
    }
    pthread_mutex_unlock(&eosLock);

    return SL_RESULT_SUCCESS;
}

//-----------------------------------------------------------------
/* Callback for decoding buffer queue events */
void DecPlayCallback(
        SLAndroidSimpleBufferQueueItf queueItf,
        void *pContext)
{
    // mutex on all global variables
    pthread_mutex_lock(&eosLock);

    CallbackCntxt *pCntxt = (CallbackCntxt*)pContext;

    /* Save the decoded data to output file */
    if (outputFp != NULL && fwrite(pCntxt->pData, 1, BUFFER_SIZE_IN_BYTES, outputFp)
                < BUFFER_SIZE_IN_BYTES) {
        fprintf(stderr, "Error writing to output file");
    }

    /* Re-enqueue the now empty buffer */
    SLresult res;
    res = (*queueItf)->Enqueue(queueItf, pCntxt->pData, BUFFER_SIZE_IN_BYTES);
    ExitOnError(res);

    /* Increase data pointer by buffer size, with circular wraparound */
    pCntxt->pData += BUFFER_SIZE_IN_BYTES;
    if (pCntxt->pData >= pCntxt->pDataBase + (NB_BUFFERS_IN_PCM_QUEUE * BUFFER_SIZE_IN_BYTES)) {
        pCntxt->pData = pCntxt->pDataBase;
    }

    // Note: adding a sleep here or any sync point is a way to slow down the decoding, or
    //  synchronize it with some other event, as the OpenSL ES framework will block until the
    //  buffer queue callback return to proceed with the decoding.

    ++decodedFrames;
    decodedSamples += SAMPLES_PER_AAC_FRAME;

    if (endOfEncodedStream && decodedSamples >= encodedSamples) {
        eos = true;
        pthread_cond_signal(&eosCondition);
    }
    pthread_mutex_unlock(&eosLock);
}

//-----------------------------------------------------------------

/* Decode an audio path by opening a file descriptor on that path  */
void TestDecToBuffQueue( SLObjectItf sl, const char *path, int fd)
{
    // check what kind of object it is
    int ok;
    struct stat statbuf;
    ok = fstat(fd, &statbuf);
    if (ok < 0) {
        perror(path);
        return;
    }

    // verify that's it is a file
    if (!S_ISREG(statbuf.st_mode)) {
        fprintf(stderr, "%s: not an ordinary file\n", path);
        return;
    }

    // map file contents into memory to make it easier to access the ADTS frames directly
    ptr = mmap(NULL, statbuf.st_size, PROT_READ, MAP_FILE | MAP_PRIVATE, fd, (off_t) 0);
    if (ptr == MAP_FAILED) {
        perror(path);
        return;
    }
    frame = (unsigned char *) ptr;
    filelen = statbuf.st_size;

    // create PCM .raw file
    if (createRaw) {
        size_t len = strlen((const char *) path);
        char* outputPath = (char*) malloc(len + 4 + 1); // save room to concatenate ".raw"
        if (NULL == outputPath) {
            ExitOnError(SL_RESULT_RESOURCE_ERROR);
        }
        memcpy(outputPath, path, len + 1);
        strcat(outputPath, ".raw");
        outputFp = fopen(outputPath, "w");
        if (NULL == outputFp) {
            // issue an error message, but continue the decoding anyway
            perror(outputPath);
        }
    } else {
        outputFp = NULL;
    }

    SLresult res;
    SLEngineItf EngineItf;

    /* Objects this application uses: one audio player */
    SLObjectItf  player;

    /* Interfaces for the audio player */
    SLPlayItf                     playItf;
    /*   to retrieve the PCM samples */
    SLAndroidSimpleBufferQueueItf decBuffQueueItf;
    /*   to queue the AAC data to decode */
    SLAndroidBufferQueueItf       aacBuffQueueItf;
    /*   for prefetch status */
    // SLPrefetchStatusItf           prefetchItf;

    SLboolean required[NUM_EXPLICIT_INTERFACES_FOR_PLAYER];
    SLInterfaceID iidArray[NUM_EXPLICIT_INTERFACES_FOR_PLAYER];

    /* Get the SL Engine Interface which is implicit */
    res = (*sl)->GetInterface(sl, SL_IID_ENGINE, (void*)&EngineItf);
    ExitOnError(res);

    /* Initialize arrays required[] and iidArray[] */
    unsigned int i;
    for (i=0 ; i < NUM_EXPLICIT_INTERFACES_FOR_PLAYER ; i++) {
        required[i] = SL_BOOLEAN_FALSE;
        iidArray[i] = SL_IID_NULL;
    }

    /* ------------------------------------------------------ */
    /* Configuration of the player  */
#define NUT_TO 0
#define NUT_PREF 1
    /* Request the AndroidSimpleBufferQueue interface */
    required[0] = SL_BOOLEAN_TRUE;
    iidArray[0] = SL_IID_ANDROIDSIMPLEBUFFERQUEUE;
    /* Request the AndroidBufferQueue interface */
    required[1 - NUT_TO] = SL_BOOLEAN_TRUE;
    iidArray[1 - NUT_TO] = SL_IID_ANDROIDBUFFERQUEUESOURCE;

    /* Setup the data source for queueing AAC buffers of ADTS data */
    SLDataLocator_AndroidBufferQueue loc_srcAbq = {
            SL_DATALOCATOR_ANDROIDBUFFERQUEUE /*locatorType*/,
            NB_BUFFERS_IN_ADTS_QUEUE          /*numBuffers*/};
    SLDataFormat_MIME format_srcMime = {
            SL_DATAFORMAT_MIME         /*formatType*/,
            SL_ANDROID_MIME_AACADTS    /*mimeType*/,
            SL_CONTAINERTYPE_RAW       /*containerType*/};
    SLDataSource decSource = {&loc_srcAbq /*pLocator*/, &format_srcMime /*pFormat*/};

    /* Setup the data sink, a buffer queue for buffers of PCM data */
    SLDataLocator_AndroidSimpleBufferQueue loc_destBq = {
            SL_DATALOCATOR_ANDROIDSIMPLEBUFFERQUEUE/*locatorType*/,
            NB_BUFFERS_IN_PCM_QUEUE                /*numBuffers*/ };

    /*    declare we're decoding to PCM, the parameters after that need to be valid,
          but are ignored, the decoded format will match the source */
    SLDataFormat_PCM format_destPcm = { /*formatType*/ SL_DATAFORMAT_PCM, /*numChannels*/ 1,
            /*samplesPerSec*/ SL_SAMPLINGRATE_8, /*pcm.bitsPerSample*/ SL_PCMSAMPLEFORMAT_FIXED_16,
            /*/containerSize*/ 16, /*channelMask*/ SL_SPEAKER_FRONT_LEFT,
            /*endianness*/ SL_BYTEORDER_LITTLEENDIAN };
    SLDataSink decDest = {&loc_destBq /*pLocator*/, &format_destPcm /*pFormat*/};
   // SLDataSink decDest;
//    SLDataLocator_OutputMix loc_destMix;
//    loc_destMix.locatorType = SL_DATALOCATOR_OUTPUTMIX;
//    loc_destMix.outputMix = slDevice()->outputMixObj();
//    decDest.pLocator = &loc_destMix;
//    decDest.pFormat = NULL;
    LOGE("TestDecode 1");

    /* Create the audio player */
    res = (*EngineItf)->CreateAudioPlayer(EngineItf, &player, &decSource, &decDest,
#ifdef QUERY_METADATA
            NUM_EXPLICIT_INTERFACES_FOR_PLAYER - NUT_TO - NUT_PREF,
#else
            NUM_EXPLICIT_INTERFACES_FOR_PLAYER - 1 - NUT_TO - NUT_PREF,
#endif
            iidArray, required);
    LOGE("Player created\n");

    /* Realize the player in synchronous mode. */
    res = (*player)->Realize(player, SL_BOOLEAN_FALSE);
    ExitOnError(res);
    LOGE("Player realized\n");

    /* Get the play interface which is implicit */
    res = (*player)->GetInterface(player, SL_IID_PLAY, (void*)&playItf);
    ExitOnError(res);

    /* Use the play interface to set up a callback for the SL_PLAYEVENT_HEAD* events */
    res = (*playItf)->SetCallbackEventsMask(playItf,
    		SL_PLAYEVENT_HEADATEND);

#if !(NUT_TO)
    /* Get the buffer queue interface which was explicitly requested */
    res = (*player)->GetInterface(player, SL_IID_ANDROIDSIMPLEBUFFERQUEUE, (void*)&decBuffQueueItf);
    ExitOnError(res);
#endif

    /* Get the Android buffer queue interface which was explicitly requested */
    res = (*player)->GetInterface(player, SL_IID_ANDROIDBUFFERQUEUESOURCE, (void*)&aacBuffQueueItf);
    ExitOnError(res);

    /* ------------------------------------------------------ */
    /* Initialize the callback and its context for the buffer queue of the decoded PCM */
#if !(NUT_TO)
    CallbackCntxt sinkCntxt;
    sinkCntxt.playItf = playItf;
    sinkCntxt.pDataBase = (int8_t*)&pcmData;
    sinkCntxt.pData = sinkCntxt.pDataBase;
    res = (*decBuffQueueItf)->RegisterCallback(decBuffQueueItf, DecPlayCallback, &sinkCntxt);
    ExitOnError(res);

    /* Enqueue buffers to map the region of memory allocated to store the decoded data */
    LOGE("Enqueueing initial empty buffers to receive decoded PCM data");
    for(i = 0 ; i < NB_BUFFERS_IN_PCM_QUEUE ; i++) {
        LOGE(" %d", i);
        res = (*decBuffQueueItf)->Enqueue(decBuffQueueItf, sinkCntxt.pData, BUFFER_SIZE_IN_BYTES);
        ExitOnError(res);
        sinkCntxt.pData += BUFFER_SIZE_IN_BYTES;
        if (sinkCntxt.pData >= sinkCntxt.pDataBase +
                (NB_BUFFERS_IN_PCM_QUEUE * BUFFER_SIZE_IN_BYTES)) {
            sinkCntxt.pData = sinkCntxt.pDataBase;
        }
    }
    LOGE("\n");
#endif

    /* Initialize the callback for the Android buffer queue of the encoded data */
    res = (*aacBuffQueueItf)->RegisterCallback(aacBuffQueueItf, AndroidBufferQueueCallback, NULL);
    ExitOnError(res);

    /* Enqueue the content of our encoded data before starting to play,
       we don't want to starve the player initially */
    LOGE("Enqueueing initial full buffers of encoded ADTS data");
    for (i=0 ; i < NB_BUFFERS_IN_ADTS_QUEUE ; i++) {
        if (filelen < 7 || frame[0] != 0xFF || (frame[1] & 0xF0) != 0xF0) {
            LOGE("\ncorrupt ADTS frame encountered; offset %zu bytes\n",
                    frame - (unsigned char *) ptr);
            // Note that prefetch will detect this error soon when it gets a premature EOF
            break;
        }
        AdtsHeader adts = {0};
		serialFrom(&adts, frame);
		logAdts(adts);

        unsigned framelen = ((frame[3] & 3) << 11) | (frame[4] << 3) | (frame[5] >> 5);
        LOGE(" %d (%u bytes)", i, framelen);
        res = (*aacBuffQueueItf)->Enqueue(aacBuffQueueItf, NULL /*pBufferContext*/,
                frame, framelen, NULL, 0);
        ExitOnError(res);
        frame += framelen;
        filelen -= framelen;
        ++encodedFrames;
        encodedSamples += SAMPLES_PER_AAC_FRAME;
    }
    LOGE("\n");

    /* ------------------------------------------------------ */
    /* Start decoding */
    LOGE("Starting to decode\n");
    res = (*playItf)->SetPlayState(playItf, SL_PLAYSTATE_PLAYING);
    ExitOnError(res);

    /* Decode until the end of the stream is reached */
    LOGE("Awaiting notification that all encoded buffers have been enqueued\n");
    pthread_mutex_lock(&eosLock);
    while (!eos) {
        if (pauseFrame > 0) {
            if (decodedFrames >= pauseFrame) {
                pauseFrame = 0;
                LOGE("Pausing after decoded frame %u for 10 seconds\n", decodedFrames);
                pthread_mutex_unlock(&eosLock);
                res = (*playItf)->SetPlayState(playItf, SL_PLAYSTATE_PAUSED);
                ExitOnError(res);
                sleep(10);
                LOGE("Resuming\n");
                res = (*playItf)->SetPlayState(playItf, SL_PLAYSTATE_PLAYING);
                ExitOnError(res);
                pthread_mutex_lock(&eosLock);
            } else {
                pthread_mutex_unlock(&eosLock);
                usleep(10*1000);
                pthread_mutex_lock(&eosLock);
            }
        } else {
            pthread_cond_wait(&eosCondition, &eosLock);
        }
    }
    pthread_mutex_unlock(&eosLock);
    LOGE("All encoded buffers have now been enqueued, but there's still more to do\n");

    LOGE("Decode is now finished\n");

    pthread_mutex_lock(&eosLock);
    LOGE("Frame counters: encoded=%u decoded=%u\n", encodedFrames, decodedFrames);
    LOGE("Sample counters: encoded=%u decoded=%u\n", encodedSamples, decodedSamples);
    LOGE("Total encode completions received: actual=%u, expected=%u\n",
            totalEncodeCompletions, encodedFrames+1/*EOS*/);
    pthread_mutex_unlock(&eosLock);

    LOGE("Frame length statistics:\n");

    /* ------------------------------------------------------ */
    /* End of decoding */

destroyRes:
    /* Destroy the AudioPlayer object */
    (*player)->Destroy(player);

    if (outputFp != NULL) {
        fclose(outputFp);
    }

    // unmap the ADTS AAC file from memory
    ok = munmap(ptr, statbuf.st_size);
    if (0 != ok) {
        perror(path);
    }
}

//-----------------------------------------------------------------
int playAACDecode()
{
    SLresult    res;
    SLObjectItf sl;

    // open pathname of encoded ADTS AAC file to get a file descriptor
    int fd;
    fd = open("sdcard/example.aac", O_RDONLY);
    if (fd < 0) {
    	LOGE("failed to open aac_pcm.pcm");
        return EXIT_FAILURE;
    }

    sl = slDevice()->engineObject();

    TestDecToBuffQueue(sl, "sdcard/example.aac", fd);

    // close the file
    (void) close(fd);

    return EXIT_SUCCESS;
}
