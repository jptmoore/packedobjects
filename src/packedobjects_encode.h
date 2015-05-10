#ifndef PACKEDOBJECTS_ENCODE_H_
#define PACKEDOBJECTS_ENCODE_H_

#include "packedobjects.h"

// main api function
char *packedobjects_encode(packedobjectsContext *pc, xmlDocPtr doc);

// convenience function
char *packedobjects_encode_with_string(packedobjectsContext *pc, const char *xml);

// auxillary functions
int encode_make_memory(packedobjectsContext *pc, size_t bytes);
void encode_free_memory(packedobjectsContext *pc);

#endif
