#include <stdio.h>
#include "packedobjects.h"
#include "canon.h"
#include "schema.h"
#include "ier.h"

#ifdef DEBUG_MODE
#define dbg(fmtstr, args...) \
  (printf(PROGNAME ":%s: " fmtstr "\n", __func__, ##args))
#else
#define dbg(dummy...)
#endif

#define CHECK_BIT(var,pos) ((var) & (1<<(pos)))

enum STRING_TYPES { STRING, BIT_STRING, NUMERIC_STRING, HEX_STRING, OCTET_STRING };



static void traverse_doc_data(packedobjectsContext *pc, xmlNode *node);
static xmlNodePtr query_schema(packedobjectsContext *pc, xmlChar *xpath);
static xmlChar *make_schema_query(xmlNodePtr node);
static xmlChar *get_start_element(xmlDocPtr doc);
static void encode_node(packedobjectsContext *pc, xmlNodePtr data_node, xmlNodePtr schema_node);
static void encode_integer(packedobjectsContext *pc, xmlNodePtr data_node, xmlNodePtr schema_node);
static void encode_string(packedobjectsContext *pc, xmlNodePtr data_node, xmlNodePtr schema_node, int type);
static void encode_sequence(packedobjectsContext *pc, xmlNodePtr data_node, xmlNodePtr schema_node);
static void encode_sequence_of(packedobjectsContext *pc, xmlNodePtr data_node, xmlNodePtr schema_node);
static void encode_sequence_optional(packedobjectsContext *pc, xmlNodePtr data_node, xmlNodePtr schema_node);
static void encode_semi_constrained_string(packedobjectsContext *pc, xmlNodePtr data_node, xmlNodePtr schema_node, int type);
static void encode_constrained_string(packedobjectsContext *pc, xmlNodePtr data_node, xmlNodePtr schema_node, int type);
static void encode_fixed_length_string(packedobjectsContext *pc, xmlNodePtr data_node, xmlNodePtr schema_node, int type);
static void encode_unconstrained_integer(packedobjectsContext *pc, xmlNodePtr data_node, xmlNodePtr schema_node);
static void encode_semi_constrained_integer(packedobjectsContext *pc, xmlNodePtr data_node, xmlNodePtr schema_node);
static void encode_constrained_integer(packedobjectsContext *pc, xmlNodePtr data_node, xmlNodePtr schema_node);
static void encode_null(packedobjectsContext *pc, xmlNodePtr data_node, xmlNodePtr schema_node);
static void encode_boolean(packedobjectsContext *pc, xmlNodePtr data_node, xmlNodePtr schema_node);
static void encode_choice(packedobjectsContext *pc, xmlNodePtr data_node, xmlNodePtr schema_node);
static void encode_enumerated(packedobjectsContext *pc, xmlNodePtr data_node, xmlNodePtr schema_node);
static void encode_currency(packedobjectsContext *pc, xmlNodePtr data_node, xmlNodePtr schema_node);
static void encode_ipv4address(packedobjectsContext *pc, xmlNodePtr data_node, xmlNodePtr schema_node);


static void decode_next(packedobjectsContext *pc, xmlNodePtr data_node, xmlNodePtr schema_node);
static void decode_node(packedobjectsContext *pc, xmlNodePtr data_node, xmlNodePtr schema_node);
static void decode_boolean(packedobjectsContext *pc, xmlNodePtr data_node, xmlNodePtr schema_node);
static void decode_sequence(packedobjectsContext *pc, xmlNodePtr data_node, xmlNodePtr schema_node);
static void decode_sequence_of(packedobjectsContext *pc, xmlNodePtr data_node, xmlNodePtr schema_node);
static void decode_sequence_optional(packedobjectsContext *pc, xmlNodePtr data_node, xmlNodePtr schema_node);
static void decode_integer(packedobjectsContext *pc, xmlNodePtr data_node, xmlNodePtr schema_node);
static void decode_unconstrained_integer(packedobjectsContext *pc, xmlNodePtr data_node, xmlNodePtr schema_node);
static void decode_semi_constrained_integer(packedobjectsContext *pc, xmlNodePtr data_node, xmlNodePtr schema_node);
static void decode_constrained_integer(packedobjectsContext *pc, xmlNodePtr data_node, xmlNodePtr schema_node);
static void decode_string(packedobjectsContext *pc, xmlNodePtr data_node, xmlNodePtr schema_node, int type);
static void decode_semi_constrained_string(packedobjectsContext *pc, xmlNodePtr data_node, xmlNodePtr schema_node, int type);
static void decode_constrained_string(packedobjectsContext *pc, xmlNodePtr data_node, xmlNodePtr schema_node, int type);
static void decode_fixed_length_string(packedobjectsContext *pc, xmlNodePtr data_node, xmlNodePtr schema_node, int type);
static void decode_decimal(packedobjectsContext *pc, xmlNodePtr data_node, xmlNodePtr schema_node);
static void decode_null(packedobjectsContext *pc, xmlNodePtr data_node, xmlNodePtr schema_node);
static void decode_currency(packedobjectsContext *pc, xmlNodePtr data_node, xmlNodePtr schema_node);
static void decode_enumerated(packedobjectsContext *pc, xmlNodePtr data_node, xmlNodePtr schema_node);
static void decode_choice(packedobjectsContext *pc, xmlNodePtr data_node, xmlNodePtr schema_node);
static void decode_ipv4address(packedobjectsContext *pc, xmlNodePtr data_node, xmlNodePtr schema_node);

packedobjectsContext *init_packedobjects(const char *schema_file)
{
  xmlDoc *doc_schema = NULL, *doc_canonical_schema = NULL;
  schemaData *schemap = NULL;
  packedobjectsContext *poCtxPtr = NULL;
  xmlXPathContextPtr xpathp = NULL;
  xmlHashTablePtr udt = NULL;
  xmlChar *start_element_name = NULL;
  char *pdu = NULL;
  packedEncode *encodep = NULL;
  
  if ((poCtxPtr = (packedobjectsContext *)malloc(sizeof(packedobjectsContext))) == NULL) {
    fprintf(stderr, "Could not alllocate memory.\n");
    exit(1);    
  }
  
  if ((doc_schema = packedobjects_new_doc(schema_file)) == NULL) {
    return NULL;
  }

  // supplied at encode
  poCtxPtr->doc_data = NULL;
  poCtxPtr->doc_schema = doc_schema;

  // set start element in schema
  if ((start_element_name = get_start_element(doc_schema))) { 
    poCtxPtr->start_element_name = start_element_name;
  } else {
    fprintf(stderr,"Failed to find start element in schema\n");
    return NULL;    
  }
  
  // setup validation context
  if ((schemap = xml_compile_schema(doc_schema)) == NULL) {
    return NULL;
  }

  poCtxPtr->schemap = schemap;


  // create user defined types hash table
  udt = xmlHashCreate(100);
  poCtxPtr->udt = udt;

  // create the canonical schema we will use for encoding/decoding
  doc_canonical_schema = packedobjects_make_canonical_schema(poCtxPtr);
  poCtxPtr->doc_canonical_schema = doc_canonical_schema;

  // setup xpath
  xpathp = xmlXPathNewContext(poCtxPtr->doc_canonical_schema);
  if (xpathp == NULL) {
    printf("Error in xmlXPathNewContext\n");
    return NULL;
  }

  if(xmlXPathRegisterNs(xpathp, (const xmlChar *)NSPREFIX, (const xmlChar *)NSURL) != 0) {
    fprintf(stderr,"Error: unable to register NS\n");
    return NULL;
  }

  poCtxPtr->xpathp = xpathp;


  // allocate buffer for PDU
  pdu = malloc(MAX_PDU);
  // setup encode structure
  encodep = initializeEncode(pdu, MAX_PDU);
  poCtxPtr->encodep = encodep;
  
  return poCtxPtr;
}

void free_packedobjects(packedobjectsContext *poCtxPtr)
{
  xml_free_schema(poCtxPtr->schemap);
  xmlFreeDoc(poCtxPtr->doc_schema);
  xmlFreeDoc(poCtxPtr->doc_canonical_schema);  
  xmlXPathFreeContext(poCtxPtr->xpathp);
  xmlFree(BAD_CAST poCtxPtr->start_element_name);
  // we created the pdu in our init function
  free(poCtxPtr->encodep->pdu);
  freeEncode(poCtxPtr->encodep);
  free(poCtxPtr);

  xmlCleanupParser();
}



  

xmlDocPtr packedobjects_new_doc(const char *file)
{
  xmlDocPtr doc = NULL;

  xmlKeepBlanksDefault(0);
  doc = xmlReadFile(file, NULL, 0);
  
  if (doc == NULL) {
    fprintf(stderr, "error: could not parse file %s\n", file);
  }

  return doc;
 
}

static xmlNodePtr query_schema(packedobjectsContext *pc, xmlChar *xpath)
{
  
  
  xmlXPathObjectPtr result = NULL;
  xmlNodeSetPtr nodes = NULL;
  xmlNodePtr node = NULL;
  
  result = xmlXPathEvalExpression(xpath, pc->xpathp);

  if (result == NULL) {
    fprintf(stderr, "Error in xmlXPathEvalExpression\n");
    return NULL;
  }
  if(xmlXPathNodeSetIsEmpty(result->nodesetval)){
    xmlXPathFreeObject(result);
    fprintf(stderr, "xpath:%s failed to query schema.\n", xpath);
    return NULL;
  }

  nodes = result->nodesetval;
  if ((nodes->nodeNr) == 1) {
    node = nodes->nodeTab[0];
  } else {
    fprintf(stderr, "xpath:%s did not find 1 node only.\n", xpath);
    return NULL;    
    
  }
  xmlXPathFreeObject(result);     
  
  return node;
}

// needed to remove any [] which come from repeating sequences
static xmlChar *make_schema_query(xmlNodePtr node)
{
  xmlChar *path1 = NULL, *path2 = NULL;
  const xmlChar *sp = NULL;
  xmlChar *left_string = NULL;
  
  path1 = xmlGetNodePath(node);
  if ((sp = xmlStrchr(path1, '['))) {
    left_string = xmlStrndup(path1, sp - path1);
    sp = xmlStrchr(sp, ']');
    path2 = xmlStrncatNew(left_string, ++sp, -1);
    xmlFree(left_string);
    xmlFree(path1);
    return path2;
  } else {
    return path1;
  }
}

char *packedobjects_encode(packedobjectsContext *pc, xmlDocPtr doc)
{
  int bytes = 0;
  // validate data against schema
  packedobjects_validate(pc, doc);
  traverse_doc_data(pc, xmlDocGetRootElement(doc));
  bytes = finalizeEncode(pc->encodep);
  pc->bytes = bytes;
  return (pc->encodep->pdu);
}

static void traverse_doc_data(packedobjectsContext *pc, xmlNode *node)
{
  xmlNode *cur_node = NULL;
  xmlChar *path = NULL;
  xmlNodePtr schema_node = NULL;  
  
  for (cur_node = node; cur_node; cur_node = cur_node->next) {
    if (cur_node->type == XML_ELEMENT_NODE) {
      path = make_schema_query(cur_node);
      if ((schema_node = query_schema(pc, path))) {
        dbg("path:%s", path);
        encode_node(pc, cur_node, schema_node);
      }
      free(path); 
    }
    traverse_doc_data(pc, cur_node->children);
  }
}

static void encode_unconstrained_integer(packedobjectsContext *pc, xmlNodePtr data_node, xmlNodePtr schema_node)
{
  xmlChar *value = NULL;
  signed long int n = 0;
  
  value = xmlNodeListGetString(pc->doc_data, data_node->xmlChildrenNode, 1);
  n = atoi((const char *) value);
  encodeUnconstrainedInteger(pc->encodep, n);
  xmlFree(value);  

}

static void encode_semi_constrained_integer(packedobjectsContext *pc, xmlNodePtr data_node, xmlNodePtr schema_node)
{
  xmlChar *value = NULL;
  xmlChar *minInclusive = NULL;
  int lb;
  signed long int n = 0;
  
  value = xmlNodeListGetString(pc->doc_data, data_node->xmlChildrenNode, 1);
  n = atoi((const char *) value);
  dbg("n:%ld", n);
  minInclusive = xmlGetProp(schema_node, BAD_CAST "minInclusive");
  lb = atoi((const char *) minInclusive);  
  encodeUnsignedSemiConstrainedInteger(pc->encodep, n, lb);
  xmlFree(minInclusive);
  xmlFree(value);  

}

static void encode_constrained_integer(packedobjectsContext *pc, xmlNodePtr data_node, xmlNodePtr schema_node)
{
  xmlChar *value = NULL;
  xmlChar *minInclusive = NULL;
  xmlChar *maxInclusive = NULL;
  int lb, ub;
  signed long int n = 0;
  
  value = xmlNodeListGetString(pc->doc_data, data_node->xmlChildrenNode, 1);
  n = atoi((const char *) value);
  minInclusive = xmlGetProp(schema_node, BAD_CAST "minInclusive");
  lb = atoi((const char *) minInclusive);
  maxInclusive = xmlGetProp(schema_node, BAD_CAST "maxInclusive");
  ub = atoi((const char *) maxInclusive);  
  encodeUnsignedConstrainedInteger(pc->encodep, n, lb, ub);
  xmlFree(minInclusive);
  xmlFree(maxInclusive);
  xmlFree(value);  

}

static void encode_integer(packedobjectsContext *pc, xmlNodePtr data_node, xmlNodePtr schema_node)
{

  xmlChar *variant = NULL;

  variant = xmlGetProp(schema_node, BAD_CAST "variant");
  if (xmlStrEqual(variant, BAD_CAST "unconstrained")) {
    encode_unconstrained_integer(pc, data_node, schema_node);
  } else if (xmlStrEqual(variant, BAD_CAST "semi-constrained")) {
    encode_semi_constrained_integer(pc, data_node, schema_node);
  } else if (xmlStrEqual(variant, BAD_CAST "constrained")) {
    encode_constrained_integer(pc, data_node, schema_node);    
  } else {
    fprintf(stderr, "Found an integer variant I can't encode.\n");
    // need to set something in pc    
  }
  xmlFree(variant);  

}

static void encode_semi_constrained_string(packedobjectsContext *pc, xmlNodePtr data_node, xmlNodePtr schema_node, int type)
{

  xmlChar *value = NULL;
  
  value = xmlNodeListGetString(pc->doc_data, data_node->xmlChildrenNode, 1);
  
  switch(type) {
  case STRING:
    encodeSemiConstrainedString(pc->encodep, (char *)value);
    break;
  case BIT_STRING:
    encodeSemiConstrainedBitString(pc->encodep, (char *)value);
    break;
  case NUMERIC_STRING:
    encodeSemiConstrainedNumericString(pc->encodep, (char *)value);
    break;
  case HEX_STRING:
    encodeSemiConstrainedHexString(pc->encodep, (char *)value);
    break;
  case OCTET_STRING:
    encodeSemiConstrainedOctetString(pc->encodep, (char *)value);
    break;  
  }
  
  xmlFree(value);  

}

static void encode_constrained_string(packedobjectsContext *pc, xmlNodePtr data_node, xmlNodePtr schema_node, int type)
{
  xmlChar *value = NULL;
  xmlChar *minLength = NULL;
  xmlChar *maxLength = NULL;
  int lb, ub;
  
  minLength = xmlGetProp(schema_node, BAD_CAST "minLength");
  maxLength = xmlGetProp(schema_node, BAD_CAST "maxLength");
  lb = atoi((const char *) minLength);
  ub = atoi((const char *) maxLength);
  value = xmlNodeListGetString(pc->doc_data, data_node->xmlChildrenNode, 1);
  switch(type) {
  case STRING:
    encodeConstrainedString(pc->encodep, (char *)value, lb, ub);
    break;
  case BIT_STRING:
    encodeConstrainedBitString(pc->encodep, (char *)value, lb, ub);
    break;
  case NUMERIC_STRING:
    encodeConstrainedNumericString(pc->encodep, (char *)value, lb, ub);
    break;
  case HEX_STRING:
    encodeConstrainedHexString(pc->encodep, (char *)value, lb, ub);
    break;
  case OCTET_STRING:
    encodeConstrainedOctetString(pc->encodep, (char *)value, lb, ub);
    break;  
  }
  
  xmlFree(minLength);
  xmlFree(maxLength);
  xmlFree(value);
}

static void encode_fixed_length_string(packedobjectsContext *pc, xmlNodePtr data_node, xmlNodePtr schema_node, int type)
{
  xmlChar *value = NULL;
  xmlChar *length = NULL;
  int len;
  
  length = xmlGetProp(schema_node, BAD_CAST "length");
  len = atoi((const char *) length);
  value = xmlNodeListGetString(pc->doc_data, data_node->xmlChildrenNode, 1);
  switch(type) {
  case STRING:
    encodeFixedLengthString(pc->encodep, (char *)value, len);
    break;
  case BIT_STRING:
    encodeFixedLengthBitString(pc->encodep, (char *)value, len);
    break;
  case NUMERIC_STRING:
    encodeFixedLengthNumericString(pc->encodep, (char *)value, len);
    break;
  case HEX_STRING:
    encodeFixedLengthHexString(pc->encodep, (char *)value, len);
    break;
  case OCTET_STRING:
    encodeFixedLengthOctetString(pc->encodep, (char *)value, len);
    break;  
  }
  
  xmlFree(length);
  xmlFree(value);

}

static void encode_string(packedobjectsContext *pc, xmlNodePtr data_node, xmlNodePtr schema_node, int type)
{
  xmlChar *variant = NULL;

  variant = xmlGetProp(schema_node, BAD_CAST "variant");
  if (xmlStrEqual(variant, BAD_CAST "semi-constrained")) {
    encode_semi_constrained_string(pc, data_node, schema_node, type);
  } else if (xmlStrEqual(variant, BAD_CAST "constrained")) {
    encode_constrained_string(pc, data_node, schema_node, type);
  } else if (xmlStrEqual(variant, BAD_CAST "fixed-length")) {
    encode_fixed_length_string(pc, data_node, schema_node, type);
  } else {
    fprintf(stderr, "Found a string variant I can't encode.\n");
    // need to set something in pc    
  }
  xmlFree(variant);
}
                                                                                               
static void encode_sequence(packedobjectsContext *pc, xmlNodePtr data_node, xmlNodePtr schema_node)
{
  // don't need to encode anything
}

static void encode_sequence_of(packedobjectsContext *pc, xmlNodePtr data_node, xmlNodePtr schema_node)
{
  xmlChar *items = NULL;
  unsigned long n = 0;

  // number of items in the sequence according to the schema
  items = xmlGetProp(schema_node, BAD_CAST "items");
  n = atoi((const char *) items);
  // work out how many times data repeats
  n = xmlChildElementCount(data_node) / n;
  dbg("sequence_of len:%d", n);
  encodeSequenceOfLength(pc->encodep, n);
  xmlFree(items);

}

static void encode_sequence_optional(packedobjectsContext *pc, xmlNodePtr data_node, xmlNodePtr schema_node)
{
  xmlNodePtr dnp = NULL;
  xmlNodePtr snp = NULL;
  int bit = 0;
  unsigned long bitmap = 0;
  xmlChar *items = NULL;
  unsigned long n = 0;
  
  // number of items in the sequence according to the schema
  items = xmlGetProp(schema_node, BAD_CAST "items");
  n = atoi((const char *) items);
  xmlFree(items);
  
  dnp = data_node->children;
  snp = schema_node->children;
  for (; dnp; dnp = dnp->next) {
    while (snp) {
      if (xmlStrEqual(dnp->name, snp->name)) {
        bitmap = bitmap ^ (1UL << bit);
        break;
      }
      bit++;
      snp = snp->next;
    }
  }
  dbg("bitmap:%lu within %lu bits", bitmap, n);
  encodeBitmap(pc->encodep, bitmap, n);
  
}

static void encode_choice(packedobjectsContext *pc, xmlNodePtr data_node, xmlNodePtr schema_node)
{
  xmlNodePtr snp = NULL;
  int index = 1;
  xmlChar *items = NULL;
  unsigned long n = 0;
  
  // number of items to choose from according to the schema
  items = xmlGetProp(schema_node, BAD_CAST "items");
  n = atoi((const char *) items);
  xmlFree(items);
  snp = schema_node->children;
  while (snp) {
    dbg("snp->name:%s", snp->name);
    if (xmlStrEqual(data_node->children->name, snp->name)) break;
    index++;
    snp = snp->next;
  }
  dbg("choice index:%d", index);
  encodeChoiceIndex(pc->encodep, index, n);
  
}

static void encode_enumerated(packedobjectsContext *pc, xmlNodePtr data_node, xmlNodePtr schema_node)
{
  xmlNodePtr snp = NULL;
  unsigned index = 0;
  xmlChar *items = NULL;
  unsigned long n = 0;
  xmlAttrPtr attr = NULL;
  xmlChar *data_value = NULL;
  xmlChar *schema_value = NULL;
  
  // number of items to choose from according to the schema
  items = xmlGetProp(schema_node, BAD_CAST "items");
  n = atoi((const char *) items);
  xmlFree(items);

  data_value = xmlNodeListGetString(pc->doc_data, data_node->children, 1);
  
  snp = schema_node;
  for(attr = snp->properties; NULL != attr; attr = attr->next) {
    if (xmlStrEqual(attr->name, BAD_CAST "enumeration")) {
      schema_value = xmlNodeListGetString(pc->doc_canonical_schema, attr->children, 1);
      if (xmlStrEqual(data_value, schema_value)) break;  
      xmlFree(schema_value);
      index++;
    }
  }
  xmlFree(data_value);
  
  dbg("enumerated index:%d from %lu items", index, n);
  encodeEnumerated(pc->encodep, index, n);
  
}

static void encode_null(packedobjectsContext *pc, xmlNodePtr data_node, xmlNodePtr schema_node)
{
  // don't need to encode anything
}


static void encode_boolean(packedobjectsContext *pc, xmlNodePtr data_node, xmlNodePtr schema_node)
{
  xmlChar *value = NULL;
  
  value = xmlNodeListGetString(pc->doc_data, data_node->xmlChildrenNode, 1);
  if ((xmlStrEqual(value, BAD_CAST "0")) || (xmlStrEqual(value, BAD_CAST "false"))) {
    dbg("boolean: 0");
    encodeBoolean(pc->encodep, 0);
  } else {
    // everything else is true
    dbg("boolean: 1");
    encodeBoolean(pc->encodep, 1);
  }
  xmlFree(value);  

}

static void encode_decimal(packedobjectsContext *pc, xmlNodePtr data_node, xmlNodePtr schema_node)
{
  xmlChar *value = NULL;
  
  
  value = xmlNodeListGetString(pc->doc_data, data_node->xmlChildrenNode, 1);
  dbg("value:%s", value);
  // encode in 4 bits per char
  encodeDecimal(pc->encodep, (char *)value);
  xmlFree(value);
  
}

static void encode_currency(packedobjectsContext *pc, xmlNodePtr data_node, xmlNodePtr schema_node)
{
  xmlChar *value = NULL;
  
  value = xmlNodeListGetString(pc->doc_data, data_node->xmlChildrenNode, 1);
  dbg("value:%s", value);

  encodeCurrency(pc->encodep, (char *)value);
  xmlFree(value);
  
}

static void encode_ipv4address(packedobjectsContext *pc, xmlNodePtr data_node, xmlNodePtr schema_node)
{
  xmlChar *value = NULL;
  
  value = xmlNodeListGetString(pc->doc_data, data_node->xmlChildrenNode, 1);
  dbg("value:%s", value);

  encodeIPv4Address(pc->encodep, (char *)value);
  xmlFree(value);
  
}


static void encode_node(packedobjectsContext *pc, xmlNodePtr data_node, xmlNodePtr schema_node)
{

  xmlChar *type = NULL;

  type = xmlGetProp(schema_node, BAD_CAST "type");
  if (xmlStrEqual(type, BAD_CAST "integer")) {
    encode_integer(pc, data_node, schema_node);
  } else if (xmlStrEqual(type, BAD_CAST "string")) {
    encode_string(pc, data_node, schema_node, STRING);
  } else if (xmlStrEqual(type, BAD_CAST "bit-string")) {
    encode_string(pc, data_node, schema_node, BIT_STRING);
  } else if (xmlStrEqual(type, BAD_CAST "numeric-string")) {
    encode_string(pc, data_node, schema_node, NUMERIC_STRING);
  } else if (xmlStrEqual(type, BAD_CAST "hex-string")) {
    encode_string(pc, data_node, schema_node, HEX_STRING);
  } else if (xmlStrEqual(type, BAD_CAST "octet-string")) {
    encode_string(pc, data_node, schema_node, OCTET_STRING);
  } else if (xmlStrEqual(type, BAD_CAST "sequence")) {
    encode_sequence(pc, data_node, schema_node);
  } else if (xmlStrEqual(type, BAD_CAST "sequence-of")) {
    encode_sequence_of(pc, data_node, schema_node);
  } else if (xmlStrEqual(type, BAD_CAST "sequence-optional")) {
    encode_sequence_optional(pc, data_node, schema_node);
  } else if (xmlStrEqual(type, BAD_CAST "null")) {
    encode_null(pc, data_node, schema_node);
  } else if (xmlStrEqual(type, BAD_CAST "boolean")) {
    encode_boolean(pc, data_node, schema_node);
  } else if (xmlStrEqual(type, BAD_CAST "choice")) {
    encode_choice(pc, data_node, schema_node);
  } else if (xmlStrEqual(type, BAD_CAST "enumerated")) {
    encode_enumerated(pc, data_node, schema_node);
  } else if (xmlStrEqual(type, BAD_CAST "currency")) {
    encode_currency(pc, data_node, schema_node);    
  } else if (xmlStrEqual(type, BAD_CAST "decimal")) {
    encode_decimal(pc, data_node, schema_node);
  } else if (xmlStrEqual(type, BAD_CAST "ipv4-address")) {
    encode_ipv4address(pc, data_node, schema_node);    
  } else {
    fprintf(stderr, "Found a type I can't encode.\n");
    // need to set something in pc
  }
  xmlFree(type);

}



void packedobjects_validate(packedobjectsContext *poCtxPtr, xmlDocPtr doc)
{
  int result;
  schemaData *schemap = poCtxPtr->schemap;
  
  result = xmlSchemaValidateDoc(schemap->validCtxt, doc);
  if (result) {
    fprintf(stderr, "Failed to validate XSD schema.\n");
    exit(1);
  }  



}



void packedobjects_dump_doc(xmlDoc *doc)
{
  xmlSaveFormatFileEnc("-", doc, "UTF-8", 1);
}


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


static void decode_boolean(packedobjectsContext *pc, xmlNodePtr data_node, xmlNodePtr schema_node)
{
  int result;

  result = decodeBoolean(pc->decodep);
  dbg("boolean:%d", result);

  
  if (result) {
    xmlNewChild(data_node, NULL, schema_node->name, BAD_CAST "true");    
  } else {
    xmlNewChild(data_node, NULL, schema_node->name, BAD_CAST "false");
  }

  decode_next(pc, data_node, schema_node->children); 
}

static void decode_null(packedobjectsContext *pc, xmlNodePtr data_node, xmlNodePtr schema_node)
{

  dbg("null");

  xmlNewChild(data_node, NULL, schema_node->name, NULL);    

  decode_next(pc, data_node, schema_node->children); 
}


static void decode_sequence(packedobjectsContext *pc, xmlNodePtr data_node, xmlNodePtr schema_node)
{
  xmlNodePtr np = NULL;
  
  np = xmlAddChild(data_node, xmlCopyNode(schema_node, 0));
  decode_next(pc, np, schema_node->children); 
  
}

static void decode_sequence_of(packedobjectsContext *pc, xmlNodePtr data_node, xmlNodePtr schema_node)
{
  xmlNodePtr np = NULL;
  int i, len;
  len = decodeSequenceOfLength(pc->decodep);
  dbg("sequence_of len:%d", len);
  np = xmlAddChild(data_node, xmlCopyNode(schema_node, 0));
  for (i=0; i<len; i++) {
    decode_next(pc, np, schema_node->children); 
  }
  
}

static void decode_sequence_optional(packedobjectsContext *pc, xmlNodePtr data_node, xmlNodePtr schema_node)
{
  xmlNodePtr dp = NULL;
  xmlNodePtr sp = NULL;
  char *items;
  unsigned long int n;
  unsigned long int bitmap;
  int i;
  
  items = xmlGetProp(schema_node, BAD_CAST "items");
  n = atoi((const char *) items);
  dbg("n:%d", n);
  bitmap = decodeBitmap(pc->decodep, n);
  xmlFree(items);
  dbg("bitmap:%lu", bitmap);
  dp = xmlAddChild(data_node, xmlCopyNode(schema_node, 0));
  sp = schema_node->children;
  for (i=0; i<n; i++) {
    if (CHECK_BIT(bitmap, i)) {
      decode_node(pc, dp, sp);
    }
    sp = sp->next;
  }  
  
}

static void decode_unconstrained_integer(packedobjectsContext *pc, xmlNodePtr data_node, xmlNodePtr schema_node)
{

  signed long int n;
  char value[11];
  
  n = decodeUnconstrainedInteger(pc->decodep);
  dbg("n:%ld", n);
  sprintf(value, "%ld", n);
  xmlNewChild(data_node, NULL, schema_node->name, BAD_CAST value);    

  decode_next(pc, data_node, schema_node->children);   

}

static void decode_semi_constrained_integer(packedobjectsContext *pc, xmlNodePtr data_node, xmlNodePtr schema_node)
{

  signed long int n;
  char value[11];
  xmlChar *minInclusive = NULL;
  int lb;

  minInclusive = xmlGetProp(schema_node, BAD_CAST "minInclusive");
  lb = atoi((const char *) minInclusive);
  n = decodeUnsignedSemiConstrainedInteger(pc->decodep, lb);
  dbg("n:%ld", n);
  sprintf(value, "%ld", n);
  xmlNewChild(data_node, NULL, schema_node->name, BAD_CAST value);    

  xmlFree(minInclusive);
  
  decode_next(pc, data_node, schema_node->children);   
  
  
}

static void decode_constrained_integer(packedobjectsContext *pc, xmlNodePtr data_node, xmlNodePtr schema_node)
{

  signed long int n;
  char value[11];
  xmlChar *minInclusive = NULL;
  xmlChar *maxInclusive = NULL;
  int lb, ub;

  minInclusive = xmlGetProp(schema_node, BAD_CAST "minInclusive");
  lb = atoi((const char *) minInclusive);
  maxInclusive = xmlGetProp(schema_node, BAD_CAST "maxInclusive");
  ub = atoi((const char *) maxInclusive); 
  n = decodeUnsignedConstrainedInteger(pc->decodep, lb, ub);
  dbg("n:%ld", n);
  sprintf(value, "%ld", n);
  xmlNewChild(data_node, NULL, schema_node->name, BAD_CAST value);    

  xmlFree(minInclusive);
  xmlFree(maxInclusive);
  
  decode_next(pc, data_node, schema_node->children);   
  
}

static void decode_integer(packedobjectsContext *pc, xmlNodePtr data_node, xmlNodePtr schema_node)
{
  xmlChar *variant = NULL;

  variant = xmlGetProp(schema_node, BAD_CAST "variant");
  if (xmlStrEqual(variant, BAD_CAST "unconstrained")) {
    decode_unconstrained_integer(pc, data_node, schema_node);
  } else if (xmlStrEqual(variant, BAD_CAST "semi-constrained")) {
    decode_semi_constrained_integer(pc, data_node, schema_node);
  } else if (xmlStrEqual(variant, BAD_CAST "constrained")) {
    decode_constrained_integer(pc, data_node, schema_node);    
  } else {
    fprintf(stderr, "Found an integer variant I can't encode.\n");
    // need to set something in pc    
  }
  xmlFree(variant); 
}


static void decode_semi_constrained_string(packedobjectsContext *pc, xmlNodePtr data_node, xmlNodePtr schema_node, int type)
{

  char *value = NULL;
  
  switch(type) {
  case STRING:
    value = decodeSemiConstrainedString(pc->decodep);
    break;
  case BIT_STRING:
    value = decodeSemiConstrainedBitString(pc->decodep);
    break;
  case NUMERIC_STRING:
    value = decodeSemiConstrainedNumericString(pc->decodep);
    break;
  case HEX_STRING:
    value = decodeSemiConstrainedHexString(pc->decodep);
    break;
  case OCTET_STRING:
    value = decodeSemiConstrainedOctetString(pc->decodep);
    break;  
  }

  xmlNewChild(data_node, NULL, schema_node->name, BAD_CAST value);    
  free(value);
  
}

  
static void decode_constrained_string(packedobjectsContext *pc, xmlNodePtr data_node, xmlNodePtr schema_node, int type)
{

  char *value = NULL;
  xmlChar *minLength = NULL;
  xmlChar *maxLength = NULL;
  int lb, ub;
  
  minLength = xmlGetProp(schema_node, BAD_CAST "minLength");
  maxLength = xmlGetProp(schema_node, BAD_CAST "maxLength");
  lb = atoi((const char *) minLength);
  ub = atoi((const char *) maxLength);
  
  dbg("lb:%d, ub:%d", lb, ub);
  
  switch(type) {
  case STRING:
    value = decodeConstrainedString(pc->decodep, lb, ub);
    break;
  case BIT_STRING:
    value = decodeConstrainedBitString(pc->decodep, lb, ub);
    break;
  case NUMERIC_STRING:
    value = decodeConstrainedNumericString(pc->decodep, lb, ub);
    break;
  case HEX_STRING:
    value = decodeConstrainedHexString(pc->decodep, lb, ub);
    break;
  case OCTET_STRING:
    value = decodeConstrainedOctetString(pc->decodep, lb, ub);
    break;  
  }

  xmlNewChild(data_node, NULL, schema_node->name, BAD_CAST value);
  xmlFree(minLength);
  xmlFree(maxLength);
  free(value);
  
}

static void decode_fixed_length_string(packedobjectsContext *pc, xmlNodePtr data_node, xmlNodePtr schema_node, int type)
{

  char *value = NULL;
  xmlChar *length = NULL;
  int len;
  
  length = xmlGetProp(schema_node, BAD_CAST "length");
  len = atoi((const char *) length);

  dbg("len:%d", len);
  
  switch(type) {
  case STRING:
    value = decodeFixedLengthString(pc->decodep, len);
    break;
  case BIT_STRING:
    value = decodeFixedLengthString(pc->decodep, len);
    break;
  case NUMERIC_STRING:
    value = decodeFixedLengthString(pc->decodep, len);
    break;
  case HEX_STRING:
    value = decodeFixedLengthString(pc->decodep, len);
    break;
  case OCTET_STRING:
    value = decodeFixedLengthString(pc->decodep, len);
    break;  
  }

  xmlNewChild(data_node, NULL, schema_node->name, BAD_CAST value);    
  xmlFree(length);
  free(value);
  
}

static void decode_string(packedobjectsContext *pc, xmlNodePtr data_node, xmlNodePtr schema_node, int type)
{

  xmlChar *variant = NULL;

  variant = xmlGetProp(schema_node, BAD_CAST "variant");
  if (xmlStrEqual(variant, BAD_CAST "semi-constrained")) {
    decode_semi_constrained_string(pc, data_node, schema_node, type);
  } else if (xmlStrEqual(variant, BAD_CAST "constrained")) {
    decode_constrained_string(pc, data_node, schema_node, type);
  } else if (xmlStrEqual(variant, BAD_CAST "fixed-length")) {
    decode_fixed_length_string(pc, data_node, schema_node, type);
  } else {
    fprintf(stderr, "Found a string variant I can't encode.\n");
    // need to set something in pc    
  }
  xmlFree(variant);  

}


static void decode_decimal(packedobjectsContext *pc, xmlNodePtr data_node, xmlNodePtr schema_node)
{

  xmlChar *value = NULL;

  value = BAD_CAST decodeDecimal(pc->decodep);
  xmlNewChild(data_node, NULL, schema_node->name, value);    
  xmlFree(value);  
  
}

static void decode_currency(packedobjectsContext *pc, xmlNodePtr data_node, xmlNodePtr schema_node)
{

  xmlChar *value = NULL;

  value = BAD_CAST decodeCurrency(pc->decodep);
  xmlNewChild(data_node, NULL, schema_node->name, value);  
  xmlFree(value);
}

static void decode_ipv4address(packedobjectsContext *pc, xmlNodePtr data_node, xmlNodePtr schema_node)
{

  xmlChar *value = NULL;

  value = BAD_CAST decodeIPv4Address(pc->decodep);
  xmlNewChild(data_node, NULL, schema_node->name, value);  
  xmlFree(value);
}

static void decode_enumerated(packedobjectsContext *pc, xmlNodePtr data_node, xmlNodePtr schema_node)
{

  xmlChar *value = NULL;
  xmlChar *items = NULL;
  xmlNodePtr snp = NULL;
  xmlAttrPtr attr = NULL;
  unsigned long int n = 0;
  int i = 0, index = 0;
  
  items = xmlGetProp(schema_node, BAD_CAST "items");
  n = atoi((const char *) items);
  xmlFree(items);
  dbg("n:%lu", n);
  index = decodeEnumerated(pc->decodep, n);
  dbg("index:%d", index);
  snp = schema_node;
  for(attr = snp->properties; NULL != attr; attr = attr->next) {
    if (xmlStrEqual(attr->name, BAD_CAST "enumeration")) {
      if (i == index) {
        value = xmlNodeListGetString(pc->doc_canonical_schema, attr->children, 1);
        xmlNewChild(data_node, NULL, schema_node->name, value);  
        xmlFree(value);
        break;
      }
      i++;
    }
  }
}

static void decode_choice(packedobjectsContext *pc, xmlNodePtr data_node, xmlNodePtr schema_node)
{

  xmlChar *items = NULL;
  xmlNodePtr sp = NULL;
  xmlNodePtr dp = NULL;
  unsigned long int n = 0;
  int i = 0, index = 0;
  
  items = xmlGetProp(schema_node, BAD_CAST "items");
  n = atoi((const char *) items);
  dbg("items:%s", items);
  xmlFree(items);
  index = decodeChoiceIndex(pc->decodep, n);
  dbg("index:%d", index);

  dp = xmlAddChild(data_node, xmlCopyNode(schema_node, 0));
  sp = schema_node->children;
  while (sp) {
    i++;
    dbg("snp->name:%s", sp->name);
    if (i == index) {
      decode_node(pc, dp, sp);
      break;
    }
    sp = sp->next;
  }
}

static void decode_next(packedobjectsContext *pc, xmlNodePtr data_node, xmlNodePtr schema_node)
{
  xmlNode *cur_node = NULL;

  for (cur_node = schema_node; cur_node; cur_node = cur_node->next) {
    if (cur_node->type == XML_ELEMENT_NODE) {
      decode_node(pc, data_node, cur_node);
    }
  }
}

static void decode_node(packedobjectsContext *pc, xmlNodePtr data_node, xmlNodePtr schema_node)
{

  xmlChar *type = NULL;
  
  type = xmlGetProp(schema_node, BAD_CAST "type");
  dbg("type:%s", type);
  
  if (xmlStrEqual(type, BAD_CAST "integer")) {
    decode_integer(pc, data_node, schema_node);
  } else if (xmlStrEqual(type, BAD_CAST "string")) {
    decode_string(pc, data_node, schema_node, STRING);
  } else if (xmlStrEqual(type, BAD_CAST "bit-string")) {
    decode_string(pc, data_node, schema_node, BIT_STRING);
  } else if (xmlStrEqual(type, BAD_CAST "numeric-string")) {
    decode_string(pc, data_node, schema_node, NUMERIC_STRING);
  } else if (xmlStrEqual(type, BAD_CAST "hex-string")) {
    decode_string(pc, data_node, schema_node, HEX_STRING);
  } else if (xmlStrEqual(type, BAD_CAST "octet-string")) {
    decode_string(pc, data_node, schema_node, OCTET_STRING);   
  } else if (xmlStrEqual(type, BAD_CAST "decimal")) {
    decode_decimal(pc, data_node, schema_node);
  } else if (xmlStrEqual(type, BAD_CAST "currency")) {
    decode_currency(pc, data_node, schema_node);
  } else if (xmlStrEqual(type, BAD_CAST "ipv4-address")) {
    decode_ipv4address(pc, data_node, schema_node);    
  } else if (xmlStrEqual(type, BAD_CAST "boolean")) {
    decode_boolean(pc, data_node, schema_node);
  } else if (xmlStrEqual(type, BAD_CAST "null")) {
    decode_null(pc, data_node, schema_node);
  } else if (xmlStrEqual(type, BAD_CAST "enumerated")) {
    decode_enumerated(pc, data_node, schema_node);    
  } else if (xmlStrEqual(type, BAD_CAST "sequence")) {
    decode_sequence(pc, data_node, schema_node);
  } else if (xmlStrEqual(type, BAD_CAST "sequence-of")) {
    decode_sequence_of(pc, data_node, schema_node);    
  } else if (xmlStrEqual(type, BAD_CAST "sequence-optional")) {
    decode_sequence_optional(pc, data_node, schema_node);
  } else if (xmlStrEqual(type, BAD_CAST "choice")) {
    decode_choice(pc, data_node, schema_node);    
  } else {
    fprintf(stderr, "Found a type I can't decode.\n");
    // need to set something in pc
  }
  xmlFree(type);

}


xmlDocPtr packedobjects_decode(packedobjectsContext *pc, char *pdu)
{
  xmlDocPtr doc_data = NULL;
  xmlNodePtr data_node = NULL;
  xmlNodePtr schema_node = NULL;
  
  pc->decodep = initializeDecode(pdu);
  pc->doc_data = doc_data;


  schema_node = xmlDocGetRootElement(pc->doc_canonical_schema);

  // add a temporary root for convenience
  data_node = xmlNewNode(NULL, BAD_CAST "root");  
  decode_node(pc, data_node, schema_node);

  dbg("creating XML data:");
  doc_data = xmlNewDoc(BAD_CAST "1.0");
  // ignore the temporary root
  xmlDocSetRootElement(doc_data, data_node->children);
  xmlFreeNode(data_node);  

  // validate data against schema
  packedobjects_validate(pc, doc_data);
  
  return doc_data;
}

