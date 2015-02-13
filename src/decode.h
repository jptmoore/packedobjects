#ifndef DECODE_H_
#define DECODE_H_

#define WORD_32BIT 32
#define WORD_BYTE 4

typedef struct {
  char *pdu;
  int ub;
  int word;
} packedDecode;


packedDecode *initializeDecode(char * pdu);
void freeDecode(packedDecode *memBuf);
unsigned long int decode(packedDecode *memBuf, int bitlen);

#endif
