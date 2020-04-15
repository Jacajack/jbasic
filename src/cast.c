#include <jbasic/cast.h>
#include <jbasic/jbasic.h>
#include <jbasic/paren.h>
#include <jbasic/resource.h>

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
	n->type = t;
	if (t == JBAS_NUM_INT)
		n->i = n->f;
	else if (t == JBAS_NUM_FLOAT)
		n->f = n->i;
	else if (t == JBAS_NUM_BOOL)
	{
		if (n->type == JBAS_NUM_FLOAT)
			n->i = n->f != 0;
		else
			n->i = n->i != 0;
	}
}

/**
	Converts token to number 
*/
jbas_error jbas_token_to_number(jbas_env *env, jbas_token *t)
{
	if (t->type == JBAS_TOKEN_NUMBER) return JBAS_OK;
	else if (t->type == JBAS_TOKEN_PAREN)
	{
		jbas_error err;
		err = jbas_eval_paren(env, t);
		if (err) return err;
		return jbas_token_to_number(env, t);
	}
	else if (t->type == JBAS_TOKEN_SYMBOL)
	{
		jbas_error err;
		err = jbas_eval_scalar_symbol(env, t);
		if (err) return err;
		return jbas_token_to_number(env, t);
	}

	JBAS_ERROR_REASON(env, "could not convert token to number");
	return JBAS_CAST_FAILED;
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
*/
bool jbas_can_cast_to_number(jbas_token *t)
{
	return t->type == JBAS_TOKEN_NUMBER || 
		(t->type == JBAS_TOKEN_SYMBOL && t->symbol_token.sym->resource && t->symbol_token.sym->resource->type == JBAS_RESOURCE_NUMBER);
}