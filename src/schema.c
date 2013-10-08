#include <unistd.h>
#include "schema.h"

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

static schemaData *compile_schema(xmlDoc *schema)
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

static void free_validation(schemaData *schemap)
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

void schema_free_validation(packedobjectsContext *pc)
{
  
  free_validation(pc->schemap);
  
}

static int validate_schema_rules(xmlDocPtr doc)
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
  if ((schemarulesp = compile_schema(doc_schemarules)) == NULL) {
    alert("Failed to preprocess schema.");
    xmlFreeDoc(doc_schemarules);
    return 1;
  }
  if (xmlSchemaValidateDoc(schemarulesp->validCtxt, doc)) {
    alert("Failed to validate schema.");
    xmlFreeDoc(doc_schemarules);
    free_validation(schemarulesp);
    return 1;
  } 

  // validation passed
  xmlFreeDoc(doc_schemarules);
  free_validation(schemarulesp);
  
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
  } else if ( access( "app/native", F_OK ) != -1 ) {
    return "app/native/packedobjectsSchemaTypes.xsd";
  } else return NULL;
}

static int validate_schema_sequence(xmlDocPtr doc)
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

int schema_setup_xpath(packedobjectsContext *pc)
{
  xmlXPathContextPtr xpathp = NULL;
  
  // setup xpath
  xpathp = xmlXPathNewContext(pc->doc_canonical_schema);
  if (xpathp == NULL) {
    alert("Error in xmlXPathNewContext.");
    return -1;
  }

  if(xmlXPathRegisterNs(xpathp, (const xmlChar *)NSPREFIX, (const xmlChar *)NSURL) != 0) {
    alert("Error: unable to register NS.");
    return -1;
  }

  pc->xpathp = xpathp;

  return 0;
}

int schema_validate_schema(packedobjectsContext *pc)
{

  // validate the schema to make sure it conforms to packedobjects schema
  if (validate_schema_rules(pc->doc_schema)) {
    return -1;
  }

  // validate the schema to make sure repeating sequences conform to packedobjects schema
  if (validate_schema_sequence(pc->doc_schema)) {
    return -1;
  }  

  return 0;
  
}

int schema_setup_validation(packedobjectsContext *pc)
{
  schemaData *schemap = NULL;
  
  // setup validation context
  if ((schemap = compile_schema(pc->doc_schema)) == NULL) {
    alert("Failed to preprocess schema.");
    return -1;
  }

  pc->schemap = schemap;  

  return 0;
}

void schema_free_xpath(packedobjectsContext *pc)
{

  xmlXPathFreeContext(pc->xpathp);
  

}

int schema_setup_schema(packedobjectsContext *pc, const char *schema_file)
{  
  xmlDoc *doc_schema = NULL;
  
  if ((doc_schema = packedobjects_new_doc(schema_file)) == NULL) {
    alert("Failed to create doc.");
    return -1;
  }
  
  // set the schema
  pc->doc_schema = doc_schema;  

  return 0;
  
}

void schema_free(packedobjectsContext *pc)
{
  xmlFreeDoc(pc->doc_schema);
}
