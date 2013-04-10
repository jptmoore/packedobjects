#ifndef SCHEMA_H_
#define SCHEMA_H_

#include "packedobjects.h"

typedef struct {
  xmlSchemaParserCtxtPtr parserCtxt;
  xmlSchemaPtr schemaPtr;
  xmlSchemaValidCtxtPtr validCtxt;
} schemaData;

schemaData *schema_compile_schema(xmlDoc *schema);
void schema_free_schema(schemaData *schemap);
int schema_validate_schema_rules(xmlDocPtr doc);
int schema_validate_schema_sequence(xmlDocPtr doc);

#endif
