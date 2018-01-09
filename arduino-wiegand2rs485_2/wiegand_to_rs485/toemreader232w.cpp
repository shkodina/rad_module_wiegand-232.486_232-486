#include "toemreader232w.h"
//=======================================================================================
#include <string.h>
#include <stdio.h>

//#include <Arduino.h>
//=======================================================================================
static ToEMReader232w g_toemreader232w;
ToEMReader232w* g_toemreader232w_p = &g_toemreader232w;
//=======================================================================================
#define PKT_START 0x3a
#define PKT_STOP1 0x0d
#define PKT_STOP2 0x0a
#define PKT_LEN 15
#define PKT_START_POS 0
#define PKT_DATA_POS 1
#define PKT_CHECKSUM_POS 11
#define PKT_STOP_POS 13

//=======================================================================================
ToEMReader232w::ToEMReader232w(){
};
//=======================================================================================
inline void prepareBuffer(uint8_t* buf){
	buf[PKT_START_POS] = PKT_START;
	buf[PKT_STOP_POS] = PKT_STOP1;
	buf[PKT_STOP_POS + 1] = PKT_STOP2;
};
//=======================================================================================
uint8_t calcCRC(uint8_t cardtype, uint32_t cardrawdata){
	uint8_t crc = 0;
	crc += cardtype;

	uint8_t* byte_p = (uint8_t*)&cardrawdata;

	crc+=*byte_p++;
	crc+=*byte_p++;
	crc+=*byte_p++;
	crc+=*byte_p++;
	
	return 255 - crc + 1;
}

//=======================================================================================
uint8_t calcCRC(uint8_t cardtype, uint16_t cardid, uint16_t cardcode){
	uint8_t crc = 0;
	crc += cardtype;

	uint8_t* byte_p = (uint8_t*)&cardid;

	crc+=*byte_p++;
	crc+=*byte_p++;
	
	byte_p = (uint8_t*)&cardcode;
	
	crc+=*byte_p++;
	crc+=*byte_p++;
	
	return 255 - crc + 1;
	
}

//=======================================================================================
uint8_t ToEMReader232w::buildMessage(uint8_t* buf, uint8_t buflen, uint8_t cardtype, uint32_t cardrawdata){
	if (buflen < PKT_LEN) return 0;
	sprintf(buf + PKT_DATA_POS, "%02X%08X%02X", cardtype, cardrawdata, calcCRC(cardtype, cardrawdata));
  prepareBuffer(buf);
	return PKT_LEN;
};
//=======================================================================================
uint8_t ToEMReader232w::buildMessage(uint8_t* buf, uint8_t buflen, uint8_t cardtype, uint16_t cardid, uint16_t cardcode){
	if (buflen < PKT_LEN) return 0;
	sprintf(buf + PKT_DATA_POS, "%02X%04X%04X%02X", cardtype, cardid, cardcode, calcCRC(cardtype, cardid, cardcode));
  prepareBuffer(buf);
	return PKT_LEN;
};
//=======================================================================================
