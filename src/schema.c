#include "packedobjects.h"


#ifdef DEBUG_MODE
#define dbg(fmtstr, args...) \
  (printf(PROGNAME ":%s: " fmtstr "\n", __func__, ##args))
#else
#define dbg(dummy...)
#endif


schemaData *xml_compile_schema(xmlDoc *schema)
{
  xmlSchemaParserCtxtPtr parserCtxt = NULL;
  xmlSchemaPtr schemaPtr = NULL;
  xmlSchemaValidCtxtPtr validCtxt = NULL;

  schemaData *schemap;

  if ((schemap = (schemaData *)malloc(sizeof(schemaData))) == NULL) {
    fprintf(stderr, "Could not alllocate memory.\n");
    exit(1);    
  }
  
  parserCtxt = xmlSchemaNewDocParserCtxt(schema);
  if (parserCtxt == NULL) {
    fprintf(stderr, "Could not create XSD schema parsing context.\n");
    exit(1);
  }
  schemaPtr = xmlSchemaParse(parserCtxt);
  if (schemaPtr == NULL) {
    fprintf(stderr, "Could not parse XSD schema.\n");
    exit(1);
  }
  validCtxt = xmlSchemaNewValidCtxt(schemaPtr);
  if (!validCtxt) {
    fprintf(stderr, "Could not create XSD schema validation context.\n");
    exit(1);
  }    

  schemap->parserCtxt = parserCtxt;
  schemap->schemaPtr = schemaPtr;
  schemap->validCtxt = validCtxt;
    
  return schemap;
}

void xml_free_schema(schemaData *schemap)
{

  if (schemap->parserCtxt) {
    xmlSchemaFreeParserCtxt(schemap->parserCtxt);
  }
  if (schemap->schemaPtr) {
    xmlSchemaFree(schemap->schemaPtr);
  }

  if (schemap->validCtxt) {
    xmlSchemaFreeValidCtxt(schemap->validCtxt);
  }

  free(schemap);
  
}
