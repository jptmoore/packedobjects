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
#include "ier.h"

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
  xmlDoc *doc_expanded_schema;
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


// some utility functions
xmlDocPtr packedobjects_new_doc(const char *file);
void packedobjects_dump_doc(xmlDoc *doc);
void packedobjects_dump_doc_to_file(const char *fname, xmlDoc *doc);

// the API
#include "packedobjects_init.h"
#include "packedobjects_encode.h"
#include "packedobjects_decode.h"


#endif
