#ifndef PACKEDOBJECTS_H_
#define PACKEDOBJECTS_H_

#include <libxml/parser.h>
#include <libxml/tree.h>
#include <libxml/xmlschemas.h>
#include <libxml/xmlschemastypes.h>
#include <libxml/xpath.h>
#include <libxml/xpathInternals.h>

#include "config.h"

#include "ier.h"

enum STRING_TYPES { STRING, BIT_STRING, NUMERIC_STRING, HEX_STRING, OCTET_STRING };

enum ERROR_CODES {
  INIT_FAILED = 100,
  INIT_SCHEMA_SETUP_FAILED,
  INIT_ENCODE_SETUP_FAILED,
  INIT_SCHEMA_VALIDATION_FAILED,
  INIT_SETUP_VALIDATION_FAILED,
  INIT_EXPANDED_SCHEMA_FAILED,
  INIT_CANON_SCHEMA_FAILED,
  INIT_XPATH_SETUP_FAILED,
  ENCODE_VALIDATION_FAILED,
  ENCODE_PDU_BUFFER_FULL,
  DECODE_VALIDATION_FAILED,
  DECODE_INVALID_PREFIX,
};

typedef struct {
  xmlSchemaParserCtxtPtr parserCtxt;
  xmlSchemaPtr schemaPtr;
  xmlSchemaValidCtxtPtr validCtxt;
} schemaData;

typedef struct {
  xmlDoc *doc_data;
  xmlDoc *doc_schema;
  xmlDoc *doc_expanded_schema;
  xmlDoc *doc_canonical_schema;
  schemaData *schemap;
  xmlXPathContextPtr xpathp;
  xmlHashTablePtr udt;
  const xmlChar *start_element_name;
  packedEncode *encodep;
  packedDecode *decodep;
  size_t pdu_size;
  int bytes;
  int init_error;
  int encode_error;
  int decode_error;
} packedobjectsContext;


// some utility functions
xmlDocPtr packedobjects_new_doc(const char *file);
void packedobjects_dump_doc(xmlDoc *doc);
void packedobjects_dump_doc_to_file(const char *fname, xmlDoc *doc);

// the API
#include "packedobjects_init.h"
#include "packedobjects_encode.h"
#include "packedobjects_decode.h"


#endif
