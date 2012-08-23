
#include <stdlib.h>
#include <stdio.h>
#include <arpa/inet.h>
#include <string.h>
#include <limits.h>

#include "encode.h"

#ifdef DEBUG_MODE
#define dbg(fmtstr, args...) \
  (printf(PROGNAME ":%s: " fmtstr "\n", __func__, ##args))
#else
#define dbg(dummy...)
#endif

unsigned mask32[] = {
  0,					
  ~(~0<<1),					
  ~(~0<<2),					
  ~(~0<<3),					
  ~(~0<<4),					
  ~(~0<<5),					
  ~(~0<<6),					
  ~(~0<<7),				
  ~(~0<<8),				
  ~(~0<<9),				
  ~(~0<<10),				
  ~(~0<<11),				
  ~(~0<<12),				
  ~(~0<<13),				
  ~(~0<<14),				
  ~(~0<<15),				
  ~(~0<<16),				
  ~(~0<<17),				
  ~(~0<<18),				
  ~(~0<<19),				
  ~(~0<<20),			
  ~(~0<<21),			
  ~(~0<<22),			
  ~(~0<<23),			
  ~(~0<<24),			
  ~(~0<<25),			
  ~(~0<<26),			
  ~(~0<<27),			
  ~(~0<<28),			
  ~(~0<<29),			
  ~(~0<<30),			
  ~(~0<<31),			
  ~0		
};


static void addBuf(packedEncode *memBuf, unsigned long int n);
static void addWord(packedEncode *memBuf, unsigned long int n);
static unsigned long int orBuf(packedEncode *memBuf);
static void resetBuf(packedEncode *memBuf);


void dumpBuffer(char *bufName, char *buf, int amount) {
  FILE *fp;
  fp = fopen(bufName, "w");
  fwrite(buf, 1, amount, fp);         
  fclose(fp);
}

packedEncode *initializeEncode(char *pdu, int size) {
  packedEncode *memBuf;
  
  if ((memBuf = (packedEncode *)malloc(sizeof(packedEncode))) == NULL) {
    dbg("replace this");
  }
  if ((memBuf->buf = calloc(WORD_BIT, WORD_BYTE)) == NULL) {
    dbg("replace this");
  }
  memBuf->pdu = pdu;
  memBuf->bufWords = 0;
  memBuf->size = size; /* in bytes */
  memBuf->bitsUsed = 0;
  memBuf->pduWords = 0;
  
  return memBuf;
}

char *pduEncode(packedEncode *memBuf) {
  return memBuf->pdu;   
}

int finalizeEncode(packedEncode *memBuf) {
  int totalbytes, bufused; 
  
  bufused = 0;
  
  /* add any leftover */
  if (memBuf->bitsUsed > 0) {
    unsigned long int word;
    int buffree;
    word = orBuf(memBuf);
    addWord(memBuf, word);
    buffree = (WORD_BIT - memBuf->bitsUsed) / CHAR_BIT;
    bufused = WORD_BYTE - buffree;
    memBuf->pduWords--;
  }
  totalbytes = (memBuf->pduWords * WORD_BYTE) + bufused;

  // reset some values
  memBuf->bufWords = 0;
  memBuf->bitsUsed = 0;
  memBuf->pduWords = 0;
  
  return totalbytes;
}

void freeEncode(packedEncode *memBuf) {
  //free(memBuf->pdu);  
  free(memBuf->buf);  
  free(memBuf);   
}


static void addBuf(packedEncode *memBuf, unsigned long int n) {
  unsigned long int word;
  int offset;
  
  /* make sure is big endian */
  word = htonl(n << (WORD_BIT - memBuf->bitsUsed));
  
  offset = ((memBuf->bufWords) * WORD_BYTE);
  memcpy((memBuf->buf)+offset, &word, WORD_BYTE);
  memBuf->bufWords++;
}

/* copy word across to the pdu */
static void addWord(packedEncode *memBuf, unsigned long int n) {
  int bytes;
  
  bytes = ((memBuf->pduWords) * WORD_BYTE);
  if (bytes+WORD_BYTE > memBuf->size) {
    dbg("replace this");
  }
  /* copy a word at pdu offset by bytes */
  memcpy((memBuf->pdu)+bytes, &n, WORD_BYTE);
  memBuf->pduWords++;
}

static unsigned long int orBuf(packedEncode *memBuf) {
  unsigned long int result;
  unsigned long int *n;
  int i;
  
  result = 0;
  
  for (i = 0 ; i < memBuf->bufWords ; i++) {
    n = (unsigned long int *)(memBuf->buf + (i * WORD_BYTE));
    result = (result | *n);
  }
  return result;
}

static void resetBuf(packedEncode *memBuf) {
  memset(memBuf->buf, 0, ((memBuf->bufWords) * WORD_BYTE));
  memBuf->bufWords = 0;
  memBuf->bitsUsed = 0;
}

void encode(packedEncode *memBuf, unsigned long int n, int bitlength) {
  //int bytes;
  unsigned long int word;
  
  //bytes = (memBuf->pduWords * WORD_BYTE);
    
  memBuf->bitsUsed = memBuf->bitsUsed + bitlength;
  
  if (memBuf->bitsUsed == WORD_BIT) { /* lucky fit */
    addBuf(memBuf, n);
    word = orBuf(memBuf);
    addWord(memBuf, word);
    resetBuf(memBuf);
  } else if (memBuf->bitsUsed < WORD_BIT) { /* some room */
    addBuf(memBuf, n);
  } else { /* exceeded space available, ie. memBuf->bitsUsed > WORD_BIT */
    int tail;
    tail = (memBuf->bitsUsed - WORD_BIT);		
    /* set the word to be full */
    memBuf->bitsUsed = WORD_BIT;
    /* remove the tail so we just add the head into the gap */
    addBuf(memBuf, (n >> tail));
    word = orBuf(memBuf);
    addWord(memBuf, word);
    resetBuf(memBuf);
    /* encode the leftover tail */
    memBuf->bitsUsed = tail;
    addBuf(memBuf, (n & mask32[tail]));
  }	
}

