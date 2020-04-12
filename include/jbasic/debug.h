#ifndef JBASIC_DEBUG_H
#define JBASIC_DEBUG_H

#include <stdio.h>
#include <jbasic/defs.h>
#include <jbasic/resource.h>
#include <jbasic/token.h>

const char *jbas_debug_keyword_str(jbas_keyword id);
void jbas_debug_dump_token(FILE *f, const jbas_token *token);
void jbas_debug_dump_resource(FILE *f, jbas_resource *res);
void jbas_debug_dump_symbol(FILE *f, jbas_symbol *sym);
void jbas_debug_dump_symbol_table(FILE *f, jbas_env *env);

#endif