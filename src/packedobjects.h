#ifndef PACKEDOBJECTS_H_
#define PACKEDOBJECTS_H_

#include <libxml/parser.h>
#include <libxml/tree.h>
#include <libxml/xmlschemas.h>
#include <libxml/xmlschemastypes.h>
#include <libxml/xpath.h>
#include <libxml/xpathInternals.h>

#include "config.h"
#include "schema.h"
#include "encode.h"
#include "decode.h"

enum STRING_TYPES { STRING, BIT_STRING, NUMERIC_STRING, HEX_STRING, OCTET_STRING };

enum ERROR_CODES {
  ENCODE_VALIDATION_FAILED = 100,
  ENCODE_PDU_BUFFER_FULL,
  DECODE_VALIDATION_FAILED,
  DECODE_INVALID_PREFIX,
};

typedef struct {
  xmlDoc *doc_data;
  xmlDoc *doc_schema;
  xmlDoc *doc_canonical_schema;
  schemaData *schemap;
  xmlXPathContextPtr xpathp;
  xmlHashTablePtr udt;
  const xmlChar *start_element_name;
  packedEncode *encodep;
  packedDecode *decodep;
  int bytes;
  int encode_error;
  int decode_error;
} packedobjectsContext;

xmlDocPtr packedobjects_new_doc(const char *file);
char *packedobjects_encode(packedobjectsContext *pc, xmlDocPtr doc);
xmlDocPtr packedobjects_decode(packedobjectsContext *pc, char *pdu);
packedobjectsContext *init_packedobjects(const char *schema_file);
void free_packedobjects(packedobjectsContext *poCtxPtr);
void packedobjects_dump_doc(xmlDoc *doc);

#endif
