#ifndef SCHEMA_H_
#define SCHEMA_H_

#include "packedobjects.h"

int schema_setup_xpath(packedobjectsContext *pc);
int schema_validate_schema(packedobjectsContext *pc);
int schema_setup_validation(packedobjectsContext *pc);
void schema_free_validation(packedobjectsContext *pc);
void schema_free_xpath(packedobjectsContext *pc);

#endif
