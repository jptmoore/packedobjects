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


static xmlChar *get_start_element(xmlDocPtr doc);

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

packedobjectsContext *init_packedobjects(const char *schema_file)
{
  xmlDoc *doc_schema = NULL, *doc_canonical_schema = NULL, *doc_expanded_schema = NULL;
  schemaData *schemap = NULL;
  packedobjectsContext *poCtxPtr = NULL;
  xmlXPathContextPtr xpathp = NULL;
  xmlHashTablePtr udt = NULL;
  xmlChar *start_element_name = NULL;
  char *pdu = NULL;
  packedEncode *encodep = NULL;
  
  if ((poCtxPtr = (packedobjectsContext *)malloc(sizeof(packedobjectsContext))) == NULL) {
    alert("Could not alllocate memory.");
    return NULL;    
  }
  
  if ((doc_schema = packedobjects_new_doc(schema_file)) == NULL) {
    alert("Failed to create doc.");
    return NULL;
  }

  // supplied at encode
  poCtxPtr->doc_data = NULL;
  poCtxPtr->doc_schema = doc_schema;

  // set start element in schema
  if ((start_element_name = get_start_element(doc_schema))) { 
    poCtxPtr->start_element_name = start_element_name;
  } else {
    alert("Failed to find start element in schema.");
    return NULL;    
  }
 
  // validate the schema to make sure it conforms to packedobjects schema
  if (xml_validate_schema_rules(doc_schema)) {
    return NULL;
  }

  // validate the schema to make sure repeating sequences conform to packedobjects schema
  if (xml_validate_schema_sequence(doc_schema)) {
    return NULL;
  }  
  
  // setup validation context
  if ((schemap = xml_compile_schema(doc_schema)) == NULL) {
    alert("Failed to preprocess schema.");
    return NULL;
  }

  poCtxPtr->schemap = schemap;


  // create user defined types hash table
  udt = xmlHashCreate(100);
  poCtxPtr->udt = udt;
  hash_user_defined_types(poCtxPtr);
  
  // create expanded schema without user defined types
  doc_expanded_schema = expand_user_defined_types(poCtxPtr);
#ifdef DEBUG_MODE
  packedobjects_dump_doc_to_file("/tmp/expand.xml", doc_expanded_schema);
#endif
  poCtxPtr->doc_expanded_schema = doc_expanded_schema;
  
  // create the canonical schema we will use for encoding/decoding
  doc_canonical_schema = packedobjects_make_canonical_schema(poCtxPtr);
#ifdef DEBUG_MODE
  packedobjects_dump_doc_to_file("/tmp/canon.xml", doc_canonical_schema);
#endif  
  poCtxPtr->doc_canonical_schema = doc_canonical_schema;

  // setup xpath
  xpathp = xmlXPathNewContext(poCtxPtr->doc_canonical_schema);
  if (xpathp == NULL) {
    alert("Error in xmlXPathNewContext.");
    return NULL;
  }

  if(xmlXPathRegisterNs(xpathp, (const xmlChar *)NSPREFIX, (const xmlChar *)NSURL) != 0) {
    alert("Error: unable to register NS.");
    return NULL;
  }

  poCtxPtr->xpathp = xpathp;


  // allocate buffer for PDU
  if ((pdu = malloc(MAX_PDU)) == NULL) {
    alert("Failed to allocate PDU buffer.");
    return NULL;
  }
  // setup encode structure
  if ((encodep = initializeEncode(pdu, MAX_PDU)) == NULL) {
    alert("Failed to initialise encoder.");
    return NULL;    
  }
  poCtxPtr->encodep = encodep;

  // set some defaults
  poCtxPtr->bytes = 0;
  poCtxPtr->encode_error = 0;
  poCtxPtr->decode_error = 0;

  
  return poCtxPtr;
}

void free_packedobjects(packedobjectsContext *poCtxPtr)
{
  xml_free_schema(poCtxPtr->schemap);
  xmlFreeDoc(poCtxPtr->doc_schema);
  xmlFreeDoc(poCtxPtr->doc_expanded_schema);
  xmlFreeDoc(poCtxPtr->doc_canonical_schema);
  // contents in hash table should be freed already
  xmlHashFree(poCtxPtr->udt, NULL);
  xmlXPathFreeContext(poCtxPtr->xpathp);
  xmlFree(BAD_CAST poCtxPtr->start_element_name);
  // we created the pdu in our init function
  free(poCtxPtr->encodep->pdu);
  freeEncode(poCtxPtr->encodep);
  free(poCtxPtr);

  xmlCleanupParser();
}



