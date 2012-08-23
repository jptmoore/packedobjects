#ifndef EXPAND_H_
#define EXPAND_H_

#include "packedobjects.h"

xmlDoc *expand_user_defined_types(packedobjectsContext *pc);
void hash_user_defined_types(packedobjectsContext *pc);
int is_simple_type(const xmlChar *type);

#endif
