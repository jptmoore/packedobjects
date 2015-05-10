#ifndef PACKEDOBJECTS_DECODE_H_
#define PACKEDOBJECTS_DECODE_H_

#include "packedobjects.h"

// main api function
xmlDocPtr packedobjects_decode(packedobjectsContext *pc, char *pdu);

// convenience function
unsigned char *packedobjects_decode_to_string(packedobjectsContext *pc, char *pdu);

#endif
