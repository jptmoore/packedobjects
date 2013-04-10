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


static xmlChar *get_start_element(xmlDocPtr doc)
{
  xmlChar *element_name = NULL;
  xmlNodePtr root_node = NULL, cur_node = NULL;
  
  root_node = xmlDocGetRootElement(doc);
  cur_node = root_node->children;
  
  while (cur_node != NULL) {
    if ((!xmlStrcmp(cur_node->name, (const xmlChar *)"element"))) {
      element_name = xmlGetProp(cur_node, (const xmlChar *)"name");      
      break;
    }
    cur_node = cur_node->next;
  }  

  return element_name;
}

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

static int set_packedobjects_start_element(packedobjectsContext *pc)
{
  xmlChar *start_element_name = NULL;
  
  // set start element in schema
  if ((start_element_name = get_start_element(pc->doc_schema))) { 
    pc->start_element_name = start_element_name;
    return 0;
  } else {
    alert("Failed to find start element in schema.");
    return -1;    
  }
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

static int create_user_defined_types(packedobjectsContext *pc)
{
  xmlHashTablePtr udt = NULL;
  
  // create user defined types hash table
  if ((udt = xmlHashCreate(100)) == NULL) {
    alert("Failed to create hash table.");
    return -1;
  }
  pc->udt = udt;
  hash_user_defined_types(pc);  

  return 0;
  
}

static int expand_schema(packedobjectsContext *pc)
{
  xmlDoc *doc_expanded_schema = NULL;
  
  // create expanded schema without user defined types
  doc_expanded_schema = expand_user_defined_types(pc);
#ifdef DEBUG_MODE
  packedobjects_dump_doc_to_file("/tmp/expand.xml", doc_expanded_schema);
#endif
  pc->doc_expanded_schema = doc_expanded_schema;  

  return 0;
  
}

static int make_canonical_schema(packedobjectsContext *pc)
{
  xmlDoc *doc_canonical_schema = NULL;

  // create the canonical schema we will use for encoding/decoding
  doc_canonical_schema = packedobjects_make_canonical_schema(pc);
#ifdef DEBUG_MODE
  packedobjects_dump_doc_to_file("/tmp/canon.xml", doc_canonical_schema);
#endif  
  pc->doc_canonical_schema = doc_canonical_schema;

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
  
  // set start element in schema
  if (set_packedobjects_start_element(pc) == -1) {
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

  // record any user define types
  if (create_user_defined_types(pc) == -1) {
    return NULL;
  }
  
  // expand user defined types in schema
  if (expand_schema(pc) == -1) {
    return NULL;
  }

  // make the canonical schema used for encoding
  if (make_canonical_schema(pc) == -1) {
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



