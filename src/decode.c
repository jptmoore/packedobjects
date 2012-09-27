
#include <stdio.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <string.h>

#include "config.h"
#include "decode.h"

#ifdef DEBUG_MODE
#define dbg(fmtstr, args...) \
  (printf(PROGNAME ":%s: " fmtstr "\n", __func__, ##args))
#else
#define dbg(dummy...)
#endif

#ifdef QUIET_MODE
#define alert(dummy...)
#else
#define alert(fmtstr, args...) \
  (printf(PROGNAME ":%s: " fmtstr "\n", __func__, ##args))
#endif

/* defined in encode.c */
extern unsigned mask32[];

static unsigned long int getn(packedDecode *memBuf, int bitlen, int lb);


packedDecode *initializeDecode(char * pdu) {
  packedDecode *memBuf;
  
  if ((memBuf = (packedDecode *)malloc(sizeof(packedDecode))) == NULL) {
    alert("Failed to allocate memory during initialisation.");
    return NULL;
  }
  memBuf->ub = WORD_BIT;
  memBuf->word = 0;
  memBuf->pdu = pdu;
  return memBuf;
}

void freeDecode(packedDecode *memBuf) {
  //free(memBuf->pdu);
  free(memBuf);
}


static unsigned long int getn(packedDecode *memBuf, int bitlen, int lb) {
  unsigned long int n;
  int offset;
  
  offset = ((memBuf->word) * WORD_BYTE);
  memcpy(&n, (memBuf->pdu)+offset, WORD_BYTE);
  /* get it back to lil endian */
  n = ntohl(n);
  n = n >> lb;
  return (n & mask32[bitlen]);
}

unsigned long int decode(packedDecode *memBuf, int bitlen) {
  unsigned long int n;
  int lb;
  
  lb = memBuf->ub - bitlen;
  
  if (lb > 0) {
    /* our value exists within the current word...kewl! */
    n = getn(memBuf, bitlen, lb);
    memBuf->ub = lb;
  } else if (lb == 0) {
    /* once we get this we hit the boundary so incr word count */
    n = getn(memBuf, bitlen, lb);
    memBuf->ub = WORD_BIT;
    memBuf->word++;
  } else {
    /* our value crossed a boundary lb < 0 */
    unsigned long int n1, n2;
    int lhs, rhs;
    lhs = lb + bitlen;
    n1 = getn(memBuf, lhs, 0);
    rhs = bitlen - lhs;
    memBuf->ub = WORD_BIT;
    memBuf->word++;
    lb = WORD_BIT - rhs;
    n2 = getn(memBuf, rhs, lb);
    n = ((n1 << rhs) | n2);
    memBuf->ub = lb;
  }
  return n;
}
