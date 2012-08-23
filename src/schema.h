#ifndef SCHEMA_H_
#define SCHEMA_H_

#include "packedobjects.h"

typedef struct {
  xmlSchemaParserCtxtPtr parserCtxt;
  xmlSchemaPtr schemaPtr;
  xmlSchemaValidCtxtPtr validCtxt;
} schemaData;

schemaData *xml_compile_schema(xmlDoc *schema);
void xml_free_schema(schemaData *schemap);

#endif
