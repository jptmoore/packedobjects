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


static packedobjectsContext *_init_packedobjects()
{
  packedobjectsContext *pc = NULL;
  
  if ((pc = (packedobjectsContext *)malloc(sizeof(packedobjectsContext))) == NULL) {
    alert("Could not alllocate memory.");
    return NULL;    
  }
  
  pc->doc_schema = NULL;
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
  pc->init_options = 0;
  pc->init_error = 0;
  pc->encode_error = 0;
  pc->decode_error = 0;

  return pc;
  
}

packedobjectsContext *init_packedobjects(const char *schema_file, size_t bytes, int options)
{
  packedobjectsContext *pc = NULL;

  // do the real allocation of structure with the provided schema
  if ((pc = _init_packedobjects()) == NULL) {
    pc->init_error = INIT_FAILED;
    return NULL;
  }

  // we will need these later
  pc->init_options = options;

  // store the schema we will work with
  if (schema_setup_schema(pc, schema_file) == -1) {
    pc->init_error = INIT_SCHEMA_SETUP_FAILED;
    return NULL;
  }  
  
  // used to store the encoded data
  if (encode_make_memory(pc, bytes) == -1) {
    pc->init_error = INIT_ENCODE_SETUP_FAILED;
    return NULL;
  }

  // check flag
  if ((options & NO_SCHEMA_VALIDATION) == 0) {
    // validate the schema conforms to PO schema
    if (schema_validate_schema(pc) == -1) {
      pc->init_error = INIT_SCHEMA_VALIDATION_FAILED;
      return NULL;
    }
  }

  // check flag
  if ((options & NO_DATA_VALIDATION) == 0) {  
    // initialise xml validation code for later 
    if (schema_setup_validation(pc) == -1) {
      pc->init_error = INIT_SETUP_VALIDATION_FAILED;
      return NULL;
    }
  }
  
  // expand user defined types in schema
  if (expand_make_expanded_schema(pc) == -1) {
    pc->init_error = INIT_EXPANDED_SCHEMA_FAILED;
    return NULL;
  }

  // make the canonical schema used for encoding
  if (canon_make_canonical_schema(pc) == -1) {
    pc->init_error = INIT_CANON_SCHEMA_FAILED;
    return NULL;
  }

  // setup xpath to use during encode
  if (schema_setup_xpath(pc) == -1) {
    pc->init_error = INIT_XPATH_SETUP_FAILED;
    return NULL;
  }
  
  return pc;
}

void free_packedobjects(packedobjectsContext *pc)
{
  if ((pc->init_options & NO_DATA_VALIDATION) == 0) {  
    schema_free_validation(pc);
  }
  expand_free(pc);
  canon_free(pc);
  schema_free_xpath(pc);
  encode_free_memory(pc);
  schema_free(pc);
  
  // free the structure
  free(pc);

  xmlCleanupParser();
}



