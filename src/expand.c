#include "expand.h"

#ifdef DEBUG_MODE
#define dbg(fmtstr, args...) \
  (printf(PROGNAME ":%s: " fmtstr "\n", __func__, ##args))
#else
#define dbg(dummy...)
#endif

static void expand_user_defined_types_worker(packedobjectsContext *pc, xmlNode *node1, xmlNode *node2);

char *simple_types[] =
  { "integer",
    "boolean",
    "numeric-string",
    "bit-string",
    "hex-string",
    "string",
    "octet-string",
    "enumerated",
    "null",
    "decimal",
    "currency",
    "ipv4-address",
    "unix-time",
    "utf8-string",
    NULL
  };

xmlDoc *expand_user_defined_types(packedobjectsContext *pc)
{
  xmlDoc *expanded_doc = NULL;
  xmlNodePtr root_node = NULL, expanded_root_node = NULL;
  xmlChar *element_name = NULL;
  xmlDoc *doc = pc->doc_schema;
  
  dbg("expanding user defined types in XML schema:");
  root_node = xmlDocGetRootElement(doc);
  
  expanded_doc = xmlNewDoc((const xmlChar *) "1.0");
  expanded_root_node = xmlCopyNode(root_node, 2);
  xmlDocSetRootElement(expanded_doc, expanded_root_node);  

  // lets ignore any includes or udt before the start element
  root_node = root_node->children;
  while (1) {
    if ((!xmlStrcmp(root_node->name, (const xmlChar *)"element"))) {
      element_name = xmlGetProp(root_node, (const xmlChar *)"name");      
      if ((!xmlStrcmp(element_name, pc->start_element_name))) {
        xmlFree(element_name);
        break;
      }
      xmlFree(element_name);
    }
    root_node  = root_node->next;
  }

  
  expand_user_defined_types_worker(pc, root_node, expanded_root_node);
  
  return expanded_doc;
  
}

void expand_user_defined_types_worker(packedobjectsContext *pc, xmlNode *node1, xmlNode *node2)
{
  xmlNode *cur_node = NULL;
  xmlChar *element_name = NULL;
  xmlNodePtr udt_node = NULL;
  xmlNodePtr np = NULL;

  xmlNodePtr new_node = NULL;
  
  for (cur_node = node1; cur_node; cur_node = cur_node->next) {
    if (cur_node->type == XML_ELEMENT_NODE) {
      // look for user defined types to replace from hash table
      element_name = xmlGetProp(cur_node, (const xmlChar *)"type");
      // if a udt
      if ( (element_name) && (!is_simple_type(element_name)) ) {
        udt_node = xmlHashLookup(pc->udt, element_name);
        xmlFree(element_name);
        new_node = xmlCopyNode(cur_node, 2);
        np = xmlAddChild(node2, new_node);
        // create a node with just complexType or simpleType;
        np = xmlNewChild(np, NULL, udt_node->name, NULL);
        new_node = xmlCopyNode(udt_node, 2);
        xmlAddChild(np->children, new_node);
        expand_user_defined_types_worker(pc, udt_node->children, np);
      } else if (cur_node->children) {
        xmlFree(element_name);
        new_node = xmlCopyNode(cur_node, 2);
        np = xmlAddChild(node2, new_node);
        xmlAddChild(np->children, new_node);
        expand_user_defined_types_worker(pc, cur_node->children, np);
      } else {
        xmlFree(element_name);
        new_node = xmlCopyNode(cur_node, 2);
        xmlAddChild(node2, new_node);
        expand_user_defined_types_worker(pc, cur_node->children, node2);
      }
    }
  }
}

void hash_user_defined_types(packedobjectsContext *pc)
{

  xmlNodePtr schema_node = NULL;
  xmlNodePtr cur_node = NULL;
  xmlDoc *doc = pc->doc_schema;
  xmlChar *element_name = NULL;

  // first node <xs:schema>
  schema_node = xmlDocGetRootElement(doc);
  // drop down a level
  cur_node = schema_node->xmlChildrenNode;
  while (cur_node != NULL) {
    if ((!xmlStrcmp(cur_node->name, (const xmlChar *)"element"))) {
      element_name = xmlGetProp(cur_node, (const xmlChar *)"name");      
      if ((!xmlStrcmp(element_name, pc->start_element_name))) {
        dbg("ignoring start element");
      }
      xmlFree(element_name);
    } else if ((!xmlStrcmp(cur_node->name, (const xmlChar *)"include"))) {
      dbg("ignoring includes");
    } else {
      // we add the rest as user defined types 
      element_name = xmlGetProp(cur_node, (const xmlChar *)"name");
      dbg("adding %s to udt hash table", element_name);
      xmlHashAddEntry(pc->udt, element_name, cur_node);
      xmlFree(element_name);
    }
    cur_node = cur_node->next;
  }

}


int is_simple_type(const xmlChar *type)
{
  int result = 0;
  char **st = simple_types;
  while (*st) {
    if (xmlStrEqual(BAD_CAST *st, type)) {
      result = 1;
    }
    st++;
  }
  return result;

}
