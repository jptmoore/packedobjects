#include <stdio.h>
#include <setjmp.h>
#include <string.h>

#include "packedobjects_encode.h"

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

// exception handling
jmp_buf encode_exception_env;

static void packedobjects_validate_encode(packedobjectsContext *poCtxPtr, xmlDocPtr doc);
static void traverse_doc_data(packedobjectsContext *pc, xmlNode *node);
static xmlNodePtr query_schema(packedobjectsContext *pc, xmlChar *xpath);
static void make_schema_query(char new_path[], char old_path[]);
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
static void encode_unix_time(packedobjectsContext *pc, xmlNodePtr data_node, xmlNodePtr schema_node);
static void encode_utf8_string(packedobjectsContext *pc, xmlNodePtr data_node, xmlNodePtr schema_node);

static xmlNodePtr query_schema(packedobjectsContext *pc, xmlChar *xpath)
{
  
  
  xmlXPathObjectPtr result = NULL;
  xmlNodeSetPtr nodes = NULL;
  xmlNodePtr node = NULL;
  
  result = xmlXPathEvalExpression(xpath, pc->xpathp);

  if (result == NULL) {
    alert("Error in xmlXPathEvalExpression.");
    return NULL;
  }
  if(xmlXPathNodeSetIsEmpty(result->nodesetval)){
    xmlXPathFreeObject(result);
    alert("xpath:%s failed to query schema.", xpath);
    return NULL;
  }

  nodes = result->nodesetval;
  if ((nodes->nodeNr) == 1) {
    node = nodes->nodeTab[0];
  } else {
    alert("xpath:%s did not find 1 node only.", xpath);
    return NULL;    
    
  }
  xmlXPathFreeObject(result);     
  
  return node;
}


// needed to remove any [] which come from repeating sequences
static void make_schema_query(char new_path[], char old_path[])
{
  char *p1 = old_path;
  char *p2 = new_path;
  int copying = 1;
  while (*p1 != '\0') {
    if (*p1 == '[') copying = 0;
    if (copying) {
      *p2 = *p1;
      p2++;
    }
    if (*p1 == ']') copying = 1;
    p1++;
  }
  // add null terminator
  *p2 = '\0';
}

// the real function
static char *_packedobjects_encode(packedobjectsContext *pc, xmlDocPtr doc)
{
  // default value indicates error
  pc->bytes = -1;
  // make sure we reset this on each call
  pc->encode_error = 0;
  
  // exception handler
  switch (setjmp(encode_exception_env)) {
  case ENCODE_VALIDATION_FAILED:  
    pc->encode_error = ENCODE_VALIDATION_FAILED;
    break;
  case ENCODE_PDU_BUFFER_FULL:
    pc->encode_error = ENCODE_PDU_BUFFER_FULL;
    break;
  case ENCODE_XPATH_QUERY_FAILED:
    pc->encode_error = ENCODE_XPATH_QUERY_FAILED;
    break;    
  case 0:
    if ((pc->init_options & NO_DATA_VALIDATION) == 0) {
      // validate data against schema depending on flag
      packedobjects_validate_encode(pc, doc);
    }
    traverse_doc_data(pc, xmlDocGetRootElement(doc));
    pc->bytes = finalizeEncode(pc->encodep);
  }
  
  return (pc->encodep->pdu);
}

int encode_make_memory(packedobjectsContext *pc, size_t bytes)
{
  char *pdu = NULL;
  packedEncode *encodep = NULL;

  // default PDU size from configure.ac
  if (bytes == 0) {
    pc->pdu_size = MAX_PDU;
  } else {
    pc->pdu_size = bytes;
  }
    
  // allocate buffer for PDU
  if ((pdu = malloc(pc->pdu_size)) == NULL) {
    alert("Failed to allocate PDU buffer.");
    return -1;
  }
  // setup encode structure
  if ((encodep = initializeEncode(pdu, pc->pdu_size)) == NULL) {
    alert("Failed to initialise encoder.");
    return -1;    
  }
  pc->encodep = encodep;

  return 0;
  
}

void encode_free_memory(packedobjectsContext *pc)
{
  // we created the pdu in our init function
  free(pc->encodep->pdu);
  freeEncode(pc->encodep);
}

char *packedobjects_encode(packedobjectsContext *pc, xmlDocPtr doc)
{
  char *pdu = NULL;
  size_t bytes = -1;

  // let's hope this works first time
  pdu = _packedobjects_encode(pc, doc);

  // otherwise we will keep trying by doubling the memory
  while (pc->encode_error == ENCODE_PDU_BUFFER_FULL) {
    bytes = pc->pdu_size;
    encode_free_memory(pc);
    encode_make_memory(pc, bytes*2);
    pdu = _packedobjects_encode(pc, doc);
  }

  return pdu;
}

char *packedobjects_encode_with_string(packedobjectsContext *pc, const char *xml) {

  char *pdu = NULL;
  xmlDocPtr doc = NULL;

  if ((doc = xmlParseMemory(xml, strlen(xml))) == NULL) {
    alert("Failed to parse XML string.");
    return NULL;
  }

  pdu = packedobjects_encode(pc, doc);
  if (pc->encode_error) {
    alert("Failed to encode with error %d.", pc->encode_error);
  }

  xmlFree(doc);

  return pdu;

}

void packedobjects_validate_encode(packedobjectsContext *poCtxPtr, xmlDocPtr doc)
{
  int result;
  schemaData *schemap = poCtxPtr->schemap;
  
  result = xmlSchemaValidateDoc(schemap->validCtxt, doc);
  if (result) {
    alert("Failed to validate XSD schema.");
    longjmp(encode_exception_env, ENCODE_VALIDATION_FAILED);
  }  

}

static void traverse_doc_data(packedobjectsContext *pc, xmlNode *node)
{
  xmlNode *cur_node = NULL;
  xmlChar *path = NULL;
  xmlNodePtr schema_node = NULL;
  // needs to hold the longest xpath
  char new_path[4096];
  
  for (cur_node = node; cur_node; cur_node = cur_node->next) {
    if (cur_node->type == XML_ELEMENT_NODE) {
      path = xmlGetNodePath(cur_node);
      make_schema_query(new_path, (char *)path);
      free(path);
      if ((schema_node = query_schema(pc, BAD_CAST new_path))) {
        dbg("new_path:%s", new_path);
        encode_node(pc, cur_node, schema_node);
      } else {
        longjmp(encode_exception_env, ENCODE_XPATH_QUERY_FAILED);
      }
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
    alert("Found an integer variant I can't encode.");
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
    alert("Found a string variant I can't encode.");
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
  xmlChar *minOccurs = NULL;
  xmlChar *maxOccurs = NULL;
  unsigned long n = 0;
  unsigned long lb = 0;

  minOccurs = xmlGetProp(schema_node, BAD_CAST "minOccurs");
  maxOccurs = xmlGetProp(schema_node, BAD_CAST "maxOccurs");
  // number of items in the sequence according to the schema
  items = xmlGetProp(schema_node, BAD_CAST "items");
  n = atoi((const char *) items);
  xmlFree(items);
  // work out how many times data repeats
  n = xmlChildElementCount(data_node) / n;
  dbg("sequence_of len:%d", n);
  // overide default of 0 if set
  if (minOccurs) lb = atoi((const char *) minOccurs);
  dbg("lb:%lu", lb);
  if (xmlStrEqual(maxOccurs, BAD_CAST "unbounded")) {
     // encode as semi-constrained
    encodeUnsignedSemiConstrainedInteger(pc->encodep, n, lb);
  } else {
    // encode as constrained
    unsigned long ub = atoi((const char *) maxOccurs);
    dbg("ub:%lu", ub);
    encodeUnsignedConstrainedInteger(pc->encodep, n, lb, ub);
  }
  xmlFree(minOccurs);
  xmlFree(maxOccurs);

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
      if (xmlStrEqual(data_value, schema_value)) {
        xmlFree(schema_value);
        break;
      } else {
        xmlFree(schema_value);
        index++;
      }
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

static void encode_unix_time(packedobjectsContext *pc, xmlNodePtr data_node, xmlNodePtr schema_node)
{
  xmlChar *value = NULL;
  
  value = xmlNodeListGetString(pc->doc_data, data_node->xmlChildrenNode, 1);
  dbg("value:%s", value);

  encodeUnixTime(pc->encodep, (char *)value);
  xmlFree(value);
  
}

// special kind of octet-string without length restrictions
static void encode_utf8_string(packedobjectsContext *pc, xmlNodePtr data_node, xmlNodePtr schema_node)
{
  xmlChar *value = NULL;
  
  value = xmlNodeListGetString(pc->doc_data, data_node->xmlChildrenNode, 1);
  dbg("value:%s", value);

  encodeSemiConstrainedOctetString(pc->encodep, (char *)value);
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
  } else if (xmlStrEqual(type, BAD_CAST "utf8-string")) {
    encode_utf8_string(pc, data_node, schema_node);    
  } else if (xmlStrEqual(type, BAD_CAST "unix-time")) {
    encode_unix_time(pc, data_node, schema_node);    
  } else {
    alert("Found a type I can't encode.");
  }
  xmlFree(type);

}

