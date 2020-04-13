#ifndef JBASIC_SYMBOL_H
#define JBASIC_SYMBOL_H

#include <jbasic/defs.h>
#include <jbasic/text.h>
#include <jbasic/resource.h>

/**
	Symbols are links between names in the code and resoruces
*/
typedef struct jbas_symbol
{
	jbas_text *name;
	jbas_resource *resource;
} jbas_symbol;

/*
	Symbol manager handles all the symbols
*/
typedef struct jbas_symbol_manager
{
	jbas_symbol *symbol_storage;
	bool *is_used;
	int *free_slots;	
	int free_slot_count;
	int max_count;
} jbas_symbol_manager;

jbas_error jbas_symbol_manager_init(jbas_symbol_manager *sm, int symbol_count);
void jbas_symbol_manager_destroy(jbas_symbol_manager *sm);

jbas_error jbas_symbol_create(jbas_env *env, jbas_symbol **sym, const char *s, const char *end);
jbas_error jbas_symbol_lookup(jbas_symbol_manager *sm, jbas_symbol **sym, const char *s, const char *end);
void jbas_symbol_destroy(jbas_symbol_manager *sm, jbas_symbol *sym);

bool jbas_is_scalar(jbas_token *t);
jbas_error jbas_eval_scalar_symbol(jbas_env *env, jbas_token *t);

#endif