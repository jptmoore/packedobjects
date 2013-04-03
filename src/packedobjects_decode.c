#include <stdio.h>
#include <setjmp.h>
#include <string.h>

#include "packedobjects_decode.h"

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
jmp_buf decode_exception_env;

static void packedobjects_validate_decode(packedobjectsContext *poCtxPtr, xmlDocPtr doc);

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
static void decode_unix_time(packedobjectsContext *pc, xmlNodePtr data_node, xmlNodePtr schema_node);
static void decode_utf8_string(packedobjectsContext *pc, xmlNodePtr data_node, xmlNodePtr schema_node);


void packedobjects_validate_decode(packedobjectsContext *poCtxPtr, xmlDocPtr doc)
{
  int result;
  schemaData *schemap = poCtxPtr->schemap;
  
  result = xmlSchemaValidateDoc(schemap->validCtxt, doc);
  if (result) {
    alert("Failed to validate XSD schema.");
    longjmp(decode_exception_env, DECODE_VALIDATION_FAILED);
  }  

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
  unsigned long i, len;
  xmlChar *minOccurs = NULL;
  xmlChar *maxOccurs = NULL;
  unsigned long lb = 0;

  minOccurs = xmlGetProp(schema_node, BAD_CAST "minOccurs");
  maxOccurs = xmlGetProp(schema_node, BAD_CAST "maxOccurs");
  // overide default of 0 if set
  if (minOccurs) lb = atoi((const char *) minOccurs);  
  dbg("lb:%lu", lb);
  if (xmlStrEqual(maxOccurs, BAD_CAST "unbounded")) {
     // decode as semi-constrained
    len = decodeUnsignedSemiConstrainedInteger(pc->decodep, lb);
  } else {
    // decode as constrained
    unsigned long ub = atoi((const char *) maxOccurs);
    dbg("ub:%lu", ub);
    len = decodeUnsignedConstrainedInteger(pc->decodep, lb, ub);
  }
  xmlFree(minOccurs);
  xmlFree(maxOccurs);  
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
  char value[12];
  
  n = decodeUnconstrainedInteger(pc->decodep);
  dbg("n:%ld", n);
  sprintf(value, "%ld", n);
  xmlNewChild(data_node, NULL, schema_node->name, BAD_CAST value);    

  decode_next(pc, data_node, schema_node->children);   

}

static void decode_semi_constrained_integer(packedobjectsContext *pc, xmlNodePtr data_node, xmlNodePtr schema_node)
{

  signed long int n;
  char value[12];
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
  char value[12];
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
    alert("Found an integer variant I can't decode.");    
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
    value = decodeFixedLengthBitString(pc->decodep, len);
    break;
  case NUMERIC_STRING:
    value = decodeFixedLengthNumericString(pc->decodep, len);
    break;
  case HEX_STRING:
    value = decodeFixedLengthHexString(pc->decodep, len);
    break;
  case OCTET_STRING:
    value = decodeFixedLengthOctetString(pc->decodep, len);
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
    alert("Found a string variant I can't decode.");
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

static void decode_unix_time(packedobjectsContext *pc, xmlNodePtr data_node, xmlNodePtr schema_node)
{

  xmlChar *value = NULL;

  value = BAD_CAST decodeUnixTime(pc->decodep);
  xmlNewChild(data_node, NULL, schema_node->name, value);  
  xmlFree(value);
}

static void decode_utf8_string(packedobjectsContext *pc, xmlNodePtr data_node, xmlNodePtr schema_node)
{

  xmlChar *value = NULL;

  value = BAD_CAST decodeSemiConstrainedOctetString(pc->decodep);
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
  } else if (xmlStrEqual(type, BAD_CAST "unix-time")) {
    decode_unix_time(pc, data_node, schema_node);
  } else if (xmlStrEqual(type, BAD_CAST "utf8-string")) {
    decode_utf8_string(pc, data_node, schema_node);    
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
    alert("Found a type I can't decode.");
  }
  xmlFree(type);

}


xmlDocPtr packedobjects_decode(packedobjectsContext *pc, char *pdu)
{
  xmlDocPtr doc_data = NULL;
  xmlNodePtr data_node = NULL;
  xmlNodePtr schema_node = NULL;

  // make sure we reset this on each call
  pc->decode_error = 0;

  // exception handler
  switch (setjmp(decode_exception_env)) {
  case DECODE_VALIDATION_FAILED:  
    pc->decode_error = DECODE_VALIDATION_FAILED;
    break;
  case DECODE_INVALID_PREFIX:  
    pc->decode_error = DECODE_INVALID_PREFIX;
    break;  
  case 0:
    pc->decodep = initializeDecode(pdu);
    pc->doc_data = doc_data;
    schema_node = xmlDocGetRootElement(pc->doc_canonical_schema);
    // add a temporary root for convenience
    data_node = xmlNewNode(NULL, BAD_CAST "root");  
    decode_node(pc, data_node, schema_node);
    freeDecode(pc->decodep);
    dbg("creating XML data:");
    doc_data = xmlNewDoc(BAD_CAST "1.0");
    // ignore the temporary root
    xmlDocSetRootElement(doc_data, data_node->children);
    xmlFreeNode(data_node);  
    // validate data against schema
    packedobjects_validate_decode(pc, doc_data);
  }
  
  return doc_data;
}

