#include <stdio.h>
#include <setjmp.h>
#include <string.h>

#include "packedobjects_init.h"

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

#define CHECK_BIT(var,pos) ((var) & (1<<(pos)))


static packedobjectsContext *_init_packedobjects(const char *schema_file)
{
  packedobjectsContext *pc = NULL;
  xmlDoc *doc_schema = NULL;
  
  if ((pc = (packedobjectsContext *)malloc(sizeof(packedobjectsContext))) == NULL) {
    alert("Could not alllocate memory.");
    return NULL;    
  }
  
  if ((doc_schema = packedobjects_new_doc(schema_file)) == NULL) {
    alert("Failed to create doc.");
    return NULL;
  }

  // set the schema
  pc->doc_schema = doc_schema;
  // set some defaults
  pc->doc_data = NULL;
  pc->doc_expanded_schema = NULL;
  pc->doc_canonical_schema = NULL;
  pc->schemap = NULL;
  pc->xpathp = NULL;
  pc->udt = NULL;
  pc->start_element_name = NULL;
  pc->encodep = NULL;
  pc->decodep = NULL;
  pc->pdu_size = 0;
  pc->bytes = 0;
  pc->encode_error = 0;
  pc->decode_error = 0;

  return pc;
  
}

static int validate_schema(packedobjectsContext *pc)
{

  // validate the schema to make sure it conforms to packedobjects schema
  if (schema_validate_schema_rules(pc->doc_schema)) {
    return -1;
  }

  // validate the schema to make sure repeating sequences conform to packedobjects schema
  if (schema_validate_schema_sequence(pc->doc_schema)) {
    return -1;
  }  

  return 0;
  
}

static int setup_data_validation(packedobjectsContext *pc)
{
  schemaData *schemap = NULL;
  
  // setup validation context
  if ((schemap = schema_compile_schema(pc->doc_schema)) == NULL) {
    alert("Failed to preprocess schema.");
    return -1;
  }

  pc->schemap = schemap;  

  return 0;
}

static int setup_xpath(packedobjectsContext *pc)
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


packedobjectsContext *init_packedobjects(const char *schema_file, size_t bytes)
{
  packedobjectsContext *pc = NULL;

  // do the real allocation of structure with the provided schema
  if ((pc = _init_packedobjects(schema_file)) == NULL) {
    return NULL;
  }

  // used to store the encoded data
  if (encode_make_memory(pc, bytes) == -1) {
    return NULL;
  }
  
  // validate the schema conforms to PO schema
  if (validate_schema(pc) == -1) {
    return NULL;
  }
  
  // initialise xml validation code for later 
  if (setup_data_validation(pc) == -1) {
    return NULL;
  }
  
  // expand user defined types in schema
  if (expand_make_expanded_schema(pc) == -1) {
    return NULL;
  }

  // make the canonical schema used for encoding
  if (canon_make_canonical_schema(pc) == -1) {
    return NULL;
  }

  // setup xpath to use during encode
  if (setup_xpath(pc) == -1) {
    return NULL;
  }
  
  return pc;
}

void free_packedobjects(packedobjectsContext *pc)
{
  schema_free_schema(pc->schemap);
  xmlFreeDoc(pc->doc_schema);
  xmlFreeDoc(pc->doc_expanded_schema);
  xmlFreeDoc(pc->doc_canonical_schema);
  // contents in hash table should be freed already
  xmlHashFree(pc->udt, NULL);
  xmlXPathFreeContext(pc->xpathp);
  xmlFree(BAD_CAST pc->start_element_name);
  encode_free_memory(pc);
  free(pc);

  xmlCleanupParser();
}



