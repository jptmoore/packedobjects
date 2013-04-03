#ifndef PACKEDOBJECTS_INIT_H_
#define PACKEDOBJECTS_INIT_H_

#include "packedobjects.h"

#include "canon.h"
#include "expand.h"

packedobjectsContext *init_packedobjects(const char *schema_file);
void free_packedobjects(packedobjectsContext *poCtxPtr);

#endif
