#include <jbasic/symbol.h>
#include <jbasic/jbasic.h>
#include <stdlib.h>

jbas_error jbas_symbol_manager_init(jbas_symbol_manager *sm, int symbol_count)
{
	sm->max_count = symbol_count;
	sm->free_slot_count = symbol_count;

	sm->symbol_storage = calloc(symbol_count, sizeof(jbas_symbol));
	sm->is_used = calloc(symbol_count, sizeof(bool));
	sm->free_slots = calloc(symbol_count, sizeof(int));
	
	if (!sm->symbol_storage)
	{
		free(sm->symbol_storage);
		free(sm->is_used);
		free(sm->free_slots);
		return JBAS_ALLOC;
	}

	for (int i = 0; i < sm->free_slot_count; i++)
		sm->free_slots[i] = i;

	return JBAS_OK;
}

/**
	Desrtoys all the symbols inside too
*/
void jbas_symbol_manager_destroy(jbas_symbol_manager *sm)
{
	for (int i = 0; i < sm->max_count; i++)
		if (sm->is_used[i])
			jbas_symbol_destroy(sm, &sm->symbol_storage[i]);

	free(sm->symbol_storage);
	free(sm->is_used);
	free(sm->free_slots);
}


/**
	Registers a new symbol name in a jbas_environment. No duplicates are allowed.
	The symbol name is also stored as a jbas_text structure.

	Name and resource object management are left up to the user! (I mean myself...)
*/
jbas_error jbas_symbol_create(jbas_env *env, jbas_symbol **sym, const char *s, const char *end)
{
	jbas_symbol_manager *sm = &env->symbol_manager;
	jbas_text_manager *tm = &env->text_manager;

	// No empty space
	if (!sm->free_slot_count) return JBAS_SYMBOL_MANAGER_OVERFLOW;

	// Look for collisions
	for (int i = 0; i < sm->max_count; i++)
	{
		if (!sm->is_used[i]) continue;
		if (!jbas_namecmp(s, end, sm->symbol_storage[i].name->str, NULL))
		{
			*sym = &sm->symbol_storage[i];
			return JBAS_SYMBOL_COLLISION;
		}
	}

	// Actually create the symbol
	jbas_text *name_text;
	jbas_error err = jbas_text_lookup_create(tm, s, end, &name_text);
	if (err) return err;

	// Get an empty slot
	int slot = sm->free_slots[--sm->free_slot_count];
	sm->is_used[slot] = true;

	sm->symbol_storage[slot].name = name_text;
	sm->symbol_storage[slot].resource = NULL;
	*sym = &sm->symbol_storage[slot];

	return JBAS_OK;
}

/**
	Looks symbol up in a symbol_manager by name. When no match is found
	a NULL pointer is returned through parameter `sym`
*/
jbas_error jbas_symbol_lookup(jbas_symbol_manager *sm, jbas_symbol **sym, const char *s, const char *end)
{
	for (int i = 0; i < sm->max_count; i++)
	{
		if (!sm->is_used[i]) continue;
		if (!jbas_namecmp(s, end, sm->symbol_storage[i].name->str, NULL))
		{
			*sym = &sm->symbol_storage[i];
			return JBAS_OK;
		}
	}

	*sym = NULL;
	return JBAS_OK;
}

/**
	\warning This function does not delete text and resource objects associated with deleted symbol
*/
void jbas_symbol_destroy(jbas_symbol_manager *sm, jbas_symbol *sym)
{
	int slot = sym - sm->symbol_storage;
	if (slot >= sm->max_count) return;

	sm->free_slots[sm->free_slot_count++] = slot;
	sm->is_used[slot] = false;
}


/**
	Returns true if provided token symbol is a scalar.
*/
bool jbas_is_scalar_symbol(jbas_token *t)
{
	if (t->type != JBAS_TOKEN_SYMBOL) return false;
	else
	{
		jbas_resource *res = t->symbol_token.sym->resource;
		if (!res) return true;

		switch (res->type)
		{
			case JBAS_RESOURCE_NUMBER:
				return true;

			case JBAS_RESOURCE_STRING:
				return true;
				break;

			default:
				return false;
				break;
		}
	}
}

/**
	Evaluates a scalar symbol - replaces it with its value
*/
jbas_error jbas_eval_scalar_symbol(jbas_env *env, jbas_token *t)
{
	if (t->type != JBAS_TOKEN_SYMBOL) return JBAS_OK;
	if (!jbas_is_scalar_symbol(t))
	{
		JBAS_ERROR_REASON(env, "attempted to evaluate non-scalar symbol value");
		return JBAS_EVAL_NON_SCALAR;
	}

	jbas_symbol *sym = t->symbol_token.sym;
	jbas_token res;

	if (!sym->resource) return JBAS_UNINITIALIZED_SYMBOL;
	switch (sym->resource->type)
	{
		case JBAS_RESOURCE_NUMBER:
			res.type = JBAS_TOKEN_NUMBER;
			res.number_token = sym->resource->number;
			break;

		default:
			return JBAS_CANNOT_EVAL_RESOURCE;
			break;
	}

	return jbas_token_move(t, &res, &env->token_pool);
}

