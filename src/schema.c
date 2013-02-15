#include <unistd.h>
#include "packedobjects.h"


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
  (fprintf(stderr, PROGNAME ":%s: " fmtstr "\n", __func__, ##args))
#endif

static const char *schema_rules_file();

schemaData *xml_compile_schema(xmlDoc *schema)
{
  xmlSchemaParserCtxtPtr parserCtxt = NULL;
  xmlSchemaPtr schemaPtr = NULL;
  xmlSchemaValidCtxtPtr validCtxt = NULL;

  schemaData *schemap;

  if ((schemap = (schemaData *)malloc(sizeof(schemaData))) == NULL) {
    fprintf(stderr, "Could not alllocate memory.\n");
    return NULL;
  }
  
  parserCtxt = xmlSchemaNewDocParserCtxt(schema);
  if (parserCtxt == NULL) {
    fprintf(stderr, "Could not create XSD schema parsing context.\n");
    return NULL;
  }
  schemaPtr = xmlSchemaParse(parserCtxt);
  if (schemaPtr == NULL) {
    fprintf(stderr, "Could not parse XSD schema.\n");
    exit(1);
  }
  validCtxt = xmlSchemaNewValidCtxt(schemaPtr);
  if (!validCtxt) {
    fprintf(stderr, "Could not create XSD schema validation context.\n");
    return NULL;
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

int xml_validate_schema_rules(xmlDocPtr doc)
{
  xmlDocPtr doc_schemarules = NULL;
  schemaData *schemarulesp = NULL;
  const char *fname = NULL;

  if ((fname = schema_rules_file()) == NULL) {
    alert("failed to find schema rules.");
    return 1;
  }

  if ((doc_schemarules = packedobjects_new_doc(fname)) == NULL) {
    alert("Failed to create doc.");  
    return 1;
  }
  if ((schemarulesp = xml_compile_schema(doc_schemarules)) == NULL) {
    alert("Failed to preprocess schema.");
    xmlFreeDoc(doc_schemarules);
    return 1;
  }
  if (xmlSchemaValidateDoc(schemarulesp->validCtxt, doc)) {
    alert("Failed to validate schema.");
    xmlFreeDoc(doc_schemarules);
    xml_free_schema(schemarulesp);
    return 1;
  } 

  // validation passed
  xmlFreeDoc(doc_schemarules);
  xml_free_schema(schemarulesp);
  
  return 0;

}

// need to try and work out where the schema rules file was installed
static const char *schema_rules_file()
{
  // we only check the sensible prefixes of /usr/local and /usr
  if ( access( "/usr/local/share/libpackedobjects", F_OK ) != -1 ) {
    return "/usr/local/share/libpackedobjects/packedobjectsSchemaTypes.xsd";
  } else if ( access( "/usr/share/libpackedobjects", F_OK ) != -1 ) {
    return "/usr/share/libpackedobjects/packedobjectsSchemaTypes.xsd";
  } else return NULL;
}

int xml_validate_schema_sequence(xmlDocPtr doc)
{
  xmlXPathContextPtr xpathp = NULL;
  xmlXPathObjectPtr result = NULL;
  xmlNodeSetPtr nodes = NULL;
  xmlNodePtr node = NULL;
  int i;

  // query to find repeating sequence types
  char *xpath = "//xs:element[@maxOccurs]/..";
  
  // setup xpath
  xpathp = xmlXPathNewContext(doc);
  if (xpathp == NULL) {
    alert("Error in xmlXPathNewContext.");
    return 1;
  }
  
  if(xmlXPathRegisterNs(xpathp, (const xmlChar *)NSPREFIX, (const xmlChar *)NSURL) != 0) {
    xmlXPathFreeContext(xpathp);
    alert("Error: unable to register NS.");
    return 1;
  }

  // find all the sequences with maxOccurs
  result = xmlXPathEvalExpression(xpath, xpathp);

  if (result == NULL) {
    xmlXPathFreeContext(xpathp);
    alert("Error in xmlXPathEvalExpression.");
    return NULL;
  }

  // this means there were no repeating sequence types which is fine
  if(xmlXPathNodeSetIsEmpty(result->nodesetval)){
    xmlXPathFreeContext(xpathp);
    xmlXPathFreeObject(result);
    return 0;
  }

  nodes = result->nodesetval;
  for (i=0; i < nodes->nodeNr; i++) {
    // examine each sequence to see how many children it has
    node = nodes->nodeTab[i];
    if ((xmlChildElementCount(node)) != 1) {
      alert("Repeating %s type at line %d has more than one child.", node->name, node->line);
      xmlXPathFreeContext(xpathp);
      xmlXPathFreeObject(result);
      return 1;
    }
  }

  // repeating sequences conform
  xmlXPathFreeObject(result);
  xmlXPathFreeContext(xpathp);
  
  return 0;
}
