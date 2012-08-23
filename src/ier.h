#ifndef IER_H_
#define IER_H_

#include "encode.h"
#include "decode.h"

void encodeBoolean(packedEncode *memBuf, int flag);
int decodeBoolean(packedDecode *memBuf);

void encodeConstrainedBitString(packedEncode *memBuf, char *s, int lb, int ub);
char *decodeConstrainedBitString(packedDecode *memBuf, int lb, int ub);

void encodeFixedLengthBitString(packedEncode *memBuf, char *s, int len);
char *decodeFixedLengthBitString(packedDecode *memBuf, int len);

void encodeSemiConstrainedBitString(packedEncode *memBuf, char *s);
char *decodeSemiConstrainedBitString(packedDecode *memBuf);

void encodeFixedLengthHexString(packedEncode *memBuf, char *s, int len);
char *decodeFixedLengthHexString(packedDecode *memBuf, int len);

void encodeConstrainedHexString(packedEncode *memBuf, char *s, int lb, int ub);
char *decodeConstrainedHexString(packedDecode *memBuf, int lb, int ub);

void encodeSemiConstrainedHexString(packedEncode *memBuf, char *s);
char *decodeSemiConstrainedHexString(packedDecode *memBuf);

void encodeFixedLengthNumericString(packedEncode *memBuf, char *s, int len);
char *decodeFixedLengthNumericString(packedDecode *memBuf, int len);

void encodeConstrainedNumericString(packedEncode *memBuf, char *s, int lb, int ub);
char *decodeConstrainedNumericString(packedDecode *memBuf, int lb, int ub);

void encodeSemiConstrainedNumericString(packedEncode *memBuf, char *s);
char *decodeSemiConstrainedNumericString(packedDecode *memBuf);

void encodeFixedLengthString(packedEncode *membuf, char *s, int len);
char *decodeFixedLengthString(packedDecode *memBuf, int len);

void encodeConstrainedString(packedEncode *memBuf, char *s, int lb, int ub);
char *decodeConstrainedString(packedDecode *memBuf, int lb, int ub);

void encodeSemiConstrainedString(packedEncode *memBuf, char *s);
char *decodeSemiConstrainedString(packedDecode *memBuf);

void encodeFixedLengthOctetString(packedEncode *membuf, char *s, int len);
char *decodeFixedLengthOctetString(packedDecode *memBuf, int len);

void encodeConstrainedOctetString(packedEncode *memBuf, char *s, int lb, int ub);
char *decodeConstrainedOctetString(packedDecode *memBuf, int lb, int ub);

void encodeSemiConstrainedOctetString(packedEncode *memBuf, char *s);
char *decodeSemiConstrainedOctetString(packedDecode *memBuf);

void encodeUnconstrainedInteger(packedEncode *memBuf, signed long int n);
signed long int decodeUnconstrainedInteger(packedDecode *memBuf);

void encodeUnsignedConstrainedInteger(packedEncode *memBuf, signed long int n, signed long int lb, signed long int ub);
signed long int decodeUnsignedConstrainedInteger(packedDecode *memBuf, signed long int lb, signed long int ub);

void encodeUnsignedSemiConstrainedInteger(packedEncode *memBuf, signed long int n, signed long int lb);
signed long int decodeUnsignedSemiConstrainedInteger(packedDecode *memBuf, signed long int lb);

void encodeEnumerated(packedEncode *memBuf, unsigned long int n, unsigned len);
unsigned long int decodeEnumerated(packedDecode *memBuf, unsigned len);

void encodeBitmap(packedEncode *memBuf, unsigned long int n, int bits);
unsigned long int decodeBitmap(packedDecode *memBuf, int bits);


void encodeChoiceIndex(packedEncode *memBuf, unsigned long int n, unsigned len);
unsigned long int decodeChoiceIndex(packedDecode *memBuf, unsigned len);

void encodeSequenceOfLength(packedEncode *memBuf, int len);
int decodeSequenceOfLength(packedDecode *memBuf);

// string interface for convenience
void encodeDecimal(packedEncode *memBuf, char *n);
char *decodeDecimal(packedDecode *memBuf);

// string interface for convenience
void encodeCurrency(packedEncode *memBuf, char *n);
char *decodeCurrency(packedDecode *memBuf);

// string interface
void encodeIPv4Address(packedEncode *memBuf, char *dottedquad);
char *decodeIPv4Address(packedDecode *memBuf);


#endif
