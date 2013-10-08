#ifndef PACKEDOBJECTS_INIT_H_
#define PACKEDOBJECTS_INIT_H_

#include "packedobjects.h"

#include "canon.h"
#include "expand.h"
#include "schema.h"

packedobjectsContext *init_packedobjects(const char *schema_file, size_t bytes, int options);
void free_packedobjects(packedobjectsContext *poCtxPtr);
// low-level api use only
packedobjectsContext *_init_packedobjects();

#endif
