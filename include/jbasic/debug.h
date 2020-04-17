#ifndef JBASIC_DEBUG_H
#define JBASIC_DEBUG_H

#include <stdio.h>
#include <jbasic/defs.h>
#include <jbasic/resource.h>
#include <jbasic/token.h>

void jbas_debug_dump_token(FILE *f, jbas_token *token);
void jbas_debug_dump_token_list_begin_end(FILE *f, jbas_token *begin, jbas_token *end);
void jbas_debug_dump_token_list(FILE *f, jbas_token *t);
void jbas_debug_dump_resource(FILE *f, jbas_resource *res);
void jbas_debug_dump_symbol(FILE *f, jbas_symbol *sym);
void jbas_debug_dump_symbol_table(FILE *f, jbas_env *env);
void jbas_debug_dump_resource_manager(FILE *f, jbas_resource_manager *rm);

#endif