#include <jbasic/op.h>
#include <jbasic/jbasic.h>

/*
	TOKEN OPERATIONS MAY *NOT* INVALIDATE ITERATOR (POINTER)
	POINTING TO THE OPERATOR ITSELF
*/

jbas_error jbas_op_assign(jbas_env *env, jbas_token *a, jbas_token *b, jbas_token *res)
{
	if (a->type != JBAS_TOKEN_SYMBOL)
	{
		JBAS_ERROR_REASON(env, "cannot assign value to non-symbol token");
		return JBAS_BAD_ASSIGN;
	}
	jbas_symbol *asym = a->symbol_token.sym;
	jbas_resource *dest;

	// Decrement reference count on the resource currently kept in the dest. variable
	if (asym->resource) jbas_resource_remove_ref(asym->resource);

	// If there's no destination resource, create it
	if (!asym->resource) jbas_resource_create(&env->resource_manager, &asym->resource);
	dest = asym->resource;

	
	switch (b->type)
	{
		// If the source is a symbol, copy the resource and we're done
		case JBAS_TOKEN_SYMBOL:
			jbas_resource_copy(dest, b->symbol_token.sym->resource);
			break;

		// Number assignment
		case JBAS_TOKEN_NUMBER:
			dest->type = JBAS_RESOURCE_NUMBER;
			dest->number = b->number_token;
			break;

		default:
			return JBAS_BAD_ASSIGN;
			break;

	}

	jbas_token_copy(res, b);
	return JBAS_OK;
}

jbas_error jbas_op_comma(jbas_env *env, jbas_token *a, jbas_token *b, jbas_token *res)
{
	// No tuple on either side
	if (a->type != JBAS_TOKEN_TUPLE && b->type != JBAS_TOKEN_TUPLE)
	{
		// Let's make a tuple
		res->type = JBAS_TOKEN_TUPLE;
		res->tuple_token.tokens = NULL;
		jbas_error err;
		err = jbas_token_list_push_back_from_pool(res->tuple_token.tokens, &env->token_pool, a, &res->tuple_token.tokens);
		if (err) return err;
		err = jbas_token_list_push_back_from_pool(res->tuple_token.tokens, &env->token_pool, b, &res->tuple_token.tokens);
		if (err) return err;
		return JBAS_OK;
	}

	// Tuple on the left side
	if (a->type == JBAS_TOKEN_TUPLE && b->type != JBAS_TOKEN_TUPLE)
	{
		// Copy the tuple on the left side
		jbas_token_copy(res, a);

		jbas_error err;
		err = jbas_token_list_push_back_from_pool(res->tuple_token.tokens, &env->token_pool, b, &res->tuple_token.tokens);
		if (err) return err;
		return JBAS_OK;
	}

	// Tuple on the right side
	if (a->type != JBAS_TOKEN_TUPLE && b->type == JBAS_TOKEN_TUPLE)
	{
		// Copy the tuple on the right side
		jbas_token_copy(res, b);

		jbas_error err;
		err = jbas_token_list_push_front_from_pool(res->tuple_token.tokens, &env->token_pool, a, NULL);
		if (err) return err;
		return JBAS_OK;
	}

	// Tuples on both sides
	if (a->type == JBAS_TOKEN_TUPLE && b->type == JBAS_TOKEN_TUPLE)
	{	
		// Transfer tuple token ownership - merge lists
		jbas_token *rb = jbas_token_list_begin(b->tuple_token.tokens);
		jbas_token *le = jbas_token_list_end(a->tuple_token.tokens);
		b->tuple_token.tokens = NULL;		
		le->r = rb;
		rb->l = le;

		// Copy the tuple on the left
		jbas_token_copy(res, a);

		return JBAS_OK;
	}

	return JBAS_OK;
}

/**
	Takes two tokens as operands and one for the result.
	Type promotions are performed after an attempt to convert
	both operands to numeric types.
*/
jbas_error jbas_binary_math_op(jbas_env *env, jbas_token *a, jbas_token *b, jbas_token *res)
{
	// Convert both operands to numbers
	jbas_error err = JBAS_OK;
	err = jbas_token_to_number(env, a);
	if (err) return err;
	err = jbas_token_to_number(env, b);
	if (err) return err;

	// From now on, we can assume that the operands are numbers
	// Resulting token is set to be a number token as well
	jbas_number_token *an = &a->number_token, *bn = &b->number_token;
	res->type = JBAS_TOKEN_NUMBER;

	// Convert both operands and result to the same type resulting from promotion
	jbas_number_type prom_type = jbas_number_type_promotion(an->type, bn->type);
	jbas_number_cast(an, prom_type);
	jbas_number_cast(bn, prom_type);
	res->number_token.type = prom_type;
	
	return JBAS_OK;
}

jbas_error jbas_op_and(jbas_env *env, jbas_token *a, jbas_token *b, jbas_token *res)
{
	// Result type
	res->type = JBAS_TOKEN_NUMBER;
	res->number_token.type = JBAS_NUM_BOOL;

	// Convert both operands to numbers
	jbas_error err = JBAS_OK;
	err = jbas_token_to_number_type(env, a, JBAS_NUM_BOOL);
	if (err) return err;

	if (!a->number_token.i)
	{
		res->number_token.i = 0;
		return JBAS_OK;
	}

	err = jbas_token_to_number_type(env, b, JBAS_NUM_BOOL);
	if (err) return err;

	res->number_token.i = a->number_token.i && b->number_token.i;
	return JBAS_OK;
}

jbas_error jbas_op_or(jbas_env *env, jbas_token *a, jbas_token *b, jbas_token *res)
{
	// Result type
	res->type = JBAS_TOKEN_NUMBER;
	res->number_token.type = JBAS_NUM_BOOL;

	// Convert both operands to numbers
	jbas_error err = JBAS_OK;
	err = jbas_token_to_number_type(env, a, JBAS_NUM_BOOL);
	if (err) return err;

	if (a->number_token.i)
	{
		res->number_token.i = 1;
		return JBAS_OK;
	}

	err = jbas_token_to_number_type(env, b, JBAS_NUM_BOOL);
	if (err) return err;

	res->number_token.i = a->number_token.i || b->number_token.i;
	return JBAS_OK;
}

jbas_error jbas_op_eq(jbas_env *env, jbas_token *a, jbas_token *b, jbas_token *res)
{
	// For numbers
	return JBAS_OK;
}

jbas_error jbas_op_less(jbas_env *env, jbas_token *a, jbas_token *b, jbas_token *res)
{
	return JBAS_OK;
}

/**
	Based on < and = operators
*/
jbas_error jbas_op_neq(jbas_env *env, jbas_token *a, jbas_token *b, jbas_token *res)
{
	jbas_error err = jbas_op_eq(env, a, b, res);
	if (err) return err;
	res->number_token.i = !res->number_token.i;
	return JBAS_OK;
}

/**
	Based on < and = operators
*/
jbas_error jbas_op_greater(jbas_env *env, jbas_token *a, jbas_token *b, jbas_token *res)
{
	return jbas_op_less(env, b, a, res);
}

/**
	Based on < and = operators
*/
jbas_error jbas_op_leq(jbas_env *env, jbas_token *a, jbas_token *b, jbas_token *res)
{
	jbas_token tmp;
	jbas_error err = jbas_op_less(env, a, b, &tmp);
	if (err) return err;
	err = jbas_op_eq(env, a, b, res);
	if (err) return err;
	res->number_token.i = res->number_token.i || tmp.number_token.i;
	return JBAS_OK;
}

/**
	Based on < and = operators
*/
jbas_error jbas_op_geq(jbas_env *env, jbas_token *a, jbas_token *b, jbas_token *res)
{
	return jbas_op_leq(env, b, a, res);
}

jbas_error jbas_op_add(jbas_env *env, jbas_token *a, jbas_token *b, jbas_token *res)
{
	jbas_error err = jbas_binary_math_op(env, a, b, res);
	if (err) return err;

	if (res->number_token.type == JBAS_NUM_INT)
		res->number_token.i = a->number_token.i + b->number_token.i;
	else
		res->number_token.f = a->number_token.f + b->number_token.f;

	return JBAS_OK;
}

jbas_error jbas_op_sub(jbas_env *env, jbas_token *a, jbas_token *b, jbas_token *res)
{
	if (a && b) // Binary action
	{
		jbas_error err = jbas_binary_math_op(env, a, b, res);
		if (err) return err;

		if (res->number_token.type == JBAS_NUM_INT)
			res->number_token.i = a->number_token.i - b->number_token.i;
		else
			res->number_token.f = a->number_token.f - b->number_token.f;	
	}
	else if (!a && b) // Unary action
	{
		jbas_error err;
		err = jbas_token_to_number(env, b);
		if (err) return err;

		res->type = JBAS_TOKEN_NUMBER;
		res->number_token.type = b->number_token.type;

		if (b->number_token.type == JBAS_NUM_INT)
			res->number_token.i = -b->number_token.i;
		else
			res->number_token.f = -b->number_token.f;
	}

	return JBAS_OK;
}

jbas_error jbas_op_mul(jbas_env *env, jbas_token *a, jbas_token *b, jbas_token *res)
{
	jbas_error err = jbas_binary_math_op(env, a, b, res);
	if (err) return err;

	if (res->number_token.type == JBAS_NUM_INT)
		res->number_token.i = a->number_token.i * b->number_token.i;
	else
		res->number_token.f = a->number_token.f * b->number_token.f;

	return JBAS_OK;
}

jbas_error jbas_op_div(jbas_env *env, jbas_token *a, jbas_token *b, jbas_token *res)
{
	jbas_error err = jbas_binary_math_op(env, a, b, res);
	if (err) return err;

	if (res->number_token.type == JBAS_NUM_INT)
		res->number_token.i = a->number_token.i / b->number_token.i;
	else
		res->number_token.f = a->number_token.f / b->number_token.f;

	return JBAS_OK;
}

jbas_error jbas_op_mod(jbas_env *env, jbas_token *a, jbas_token *b, jbas_token *res)
{
	jbas_error err = jbas_binary_math_op(env, a, b, res);
	if (err) return err;

	if (res->number_token.type == JBAS_NUM_INT)
		res->number_token.i = a->number_token.i % b->number_token.i;
	else
		res->number_token.f = fmodf(a->number_token.f, b->number_token.f);

	return JBAS_OK;
}

jbas_error jbas_op_not(jbas_env *env, jbas_token *a, jbas_token *b, jbas_token *res)
{
	return JBAS_OK;
}

const jbas_operator jbas_operators[JBAS_OPERATOR_COUNT] = 
{
	// Assignment operators
	{.str = "=",   .level = 0, .type = JBAS_OP_BINARY_RL, .fallback = 0, .handler = jbas_op_assign},
	
	// Commas for making tuples
	{.str = ",",   .level = 1, .type = JBAS_OP_BINARY_LR, .fallback = 0, .handler = jbas_op_comma},

	// Binary logical operators
	{.str = "&&",  .level = 2, .type = JBAS_OP_BINARY_LR, .fallback = 0, .handler = jbas_op_and},
	{.str = "||",  .level = 2, .type = JBAS_OP_BINARY_LR, .fallback = 0, .handler = jbas_op_or},
	{.str = "AND", .level = 2, .type = JBAS_OP_BINARY_LR, .fallback = 0, .handler = jbas_op_and},
	{.str = "OR",  .level = 2, .type = JBAS_OP_BINARY_LR, .fallback = 0, .handler = jbas_op_or},

	// Comparison operators
	{.str = "==",  .level = 3, .type = JBAS_OP_BINARY_LR, .fallback = 0, .handler = jbas_op_eq},
	{.str = "!=",  .level = 3, .type = JBAS_OP_BINARY_LR, .fallback = 0, .handler = jbas_op_neq},
	{.str = "<",   .level = 3, .type = JBAS_OP_BINARY_LR, .fallback = 0, .handler = jbas_op_less},
	{.str = ">",   .level = 3, .type = JBAS_OP_BINARY_LR, .fallback = 0, .handler = jbas_op_greater},
	{.str = "<=",  .level = 3, .type = JBAS_OP_BINARY_LR, .fallback = 0, .handler = jbas_op_leq},
	{.str = ">=",  .level = 3, .type = JBAS_OP_BINARY_LR, .fallback = 0, .handler = jbas_op_geq},

	// Mathematical operators
	{.str = "+",   .level = 4, .type = JBAS_OP_BINARY_LR, .fallback = 0, .handler = jbas_op_add},
	{.str = "-",   .level = 4, .type = JBAS_OP_BINARY_LR, .handler = jbas_op_sub, .fallback = JBAS_OP_UNARY_PREFIX, .fallback_level = 5},
	{.str = "*",   .level = 5, .type = JBAS_OP_BINARY_LR, .fallback = 0, .handler = jbas_op_mul},
	{.str = "/",   .level = 5, .type = JBAS_OP_BINARY_LR, .fallback = 0, .handler = jbas_op_div},
	{.str = "%",   .level = 5, .type = JBAS_OP_BINARY_LR, .fallback = 0, .handler = jbas_op_mod},

	// Unary prefix operators
	{.str = "!",   .level = 6, .type = JBAS_OP_UNARY_PREFIX, .handler = jbas_op_not},
	{.str = "NOT", .level = 6, .type = JBAS_OP_UNARY_PREFIX, .handler = jbas_op_not},

};

int jbas_is_operator_char(char c)
{
	return isalpha(c) || strchr("=<>!,&|+-*/%", c);
}

