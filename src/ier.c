
#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <setjmp.h>
#include <math.h>
#include <time.h>

#include "packedobjects.h"
#include "config.h"
#include "ier.h"

#define WORD_BIT 32
#define WORD_BYTE 4
#define EIGHT_BIT 8
#define SEVEN_BIT 7
#define FOUR_BIT 4
#define ONE_BIT 1

#define SIXTEEN_K (16*1024)


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

// defined in packedobject.c
extern jmp_buf encode_exception_env;
extern jmp_buf decode_exception_env;

static char hexchar[] = {
  '0',
  '1',
  '2',
  '3',
  '4',
  '5',
  '6',
  '7',
  '8',
  '9',
  'a',
  'b',
  'c',
  'd',
  'e',
  'f'
};

static char numchar[] = {
  '0',
  '1',
  '2',
  '3',
  '4',
  '5',
  '6',
  '7',
  '8',
  '9',
  '.',
  '+',
  '-'
};

	

static int bitcount (unsigned int n);
static int bits_required(unsigned int lb, unsigned int ub);
static time_t rfc3339string_to_epoch(const char *timestring);
static char *epoch_to_rfc3339string(char *buf, int size, time_t t);

/* can not calculate for value 0 */
static int bitcount (unsigned int n)
{
  int count=0;
  while (n) {
    count++;
    n >>= 1 ;
  }
  return count ;
}

static int bits_required(unsigned int lb, unsigned int ub)
{
  unsigned int range = ub - lb;

  if (range == 0) {
    return 1;
  } else {
    return (bitcount(range));
  }
  

}


void encodeBoolean(packedEncode *memBuf, int flag) {
  encode(memBuf, flag, 1);
}

int decodeBoolean(packedDecode *memBuf) {
  return (decode(memBuf, 1));
}


void encodeConstrainedBitString(packedEncode *memBuf, char *s, int lb, int ub) {
  int bits, len, i;
  char c;
  
  bits = bits_required(lb, ub);
  
  // in case s is null
  (s) ? (len=strlen(s)) : (len=0);
  
  /* encode length as constrained whole number */
  encode(memBuf, len - lb, bits);
  
  for (i = 0; i < len; i++) {
    c = *(s+i);
    encode(memBuf, c - '0', ONE_BIT);
  }
  
}

char *decodeConstrainedBitString(packedDecode *memBuf, int lb, int ub) {
  int len, i;
  char *s, *baseptr;
  
  /* get length as constrained whole number */
  len = decode(memBuf, bits_required(lb, ub)) + lb;
    
  // add room for string plus null terminator
  if ((s = (char *)malloc(len + 1)) == NULL) {
    alert("Failed to allocate memory for string.");
    return NULL;
  }  
  baseptr = s;
  
  for (i = 0; i < len; i++) {
    *s = decode(memBuf, ONE_BIT) + '0';
    s++;
  }
  *s = '\0';
  
  s = baseptr;
  
  return s;
  
}

void encodeFixedLengthBitString(packedEncode *memBuf, char *s, int len) {
  int i;
  char c;
  
  for (i = 0; i < len; i++) {
    c = *(s+i);
    encode(memBuf, c - '0', ONE_BIT);
  }
}

char *decodeFixedLengthBitString(packedDecode *memBuf, int len) {
  int i;
  char *s, *baseptr;
    
  // add room for string plus null terminator
  if ((s = (char *)malloc(len + 1)) == NULL) {
    alert("Failed to allocate memory for string.");
    return NULL;
  }   
  baseptr = s;
  
  for (i = 0; i < len; i++) {
    *s = decode(memBuf, ONE_BIT) + '0';
    s++;
  }
  *s = '\0';
  
  s = baseptr;
  
  return s;
}

void encodeSemiConstrainedBitString(packedEncode *memBuf, char *s) {
  int len, i;
  char c;
  
  // in case s is null
  (s) ? (len=strlen(s)) : (len=0);
  
  // encode the length as semi constrained integer with 0 lb
  encodeUnsignedSemiConstrainedInteger(memBuf, len, 0);
  
  for (i = 0; i < len; i++) {
    c = *(s+i);
    encode(memBuf, c - '0', ONE_BIT); 
  }
}

char *decodeSemiConstrainedBitString(packedDecode *memBuf) {
  int len, i;
  char *s, *baseptr;
  
  len = decodeUnsignedSemiConstrainedInteger(memBuf, 0);

  // add room for string plus null terminator
  if ((s = (char *)malloc(len + 1)) == NULL) {
    alert("Failed to allocate memory for string.");
    return NULL;      
  }	
  baseptr = s;	
  
  for (i = 0; i < len; i++) {
    *s = decode(memBuf, ONE_BIT) + '0';
    s++;
  }	
  *s = '\0';
  
  s = baseptr;
   
  return s;
}



void encodeFixedLengthHexString(packedEncode *memBuf, char *s, int len) {
  int i;
  char c;
  
  for (i = 0; i < len; i++) {
    c = *(s+i);
    if (c >= '0' && c <= '9')
      encode(memBuf, c - '0', FOUR_BIT);
    else if (c >= 'A' && c <= 'Z')
      encode(memBuf, c - 'A' + 10, FOUR_BIT);
    else if (c >= 'a' && c <= 'z')
      encode(memBuf, c - 'a' + 10, FOUR_BIT);
    else {
      alert("Unsupported character.\n");
    }
  }
}

char *decodeFixedLengthHexString(packedDecode *memBuf, int len) {
  int i;
  char *s, *baseptr;
  unsigned int n;
  
  // add room for string plus null terminator
  if ((s = (char *)malloc(len + 1)) == NULL) {
    alert("Failed to allocate memory for string.");
    return NULL;       
  }   
  baseptr = s;
  
  for (i = 0; i < len; i++) {
    n = decode(memBuf, FOUR_BIT);
    *s = hexchar[n];
    s++;
  }
  *s = '\0';
  
  s = baseptr;
  
  return s;
}


void encodeConstrainedHexString(packedEncode *memBuf, char *s, int lb, int ub) {
  int bits, len, i;
  char c;
  
  bits = bits_required(lb, ub);
  
  // in case s is null
  (s) ? (len=strlen(s)) : (len=0);
  
  /* encode length as constrained whole number */
  encode(memBuf, len - lb, bits);
  
  for (i = 0; i < len; i++) {
    c = *(s+i);
    if (c >= '0' && c <= '9')
      encode(memBuf, c - '0', FOUR_BIT);
    else if (c >= 'A' && c <= 'Z')
      encode(memBuf, c - 'A' + 10, FOUR_BIT);
    else if (c >= 'a' && c <= 'z')
      encode(memBuf, c - 'a' + 10, FOUR_BIT);
    else {
      alert("Unsupported character");
    }
  }
}

char *decodeConstrainedHexString(packedDecode *memBuf, int lb, int ub) {
  int len, i;
  char *s, *baseptr;
  unsigned int n;
  
  /* get length as constrained whole number */
  len = decode(memBuf, bits_required(lb, ub)) + lb;
   
  // add room for string plus null terminator
  if ((s = (char *)malloc(len + 1)) == NULL) {
    alert("Failed to allocate memory for string.");
    return NULL;   
  }  
  baseptr = s;
  
  for (i = 0; i < len; i++) {
    n = decode(memBuf, FOUR_BIT);
    *s = hexchar[n];
    s++;
  }
  *s = '\0';
  
  s = baseptr;
    
  return s;
}

void encodeSemiConstrainedHexString(packedEncode *memBuf, char *s) {
  int len, i;
  char c;

  // in case s is null
  (s) ? (len=strlen(s)) : (len=0);
  
  // encode the length as semi constrained integer with 0 lb
  encodeUnsignedSemiConstrainedInteger(memBuf, len, 0);
  
  for (i = 0; i < len; i++) {
    c = *(s+i);
    if (c >= '0' && c <= '9')
      encode(memBuf, c - '0', FOUR_BIT);
    else if (c >= 'A' && c <= 'Z')
      encode(memBuf, c - 'A' + 10, FOUR_BIT);
    else if (c >= 'a' && c <= 'z')
      encode(memBuf, c - 'a' + 10, FOUR_BIT);
    else {
      alert("Unsupported character");
    }
  }
  
}

char *decodeSemiConstrainedHexString(packedDecode *memBuf) {
  int len, i;
  char *s, *baseptr;
  unsigned int n;
  
  len = decodeUnsignedSemiConstrainedInteger(memBuf, 0);  
  
  // add room for string plus null terminator
  if ((s = (char *)malloc(len + 1)) == NULL) {
    alert("Failed to allocate memory for string.");
    return NULL;      
  } 	
  baseptr = s;	
  
  for (i = 0; i < len; i++) {
    n = decode(memBuf, FOUR_BIT);
    *s = hexchar[n];
    s++;
  }
  *s = '\0';
  
  s = baseptr;
  
  return s;
}

//////

void encodeFixedLengthNumericString(packedEncode *memBuf, char *s, int len) {
  int i;
  char c;
    
  for (i = 0; i < len; i++) {
    c = *(s+i);
    if (c >= '0' && c <= '9')
      encode(memBuf, c - '0', FOUR_BIT);
    else if (c == '.')
      encode(memBuf, 10, FOUR_BIT);
    else if (c == '+')
      encode(memBuf, 11, FOUR_BIT);
    else if (c == '-')
      encode(memBuf, 12, FOUR_BIT);
    else {
      alert("Unsupported character");
    }
  }  
  
}

char *decodeFixedLengthNumericString(packedDecode *memBuf, int len) {  
  int i;
  char *s, *baseptr;
  unsigned int n;
    
  // add room for string plus null terminator
  if ((s = (char *)malloc(len + 1)) == NULL) {
    alert("Failed to allocate memory for string.");
    return NULL;      
  }   
  baseptr = s;
  
  for (i = 0; i < len; i++) {
    n = decode(memBuf, FOUR_BIT);
    *s = numchar[n];
    s++;
  }
  *s = '\0';
  
  s = baseptr;
  
  return s;
  
}


void encodeConstrainedNumericString(packedEncode *memBuf, char *s, int lb, int ub) {
  int bits, len, i;
  char c;
  
  bits = bits_required(lb, ub);

  // in case s is null
  (s) ? (len=strlen(s)) : (len=0);
  
  /* encode length as constrained whole number */
  encode(memBuf, len - lb, bits);
  
  for (i = 0; i < len; i++) {
    c = *(s+i);
    if (c >= '0' && c <= '9')
      encode(memBuf, c - '0', FOUR_BIT);
    else if (c == '.')
      encode(memBuf, 10, FOUR_BIT);
    else if (c == '+')
      encode(memBuf, 11, FOUR_BIT);
    else if (c == '-')
      encode(memBuf, 12, FOUR_BIT);
    else {
      alert("Unsupported character"); 
    }
  }  
}

char *decodeConstrainedNumericString(packedDecode *memBuf, int lb, int ub) {	
  int len, i;
  char *s, *baseptr;
  unsigned int n;
  
  /* get length as constrained whole number */
  len = decode(memBuf, bits_required(lb, ub)) + lb;

  // add room for string plus null terminator
  if ((s = (char *)malloc(len + 1)) == NULL) {
    alert("Failed to allocate memory for string.");
    return NULL;      
  }  
  baseptr = s;

  for (i = 0; i < len; i++) {
    n = decode(memBuf, FOUR_BIT);
    *s = numchar[n];
    s++;
  }
  *s = '\0';
  
  s = baseptr;
  
  return s;
}

void encodeSemiConstrainedNumericString(packedEncode *memBuf, char *s) {
  int len, i;
  char c;

  // in case s is null
  (s) ? (len=strlen(s)) : (len=0);
  
  // encode the length as semi constrained integer with 0 lb
  encodeUnsignedSemiConstrainedInteger(memBuf, len, 0);

  /* printf("len = %d\n", len); */
  
  for (i = 0; i < len; i++) {
    c = *(s+i);
    if (c >= '0' && c <= '9')
      encode(memBuf, c - '0', FOUR_BIT);
    else if (c == '.')
      encode(memBuf, 10, FOUR_BIT);
    else if (c == '+')
      encode(memBuf, 11, FOUR_BIT);
    else if (c == '-')
      encode(memBuf, 12, FOUR_BIT);
    else {
      alert("Unsupported character"); 
    }
  }  
}

char *decodeSemiConstrainedNumericString(packedDecode *memBuf) {	
  int len, i;
  char *s, *baseptr;
  unsigned int n;
  
  len = decodeUnsignedSemiConstrainedInteger(memBuf, 0);
  
  // add room for string plus null terminator
  if ((s = (char *)malloc(len + 1)) == NULL) {
    alert("Failed to allocate memory for string.");
    return NULL;      
  }   
  baseptr = s;  
  
  for (i = 0; i < len; i++) {
    n = decode(memBuf, FOUR_BIT);
    *s = numchar[n];
    s++;
  }
  *s = '\0';
  
  s = baseptr;
  
  return s;
}

//////

void encodeFixedLengthString(packedEncode *memBuf, char *s, int len) {
  int i;
  char c;
    
  for (i = 0; i < len; i++) {
    c = *(s+i);
    encode(memBuf, c, SEVEN_BIT);
  }
}

char *decodeFixedLengthString(packedDecode *memBuf, int len) {
  int i;
  char *s, *baseptr;
  
  // add room for string plus null terminator
  if ((s = (char *)malloc(len + 1)) == NULL) {
    alert("Failed to allocate memory for string.");
    return NULL;      
  }  
  baseptr = s;
  
  for (i = 0; i < len; i++) {
    *s = decode(memBuf, SEVEN_BIT);
    s++;
  }
  *s = '\0';
  
  s = baseptr;	
  
  return s;
}

void encodeConstrainedString(packedEncode *memBuf, char *s, int lb, int ub) {
  int bits, len, i;
  char c;
  
  bits = bits_required(lb, ub);

  // in case s is null
  (s) ? (len=strlen(s)) : (len=0);
  
  /* encode length as constrained whole number */
  encode(memBuf, len - lb, bits);
  
  for (i = 0; i < len; i++) {
    c = *(s+i);
    encode(memBuf, c, SEVEN_BIT);
  }
  
}

char *decodeConstrainedString(packedDecode *memBuf, int lb, int ub) {
	
  int len, i;
  char *s, *baseptr;
  
  /* get length as constrained whole number */
  len = decode(memBuf, bits_required(lb, ub)) + lb;
  
  // add room for string plus null terminator
  if ((s = (char *)malloc(len + 1)) == NULL) {
    alert("Failed to allocate memory for string.");
    return NULL;    
  }
  baseptr = s;
  
  for (i = 0; i < len; i++) {
    *s = decode(memBuf, SEVEN_BIT);
    s++;
  }
  *s = '\0';
  
  s = baseptr;
  
  return s;

}


void encodeSemiConstrainedString(packedEncode *memBuf, char *s) {
  int len, i;
  char c;

  // in case s is null
  (s) ? (len=strlen(s)) : (len=0);
    
  // encode the length as semi constrained integer with 0 lb
  encodeUnsignedSemiConstrainedInteger(memBuf, len, 0);
  
  for (i = 0; i < len; i++) {
    c = *(s+i);
    encode(memBuf, c, SEVEN_BIT);
  }
  
}

char *decodeSemiConstrainedString(packedDecode *memBuf) {
  int len, i;
  char *s, *baseptr;

  len = decodeUnsignedSemiConstrainedInteger(memBuf, 0);
  
  // add room for string plus null terminator
  if ((s = (char *)malloc(len + 1)) == NULL) {
    alert("Failed to allocate memory for string.");
    return NULL;      
  }
  baseptr = s;
  
  for (i = 0; i < len; i++) {
    *s = decode(memBuf, SEVEN_BIT);
    s++;
  }
  *s = '\0';
  
  s = baseptr;
  
  return s;
}

/* octetString */

void encodeFixedLengthOctetString(packedEncode *memBuf, char *s, int len) {
  int i;
    
  for (i = 0; i < len; i++) {
    encode(memBuf, (unsigned char)*(s+i), EIGHT_BIT);
  }
}

char *decodeFixedLengthOctetString(packedDecode *memBuf, int len) {
  int i;
  char *s, *baseptr;

  // add room for string plus null terminator
  if ((s = (char *)malloc(len + 1)) == NULL) {
    alert("Failed to allocate memory for string.");
    return NULL;       
  }  
  baseptr = s;
  
  for (i = 0; i < len; i++)
    *s++ = decode(memBuf, EIGHT_BIT);
  *s = '\0';
  
  s = baseptr;
	
  return s;
}

void encodeConstrainedOctetString(packedEncode *memBuf, char *s, int lb, int ub) {
  int bits, len, i;
  
  bits = bits_required(lb ,ub);

  // in case s is null
  (s) ? (len=strlen(s)) : (len=0);
  
  /* encode length as constrained whole number */
  encode(memBuf, len - lb, bits);
  
  for (i = 0; i < len; i++) {
    encode(memBuf, (unsigned char)*(s+i), EIGHT_BIT);
  }
  
}

char *decodeConstrainedOctetString(packedDecode *memBuf, int lb, int ub) {
  int len, i;
  char *s, *baseptr;
  
  /* get length as constrained whole number */
  len = decode(memBuf, bits_required(lb, ub)) + lb;
  
  // add room for string plus null terminator
  if ((s = (char *)malloc(len + 1)) == NULL) {
    alert("Failed to allocate memory for string.");
    return NULL;      
  }	
  baseptr = s;
  
  for (i = 0; i < len; i++)
    *s++ = decode(memBuf, EIGHT_BIT);
  *s = '\0';
  
  s = baseptr;
  
  return s;
        
}


void encodeSemiConstrainedOctetString(packedEncode *memBuf, char *s) {
  int len, i;

  // in case s is null
  (s) ? (len=strlen(s)) : (len=0);
  
  // encode the length as semi constrained integer with 0 lb
  encodeUnsignedSemiConstrainedInteger(memBuf, len, 0);
  
  for (i = 0; i < len; i++) {
    encode(memBuf, (unsigned char)*(s+i), EIGHT_BIT);
  }
  
}

char *decodeSemiConstrainedOctetString(packedDecode *memBuf) {
  int len, i;
  char *s, *baseptr;
  
  len = decodeUnsignedSemiConstrainedInteger(memBuf, 0);
  
  // add room for string plus null terminator
  if ((s = (char *)malloc(len + 1)) == NULL) {
    alert("Failed to allocate memory for string.");
    return NULL;      
  }  
  baseptr = s;
  
  for (i = 0; i < len; i++)
    *s++ = decode(memBuf, EIGHT_BIT);
  *s = '\0';
  
  s = baseptr; 
  
  return s;
}



void encodeUnconstrainedInteger(packedEncode *memBuf, signed long int n) {
  
  if (n >= SCHAR_MIN && n <= SCHAR_MAX) {
    uint8_t n8 = n;
    encode(memBuf, 0, 2);
    encode(memBuf, n8, 8);
  } else if (n >= SHRT_MIN && n <= SHRT_MAX) {
    uint16_t n16 = n;
    encode(memBuf, 1, 2);
    encode(memBuf, n16, 16);
  } else if (n >= INT_MIN && n <= INT_MAX) {
    uint32_t n32 = n;
    encode(memBuf, 2, 2);
    encode(memBuf, n32, 32);    
  } else {
    alert("Unsupported integer range.");
  }
  
}

signed long int decodeUnconstrainedInteger(packedDecode *memBuf) {
  int prefix = 0;
  signed long int n = 0;

  prefix = decode(memBuf, 2);
  dbg("prefix:%d", prefix);

  
  if (prefix == 0) {
    int8_t n8;
    n = (signed char)decode(memBuf, 8);
    n8 = n;
    return n8;
  } else if (prefix == 1) {
    int16_t n16;
    n = (signed short)decode(memBuf, 16);
    n16 = n;
    return n16;
  } else if (prefix == 2) {
    int32_t n32;
    n = (signed long)decode(memBuf, 32);
    n32 = n;
    return n32;
  } else {
    alert("Invalid integer prefix.");
    longjmp(decode_exception_env, DECODE_INVALID_PREFIX);
  }

  // dummy return
  return n;
}


void encodeUnsignedSemiConstrainedInteger(packedEncode *memBuf, signed long int n, signed long int lb)
{

  // adjust n by lower bound so n is always positive
  n = n - lb;

  dbg("n:%lu", (unsigned long int)n);
  
  if (n <= UCHAR_MAX) {
    encode(memBuf, 0, 2);
    encode(memBuf, (unsigned char)n, 8);
  } else if (n <= USHRT_MAX) {
    encode(memBuf, 1, 2);
    encode(memBuf, (unsigned short)n, 16);
  } else if (n <= UINT_MAX) {
    encode(memBuf, 2, 2);
    encode(memBuf, (unsigned long)n, 32);
  } else {
    // nothing to see here move along
  }
  
}

signed long int decodeUnsignedSemiConstrainedInteger(packedDecode *memBuf, signed long int lb)
{
  unsigned long int n = 0;
  int prefix = 0;
  
  prefix = decode(memBuf, 2);
  
  if (prefix == 0) {
    n = (unsigned char)decode(memBuf, 8);
  } else if (prefix == 1) {
    n = (unsigned short)decode(memBuf, 16);
  } else if (prefix == 2) {
    n = (unsigned long)decode(memBuf, 32);
  } else {
    alert("Invalid integer prefix.");
    longjmp(decode_exception_env, DECODE_INVALID_PREFIX);
  }

  dbg("n: %lu, lb: %ld", n, lb);
  
  return n + lb;	
  
}

void encodeUnsignedConstrainedInteger(packedEncode *memBuf, signed long int n, signed long int lb, signed long int ub)
{
  int bits = bits_required(lb, ub);
  encode(memBuf, (unsigned long)(n - lb), bits);
}

signed long int decodeUnsignedConstrainedInteger(packedDecode *memBuf, signed long int lb, signed long int ub)
{
  return (decode(memBuf, (unsigned long)bits_required(lb, ub)) + lb);
}


void encodeEnumerated(packedEncode *memBuf, unsigned long int n, unsigned len) {
  /* ub = length - 1 */
  encodeUnsignedConstrainedInteger(memBuf, n, 0, len-1);
}

unsigned long int decodeEnumerated(packedDecode *memBuf, unsigned len) {
  return (decodeUnsignedConstrainedInteger(memBuf, 0, len-1));
}

void encodeBitmap(packedEncode *memBuf, unsigned long int n, int bits)
{
  encode(memBuf, n, bits);
}

unsigned long int decodeBitmap(packedDecode *memBuf, int bits)
{
  return (decode(memBuf, bits));
}



void encodeChoiceIndex(packedEncode *memBuf, unsigned long int n, unsigned len) {
  encodeUnsignedConstrainedInteger(memBuf, n, 1, len);
}

unsigned long int decodeChoiceIndex(packedDecode *memBuf, unsigned len) {
  return (decodeUnsignedConstrainedInteger(memBuf, 1, len));
}


void encodeSequenceOfLength(packedEncode *memBuf, int len) {
  //encodeGeneralLengthDeterminant(memBuf, len);
  encodeUnsignedSemiConstrainedInteger(memBuf, len, 0);
}

int decodeSequenceOfLength(packedDecode *memBuf) {
  //return (decodeGeneralLengthDeterminant(memBuf));
  return (decodeUnsignedSemiConstrainedInteger(memBuf, 0));
}

// string interface for convenience
void encodeDecimal(packedEncode *memBuf, char *n)
{
  // use numeric string format
  encodeSemiConstrainedNumericString(memBuf, n);

}

// string interface for convenience
char *decodeDecimal(packedDecode *memBuf)
{
  return (decodeSemiConstrainedNumericString(memBuf));

}

// string interface for convenience
void encodeCurrency(packedEncode *memBuf, char *n)
{

  double x = 0.0;
  unsigned long int y = 0;
  
  // need to check for error
  x = strtod((const char *)n, NULL);

  dbg("x:%f", x);
  // get rid of 2 dp
  y = round(x * 100);
  dbg("y:%ld", y);

  encodeUnsignedSemiConstrainedInteger(memBuf, y, 0);  
  
} 

// string interface for convenience
char *decodeCurrency(packedDecode *memBuf)
{
  char *n = NULL;
  unsigned long int x = 0;
  double y = 0.0;

  // allocating on heap to be consistent with other string functions
  if ((n = (char *)malloc(12)) == NULL) {
    alert("Insufficient memory.");
    return NULL;
  }
  x = decodeUnsignedSemiConstrainedInteger(memBuf, 0);
  dbg("x:%lu", x);
  y = x;
  // restore 2 dp
  y = y/100;
  dbg("y:%f", y);
  sprintf(n, "%.2f", y);

  // needs to be freed
  return n;
}


// string interface for convenience
void encodeIPv4Address(packedEncode *memBuf, char *dottedquad)
{
  struct in_addr addr;

  // Convert address to byte order
  if(!inet_pton(AF_INET, dottedquad, &addr)) {
    alert("Could not convert address");
  }
  dbg("ip:%u", addr.s_addr);
  // covers the valid range
  encodeUnsignedConstrainedInteger(memBuf, addr.s_addr, 1, 4294967263);  
  
} 

// string interface for convenience
char *decodeIPv4Address(packedDecode *memBuf)
{
  char *ip = NULL;
  struct in_addr addr;
    
  
  // allocating on heap to be consistent with other string functions
  if ((ip = (char *)malloc(16)) == NULL) {
    alert("Insufficient memory.");
    return NULL;
  }
  // covers the valid range value
  addr.s_addr = decodeUnsignedConstrainedInteger(memBuf, 1, 4294967263);
  dbg("ip:%u", addr.s_addr);
  if(inet_ntop(AF_INET, &addr.s_addr, ip, 16) == NULL) {
    alert("Invalid IP address.");
  }  
  dbg("dotted quad:%s", ip);
  
  // needs to be freed
  return ip;
}


// utility func
static time_t rfc3339string_to_epoch(const char *timestring)
{
  struct tm tm;
  time_t t;
  time_t offset;

  memset(&tm, 0, sizeof(struct tm));
  
  if (strptime(timestring, "%FT%TZ", &tm) != 0) goto done;
  if (strptime(timestring, "%F %TZ", &tm) != 0) goto done;  
  if (strptime(timestring, "%FT%T%z", &tm) != 0) goto done;
  if (strptime(timestring, "%F %T%z", &tm) != 0) goto done;  
  
  // fall through to error
  alert("strptime() failed.");
  return 0;
  
 done:

  offset = tm.tm_gmtoff;
  
  tm.tm_isdst = -1;
  t = timegm(&tm);
  if (t == -1) {
    alert("daylight saving error.");
    return 0;    
  }
  
  // add time zone offset
  return t + offset;
  
}

// utility function
static char *epoch_to_rfc3339string(char *buf, int size, time_t t)
{
  const char *format = "%FT%TZ";
  
  if (strftime(buf, size, format, gmtime(&t)) == 0) {
    alert("strftime failed.");
  }
  
  return buf;
  
}

void encodeUnixTime(packedEncode *memBuf, char *timestring)
{
  time_t t;

  t =  rfc3339string_to_epoch(timestring);
  dbg("epoch:%ld", (long)t);
  encodeUnconstrainedInteger(memBuf, (long)t);  
  
}

char *decodeUnixTime(packedDecode *memBuf)
{
  char *timestring = NULL;
  time_t t;
    
  
  // allocating on heap to be consistent with other string functions
  if ((timestring = (char *)malloc(30)) == NULL) {
    alert("Insufficient memory.");
    return NULL;
  }
  t = (time_t)decodeUnconstrainedInteger(memBuf);
  dbg("epoch:%ld", (long)t);
  epoch_to_rfc3339string(timestring, 30, t);
  dbg("timestring:%s", timestring);
  
  // needs to be freed
  return timestring;
}
