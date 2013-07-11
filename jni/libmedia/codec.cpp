#include "codec.h"

uint32_t readBits (uint8_t * bytes, int *startTo, int len){
	uint32_t ret = 0;
	int idx = 0;
	int start = *startTo;
	while(idx < len){
		ret += ( ( ( (bytes[(start + idx)/8])  & ( 1 <<  (7 - (start + idx)%8 )) ) ? 1 : 0)  << idx );
		++idx;
	}
	*startTo =  start + len;
	return ret;
}

void serialFrom(AdtsHeader *adts, void *data){
	int startTo = 0;
	uint8_t *bytes = (uint8_t*)(data);
	adts->syncword = bytes[0] | (bytes[1] & 0xF0) << 4;
	adts->version = ( bytes[1] >> 3 & 0x01 );
	adts->layer = ( bytes[1] >> 1 & 0x03 );
	adts->protection = (bytes[1] & 0x01);

	adts->profile = (bytes[2] >> 6 & 0x03 );
	adts->sampleFrequency = (bytes[2] >> 2 & 0x0F);
	adts->privateStream = (bytes[2] >> 1 & 0x01);
	adts->channelConfig = (bytes[2] & 0x01) << 2 | (bytes[3] & (0x03 << 6)) >> 6;

	adts->originality = (bytes[3] >> 5 & 0x01);
	adts->home = (bytes[3] >> 4 & 0x01);
	adts->copyrighted = (bytes[3] >> 3 & 0x01);
	adts->copyrightStart = (bytes[3] >> 2 & 0x01);

	adts->frameLength = ((bytes[3] & 0x03) << 11) | (bytes[4] << 3) | (bytes[5] >> 5);

	adts->bufferFullness = ((bytes[5] & 0x1F) << 6) | ((bytes[6] >> 2) & 0x3F);
	adts->numOfFrames = (bytes[6] & 0x03);
	if(adts->protection == 0){
		adts->crc = bytes[7] << 8 | bytes[8];
	}
}


/**
 * serial to the data
 * */
void serialTo(AdtsHeader *adts, void *data){
	uint8_t *bytes = (uint8_t*)(data);
	bytes[0] = adts->syncword >> 4;
	bytes[1] = ((adts->syncword & 0x0F) << 4) | (adts->version << 3) | (adts->layer << 1) |
			(adts->protection);
	bytes[2] = (adts->profile << 6) | (adts->sampleFrequency << 2) | (adts->privateStream << 1) |
			((adts->channelConfig & 0x4) >> 2);
	bytes[3] = ((adts->channelConfig & 0x3) << 6) | (adts->originality << 5) |
			(adts->home << 4) | (adts->copyrighted << 3) | (adts->copyrightStart << 2) |
			(adts->frameLength >> 11);
	bytes[4] = adts->frameLength >> 3;
	bytes[5] = ((adts->frameLength & 0x7) << 5) | (adts->bufferFullness >> 6);
	bytes[6] = ((adts->bufferFullness & 0x3F) << 2) | (adts->numOfFrames);
	if(adts->protection == 0){
		bytes[7] = (adts->crc >> 8);
		bytes[8] = (adts->crc & 0x00FF);
	}
}

/**
 * do log adts, level : verbose, debug, info, warning, error, fatal(assert)
 * */
void logAdts(const AdtsHeader &adts, int level){
	__android_log_print(level, HUNCENT_TAG,
			"adts : syncword %d, version %d, layer %d, protection %d, profile %d, \n"
			"sampleFrequency %d, privateStream %d, channelConfig %d, \n"
			"originality %d, home %d, copyrighted %d, copyrightStart %d, \n"
			"frameLength %d, bufferFullness %d, numOfFrames %d",
			adts.syncword, adts.version, adts.layer, adts.protection, adts.profile,
			adts.sampleFrequency, adts.privateStream, adts.channelConfig,
			adts.originality, adts.home, adts.copyrighted, adts.copyrightStart,
			adts.frameLength, adts.bufferFullness, adts.numOfFrames);
}

/**
 * get channel mask from the channel count
 * */
uint32_t channelCountToMask(uint32_t channelCount)
{
#ifndef UNKNOWN_CHANNELMASK
#define UNKNOWN_CHANNELMASK 0
#endif
    // FIXME channel mask is not yet implemented by Stagefright, so use a reasonable default
    //       that is computed from the channel count
    uint32_t channelMask;
    switch (channelCount) {
    case 1:
        // see explanation in data.c re: default channel mask for mono
        return SL_SPEAKER_FRONT_LEFT;
    case 2:
        return SL_SPEAKER_FRONT_LEFT | SL_SPEAKER_FRONT_RIGHT;
    default:
        return UNKNOWN_CHANNELMASK;
    }
}
