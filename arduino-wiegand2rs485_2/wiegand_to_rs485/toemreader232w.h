//#include "toemreader232w.h"
#ifndef TOEMREADER232WH
#define TOEMREADER232WH

#include <Arduino.h>

class ToEMReader232w {
public:
	ToEMReader232w();
	uint8_t buildMessage(uint8_t* buf, uint8_t buflen, uint8_t cardtype, uint32_t cardrawdata);
	uint8_t buildMessage(uint8_t* buf, uint8_t buflen, uint8_t cardtype, uint16_t cardid, uint16_t cardcode);
};

extern ToEMReader232w* g_toemreader232w_p;
#endif
