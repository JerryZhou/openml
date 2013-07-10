#include "stdpre.h"

#ifndef _CODEC_H_
#define _CODEC_H_

//---------------------------------------------------------------------------------
/**
 * http://wiki.multimedia.cx/index.php?title=ADTS
 * Structure :
 * AAAAAAAA AAAABCCD EEFFFFGH HHIJKLMM MMMMMMMM MMMOOOOO OOOOOOPP (QQQQQQQQ QQQQQQQQ)
 * Header consists of 7 or 9 bytes (without or with CRC).
 * Letter	 Length (bits)	 Description
 * A	 12	 syncword 0xFFF, all bits must be 1
 * B	 1	 MPEG Version: 0 for MPEG-4, 1 for MPEG-2
 * C	 2	 Layer: always 0
 * D	 1	 protection absent, Warning, set to 1 if there is no CRC and 0 if there is CRC
 * E	 2	 profile, the MPEG-4 Audio Object Type minus 1
 * F	 4	MPEG-4 Sampling Frequency Index (15 is forbidden)
 * G	 1	 private stream, set to 0 when encoding, ignore when decoding
 * H	 3	MPEG-4 Channel Configuration (in the case of 0, the channel configuration is sent via an inband PCE)
 * I	 1	 originality, set to 0 when encoding, ignore when decoding
 * J	 1	 home, set to 0 when encoding, ignore when decoding
 * K	 1	 copyrighted stream, set to 0 when encoding, ignore when decoding
 * L	 1	 copyright start, set to 0 when encoding, ignore when decoding
 * M	 13	 frame length, this value must include 7 or 9 bytes of header length: FrameLength = (ProtectionAbsent == 1 ? 7 : 9) + size(AACFrame)
 * O	 11	 Buffer fullness
 * P	 2	 Number of AAC frames (RDBs) in ADTS frame minus 1, for maximum compatibility always use 1 AAC frame per ADTS frame
 * Q	 16	 CRC if protection absent is 0
 *
 * sample frequency index:
 * 0x0                           96000
 * 0x1                           88200
 * 0x2                           64000
 * 0x3                           48000
 * 0x4                           44100
 * 0x5                           32000
 * 0x6                           24000
 * 0x7                           22050
 * 0x8                           16000
 * 0x9                           2000
 * 0xa                           11025
 * 0xb                           8000
 * */
// read atleast 9 byte,every time try read aac frame
#define adts_len_no_crc 9

typedef struct AdtsHeader{
	uint32_t syncword : 12;
	uint32_t version : 1;
	uint32_t layer : 2;
	uint32_t protection : 1;
	uint32_t profile : 2;
	uint32_t sampleFrequency : 4;
	uint32_t privateStream : 1;
	uint32_t channelConfig : 3;
	uint32_t originality : 1;
	uint32_t home : 1;
	uint32_t copyrighted : 1;
	uint32_t copyrightStart : 1;
	uint32_t frameLength : 13;
	uint32_t bufferFullness : 11;
	uint32_t numOfFrames : 2;
	uint32_t crc : 16;
}AdtsHeader;

/**
 * serial from the data
 * */
void serialFrom(AdtsHeader *adts, void *data);

/**
 * serial to the data
 * */
void serialTo(AdtsHeader *adts, void *data);

/**
 * do log adts, level : verbose, debug, info, warning, error, fatal(assert)
 * */
void logAdts(const AdtsHeader &adts, int level = ANDROID_LOG_ERROR);

/**
 * get channel mask from the channel count
 * */
uint32_t channelCountToMask(uint32_t channelCount);

#endif
