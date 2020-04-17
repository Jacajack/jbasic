#include <jbasic/cast.h>
#include <jbasic/jbasic.h>
#include <jbasic/paren.h>
#include <jbasic/resource.h>

/**
	Replaces symbol with resource it has attached
*/
jbas_error jbas_symbol_to_resource(jbas_env *env, jbas_token *t)
{
	// Symbols are replaced with their resources
	if (t->type == JBAS_TOKEN_SYMBOL)
	{
		jbas_symbol *sym = t->symbol_token.sym;
		
		// Extract the resource pointer
		// A new temporary resource token is created from the
		// symbol hence the ref count must be updated
		jbas_resource *res = sym->res;
		jbas_token rt = {.type = JBAS_TOKEN_RESOURCE};
		jbas_resource_add_ref(res);
		rt.resource_token.res = res;

		jbas_error err = jbas_token_move(t, &rt, &env->token_pool);
		if (err) return err;
	}

	return JBAS_OK;
}

/**
	Tries to extract the 'real' value from provided token.
	Parentheses are evaluated, one element tuples are replaced with their contents,
	Symbols are converted to resource tokens.
	Resource tokens are converted to values if possible.

	\note This function is kinda very aggresive 
*/
jbas_error jbas_to_value(jbas_env *env, jbas_token *t)
{
	if (!t) return JBAS_OK;

	// Parentheses
	if (jbas_is_paren(t))
	{
		jbas_error err = jbas_eval_paren(env, t);
		if (err) return err;
		return jbas_to_value(env, t);
	}

	// Tuples
	if (t->type == JBAS_TOKEN_TUPLE)
	{
		// One element only
		if (t->tuple_token.tokens && t->tuple_token.tokens->l == NULL && t->tuple_token.tokens->r)
		{
			jbas_error err = jbas_token_move(t, t->tuple_token.tokens, &env->token_pool);
			if (err) return err;
			return jbas_to_value(env, t);
		}
	}

	// Symbol token
	jbas_error err = jbas_symbol_to_resource(env, t);
	if (err) return err;

	// Resource tokens
	if (t->type == JBAS_TOKEN_RESOURCE)
	{
		jbas_resource *res = t->resource_token.res;
		if (res)
		{
			// Ordinary number
			if (res->type == JBAS_RESOURCE_NUMBER)
			{
				jbas_token nt = {.type = JBAS_TOKEN_NUMBER};
				nt.number_token = res->number;
				jbas_error err = jbas_token_move(t, &nt, &env->token_pool);
				if (err) return err;
				return jbas_to_value(env, t);
			}

			// Int pointer
			if (res->type == JBAS_RESOURCE_INT_PTR)
			{
				jbas_token nt = {.type = JBAS_TOKEN_NUMBER};
				nt.number_token.type = JBAS_NUM_INT;
				nt.number_token.i = *res->iptr;
				jbas_error err = jbas_token_move(t, &nt, &env->token_pool);
				if (err) return err;
				return jbas_to_value(env, t);
			}

			// Float pointer
			if (res->type == JBAS_RESOURCE_FLOAT_PTR)
			{
				jbas_token nt = {.type = JBAS_TOKEN_NUMBER};
				nt.number_token.type = JBAS_NUM_FLOAT;
				nt.number_token.f = *res->fptr;
				jbas_error err = jbas_token_move(t, &nt, &env->token_pool);
				if (err) return err;
				return jbas_to_value(env, t);
			}
		}
	}

	return JBAS_OK;
}

/**
	Determines the proper numeric type for the result of a mathematical
	operation on two numeric operands
*/
jbas_number_type jbas_number_type_promotion(jbas_number_type a, jbas_number_type b)
{
	if (a >= b) return a;
	else return b;
}

/**
	Casts number token to specified type
*/
void jbas_number_cast(jbas_number_token *n, jbas_number_type t)
{
	if (n->type == t) return;
	if (t == JBAS_NUM_INT && n->type == JBAS_NUM_FLOAT)
		n->i = n->f;
	else if (t == JBAS_NUM_FLOAT && (n->type == JBAS_NUM_INT || n->type == JBAS_NUM_BOOL))
		n->f = n->i;
	else if (t == JBAS_NUM_BOOL)
	{
		if (n->type == JBAS_NUM_FLOAT)
			n->i = n->f != 0;
		else if (n->type == JBAS_NUM_INT)
			n->i = n->i != 0;
	}

	n->type = t;
}

/**
	Converts token to number 
*/
jbas_error jbas_token_to_number(jbas_env *env, jbas_token *t)
{
	if (t->type == JBAS_TOKEN_NUMBER) return JBAS_OK;
	jbas_error err = jbas_to_value(env, t);
	if (err) return err;
	if (t->type != JBAS_TOKEN_NUMBER)
	{
		JBAS_ERROR_REASON(env, "could not convert token to number");
		return JBAS_CAST_FAILED;
	}

	return JBAS_OK;
}

/**
	Converts token to number and casts it to specified type
*/
jbas_error jbas_token_to_number_type(jbas_env *env, jbas_token *t, jbas_number_type type)
{
	jbas_error err = jbas_token_to_number(env, t);
	if (err) return err;
	jbas_number_cast(&t->number_token, type);
	return JBAS_OK;
}

/**
	Determines whether the token can be casted to a number
	\todo fix
*/
bool jbas_can_cast_to_number(jbas_token *t)
{
	return t->type == JBAS_TOKEN_NUMBER;
	// || (t->type == JBAS_TOKEN_SYMBOL && t->symbol_token.sym->resource && t->symbol_token.sym->resource->type == JBAS_RESOURCE_NUMBER);
}