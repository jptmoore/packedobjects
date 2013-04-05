#include "canon.h"
#include "expand.h"

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

static xmlDocPtr make_canonical_schema(xmlDocPtr doc);
static void make_canonical_schema_worker(xmlNode *node1, xmlNode *node2);  
static xmlNodePtr make_simple_simple_type(xmlNodePtr node);
static xmlNodePtr make_simple_type(xmlNodePtr node);
static xmlChar *make_variant(xmlNodePtr node1, xmlNodePtr node2, xmlChar *type, long unsigned int count);
static xmlChar *make_string_variant(xmlNodePtr node1, xmlNodePtr node2, long unsigned int count);
static xmlChar *make_integer_variant(xmlNodePtr node1, xmlNodePtr node2, long unsigned int count);
static xmlChar *make_enumerated_variant(xmlNodePtr node1, xmlNodePtr node2, long unsigned int count);
xmlChar *get_sequence_type(xmlNodePtr node);


xmlDocPtr packedobjects_make_canonical_schema(packedobjectsContext *pc)
{
  xmlDocPtr canonical_doc = NULL;
  xmlNodePtr root_node = NULL, canonical_root_node = NULL;

  xmlDocPtr doc = pc->doc_expanded_schema;
  
  dbg("creating canonical XML schema:");
  root_node = xmlDocGetRootElement(doc);
  canonical_doc = xmlNewDoc(BAD_CAST "1.0");
  canonical_root_node = xmlCopyNode(root_node, 2);
  xmlDocSetRootElement(canonical_doc, canonical_root_node); 

  make_canonical_schema_worker(root_node->children, canonical_root_node);

  // get rid of xs:schema node
  xmlDocSetRootElement(canonical_doc, canonical_root_node->children);
  xmlUnlinkNode(canonical_root_node);
  xmlFreeNode(canonical_root_node);
  
  return canonical_doc;
}


static xmlNodePtr make_simple_simple_type(xmlNodePtr node)
{

  xmlChar *node_name = NULL;
  xmlNodePtr new_node = NULL;
  xmlChar *type = NULL;
  xmlChar *variant = NULL;
  
  node_name = xmlGetProp(node, BAD_CAST "name");
  new_node = xmlNewNode(NULL, node_name);
  xmlFree(node_name);
  type = xmlGetProp(node, BAD_CAST "type");
  xmlNewProp(new_node, BAD_CAST "type", type);
  variant = make_variant(node, new_node, type, 0);
  xmlNewProp(new_node, BAD_CAST "variant", variant);
  xmlFree(type);
  
  return new_node;

}

static xmlChar *make_variant(xmlNodePtr node1, xmlNodePtr node2, xmlChar *type, long unsigned int count)
{
  xmlChar *variant = NULL;
  
  if ( (xmlStrEqual(type, BAD_CAST "string")) ||
       (xmlStrEqual(type, BAD_CAST "bit-string")) ||
       (xmlStrEqual(type, BAD_CAST "numeric-string")) ||
       (xmlStrEqual(type, BAD_CAST "hex-string")) ||
       (xmlStrEqual(type, BAD_CAST "octet-string")) )

    { variant = make_string_variant(node1, node2, count); }
  
  if (xmlStrEqual(type, BAD_CAST "integer")) {
    variant = make_integer_variant(node1, node2, count); 
  }

  if (xmlStrEqual(type, BAD_CAST "enumerated")) {
    variant = make_enumerated_variant(node1, node2, count); 
  }
  
  return variant;
}

static xmlChar *make_enumerated_variant(xmlNodePtr node1, xmlNodePtr node2, long unsigned int count)
{
  xmlChar *variant = NULL;
  char items[11];

  sprintf(items, "%lu", count);
  xmlNewProp(node2, BAD_CAST "items", BAD_CAST items);
  
  return variant;

}

static xmlChar *make_string_variant(xmlNodePtr node1, xmlNodePtr node2, long unsigned int count)
{
  xmlChar *variant = BAD_CAST "unknown";

  if (count == 0) {
    variant = BAD_CAST "semi-constrained";
    xmlNewProp(node2, BAD_CAST "minLength", BAD_CAST "0");
  }
  
  if (count == 1) {
    if (xmlStrEqual(node1->name, BAD_CAST "length")) {
      variant = BAD_CAST "fixed-length";
    } else if (xmlStrEqual(node1->name, BAD_CAST "maxLength")) {
      variant = BAD_CAST "constrained";
      xmlNewProp(node2, BAD_CAST "minLength", BAD_CAST "0");
    } else if (xmlStrEqual(node1->name, BAD_CAST "minLength")) {
      variant = BAD_CAST "semi-constrained";      
    } else {
      fprintf(stderr, "unknown constraints\n");
    }
  }

  if (count == 2) {
    if ((xmlStrEqual(node1->name, BAD_CAST "minLength")) &&
        (xmlStrEqual(node1->next->name, BAD_CAST "maxLength"))) {
      variant =  BAD_CAST "constrained";
    } else {
      fprintf(stderr, "unknown constraints\n");
    }
  }

  return variant;
}

static xmlChar *make_integer_variant(xmlNodePtr node1, xmlNodePtr node2, long unsigned int count)
{
  xmlChar *variant = BAD_CAST "unknown";

  if (count == 0) {
    variant = BAD_CAST "unconstrained";
  }  

  if (count == 1) {
    if (xmlStrEqual(node1->name, BAD_CAST "maxInclusive")) {
      variant = BAD_CAST "unconstrained";
    } else if (xmlStrEqual(node1->name, BAD_CAST "minInclusive")) {
      variant = BAD_CAST "semi-constrained";   
    } else {
      fprintf(stderr, "unknown constraints\n");
    }
  }
  
  if (count == 2) {
    if ((xmlStrEqual(node1->name, BAD_CAST "minInclusive")) &&
        (xmlStrEqual(node1->next->name, BAD_CAST "maxInclusive"))) {
      variant =  BAD_CAST "constrained";
    } else {
      fprintf(stderr, "unknown constraints\n");
    }
  }
  
  return variant;
}

static xmlNodePtr make_simple_type(xmlNodePtr node)
{

  xmlNodePtr np = NULL;
  xmlNodePtr new_node = NULL;
  xmlChar *node_name = NULL;
  xmlChar *type = NULL;
  xmlChar *value = NULL;
  xmlChar *variant = NULL;
  long unsigned int count;
  
  node_name = xmlGetProp(node, BAD_CAST "name");
  new_node = xmlNewNode(NULL, node_name);
  xmlFree(node_name);
  // move to restriction
  np = node->children->children;
  type = xmlGetProp(np, BAD_CAST "base"); 
  xmlNewProp(new_node, BAD_CAST "type", type);
  
  // count from restriction element
  count = xmlChildElementCount(np);
  
  // move to the attributes
  np = np->children;
  variant = make_variant(np, new_node, type, count);
  xmlNewProp(new_node, BAD_CAST "variant", variant);
  xmlFree(type);

  // set all the attributes
  for (; np; np = np->next) {
    value = xmlGetProp(np, BAD_CAST "value");
    xmlNewProp(new_node, np->name, value);
    xmlFree(value);
  }
  
  return new_node;

}

xmlChar *get_sequence_type(xmlNodePtr node)
{ 
  xmlChar *value = NULL;
  xmlNodePtr np = NULL;

  xmlChar *type = BAD_CAST "sequence";
  
  np = node;
  while (np) {
    value = xmlGetProp(np, BAD_CAST "maxOccurs");
    if (value) {
      xmlFree(value);
      type = BAD_CAST "sequence-of";
      break;
    }
    
    value = xmlGetProp(np, BAD_CAST "minOccurs");    
    if (xmlStrEqual(value, BAD_CAST "0")) {
      xmlFree(value);
      type = BAD_CAST "sequence-optional";
      break;
    }
    np = np->next;
  }
  
  return type;
}

// refactor this
static xmlNodePtr make_complex_type(xmlNodePtr node)
{

  xmlNodePtr np = NULL;
  xmlNodePtr new_node = NULL;
  xmlChar *node_name = NULL;
  xmlChar *sequence_type = NULL;
  
  // move to the complex type
  np = node->children->children;
  if (xmlStrEqual(np->name, BAD_CAST "sequence")) {
    // move to the first element
    np = np->children;
    node_name = xmlGetProp(node, BAD_CAST "name");
    new_node = xmlNewNode(NULL, node_name);
    xmlFree(node_name);
    sequence_type = get_sequence_type(np);
    dbg("sequence_type:%s", sequence_type);
    if (xmlStrEqual(sequence_type, BAD_CAST "sequence-of")) {
      char items[11];
      unsigned long n = xmlChildElementCount(np->parent);
      xmlChar *minOccurs = NULL;
      xmlChar *maxOccurs = NULL;
      sprintf(items, "%lu", n);
      xmlNewProp(new_node, BAD_CAST "type", BAD_CAST "sequence-of");
      xmlNewProp(new_node, BAD_CAST "items", BAD_CAST items);
      minOccurs = xmlGetProp(np, BAD_CAST "minOccurs");
      xmlNewProp(new_node, BAD_CAST "minOccurs", minOccurs);
      maxOccurs = xmlGetProp(np, BAD_CAST "maxOccurs");
      xmlNewProp(new_node, BAD_CAST "maxOccurs", maxOccurs);
      xmlFree(minOccurs);
      xmlFree(maxOccurs);
    } else if (xmlStrEqual(sequence_type, BAD_CAST "sequence-optional")) {
      char items[11];
      unsigned long n = xmlChildElementCount(np->parent);
      sprintf(items, "%lu", n);      
      xmlNewProp(new_node, BAD_CAST "type", BAD_CAST "sequence-optional");
      xmlNewProp(new_node, BAD_CAST "items", BAD_CAST items);      
    } else {
      xmlNewProp(new_node, BAD_CAST "type", BAD_CAST "sequence");
    }
  } else if (xmlStrEqual(np->name, BAD_CAST "choice")) {
    char items[11];
    unsigned long n = 0;
    n = xmlChildElementCount(np);
    sprintf(items, "%lu", n);
    dbg("items:%s", items);
    node_name = xmlGetProp(node, BAD_CAST "name");
    new_node = xmlNewNode(NULL, node_name);
    xmlFree(node_name);
    xmlNewProp(new_node, BAD_CAST "type", BAD_CAST "choice");
    xmlNewProp(new_node, BAD_CAST "items", BAD_CAST items);
  } else {
    alert("unsupported type: %s", np->name);
    // replace this with longjmp when exception handler added
    exit(EXIT_FAILURE);
  }
  
  return new_node;

}


static void make_canonical_schema_worker(xmlNode *node1, xmlNode *node2)
{
  xmlNodePtr cur_node = NULL;
  xmlNodePtr new_node = NULL;
  xmlNodePtr np = NULL;

  
  for (cur_node = node1; cur_node; cur_node = cur_node->next) {
    if (xmlStrEqual(cur_node->name, BAD_CAST "element")) {
      if (cur_node->children) {
        if (xmlStrEqual(cur_node->children->name, BAD_CAST "complexType")) {
          // complexType
          //np = xmlAddChild(node2, xmlCopyNode(cur_node, 2));
          new_node = make_complex_type(cur_node);
          np = xmlAddChild(node2, new_node);
          xmlAddChild(np->children, xmlCopyNode(cur_node, 2));
          make_canonical_schema_worker(cur_node->children, np);
        } else {
          // simpleType with restrictions
          new_node = make_simple_type(cur_node);
          xmlAddChild(node2, new_node);
          make_canonical_schema_worker(cur_node->children, node2);
        }
      } else {
        // straight forward simple type
        new_node = make_simple_simple_type(cur_node);
        xmlAddChild(node2, new_node);
        make_canonical_schema_worker(cur_node->children, node2);
      }
    } else {
      make_canonical_schema_worker(cur_node->children, node2);
    }
  }
}



