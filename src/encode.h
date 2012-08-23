#ifndef ENCODE_H_
#define ENCODE_H_

#define WORD_BIT 32
#define WORD_BYTE 4


typedef struct {
  char *buf;
  char *pdu;
  int bufWords;
  int size;
  int pduWords;
  int bitsUsed;
} packedEncode;


packedEncode *initializeEncode(char *pdu, int size);
int finalizeEncode(packedEncode *memBuf);
char *pduEncode(packedEncode *memBuf);
void freeEncode(packedEncode *memBuf);
void encode(packedEncode *memBuf, unsigned long int n, int bitlength);
void dumpBuffer(char *bufName, char * buf, int amount);

#endif
