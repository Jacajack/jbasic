#include <jbasic/op.h>
#include <jbasic/jbasic.h>
#include <jbasic/paren.h>
#include <jbasic/cast.h>

/*
	TOKEN OPERATIONS MAY *NOT* INVALIDATE ITERATOR (POINTER)
	POINTING TO THE OPERATOR ITSELF
*/

static jbas_error jbas_op_assign(jbas_env *env, jbas_token *a, jbas_token *b, jbas_token *res)
{
	// Two tuples case
	if (a->type == JBAS_TOKEN_TUPLE && b->type == JBAS_TOKEN_TUPLE)
	{
		jbas_token *t = jbas_token_list_begin(a->tuple_token.tokens);
		jbas_token *u = jbas_token_list_begin(b->tuple_token.tokens);
		for (; t && u; t = t->r, u = u->r)
		{
			jbas_error err = jbas_op_assign(env, t, u, NULL);
			if (err) return err;
		}

		if (res) return jbas_token_move(res, b, &env->token_pool);
		else return JBAS_OK;
	}

	if (a->type != JBAS_TOKEN_SYMBOL)
	{
		JBAS_ERROR_REASON(env, "cannot assign value to non-symbol token");
		return JBAS_BAD_ASSIGN;
	}
	jbas_symbol *asym = a->symbol_token.sym;
	jbas_resource *dest;

	// Decrement reference count on the resource currently kept in the dest. variable
	if (asym->res) jbas_resource_remove_ref(asym->res);

	// If there's no destination resource, create it
	if (!asym->res) jbas_resource_create(&env->resource_manager, &asym->res);
	dest = asym->res;

	
	switch (b->type)
	{
		// If the source is a symbol, copy the resource and we're done
		case JBAS_TOKEN_SYMBOL:
			jbas_resource_copy(dest, b->symbol_token.sym->res);
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

	if (res) return jbas_token_move(res, b, &env->token_pool);
	else return JBAS_OK;
}

static jbas_error jbas_op_comma(jbas_env *env, jbas_token *a, jbas_token *b, jbas_token *res)
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
		jbas_error err;
		err = jbas_token_move(res, a, &env->token_pool);
		if (err) return err;

		err = jbas_token_list_push_back_from_pool(res->tuple_token.tokens, &env->token_pool, b, &res->tuple_token.tokens);
		if (err) return err;
		return JBAS_OK;
	}

	// Tuple on the right side
	if (a->type != JBAS_TOKEN_TUPLE && b->type == JBAS_TOKEN_TUPLE)
	{
		// Copy the tuple on the right side
		jbas_error err;
		err = jbas_token_move(res, b, &env->token_pool);
		if (err) return err;

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
		jbas_error err = jbas_token_move(res, a, &env->token_pool);
		if (err) return err;

		return JBAS_OK;
	}

	return JBAS_OK;
}

/**
	Takes two tokens as operands and one for the result.
	Type promotions are performed after an attempt to convert
	both operands to numeric types.
*/
static jbas_error jbas_binary_math_op(jbas_env *env, jbas_token *a, jbas_token *b, jbas_token *res)
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

static jbas_error jbas_op_and(jbas_env *env, jbas_token *a, jbas_token *b, jbas_token *res)
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

	res->number_token.i = b->number_token.i != 0;
	return JBAS_OK;
}

static jbas_error jbas_op_or(jbas_env *env, jbas_token *a, jbas_token *b, jbas_token *res)
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

	res->number_token.i = b->number_token.i != 0;
	return JBAS_OK;
}

static jbas_error jbas_op_eq(jbas_env *env, jbas_token *a, jbas_token *b, jbas_token *res)
{
	// For numbers
	if (jbas_can_cast_to_number(a) && jbas_can_cast_to_number(b))
	{
		jbas_error err = jbas_binary_math_op(env, a, b, res);
		if (err) return err;

		if (res->number_token.type == JBAS_NUM_FLOAT) // Floats
		{
			res->number_token.type = JBAS_NUM_BOOL;
			res->number_token.i = a->number_token.f == b->number_token.f;
		}
		else // Ints and bools
		{
			res->number_token.type = JBAS_NUM_BOOL;
			res->number_token.i = a->number_token.i == b->number_token.i;
		}
	}
	else
	{
		JBAS_ERROR_REASON(env, "attempted to compare (==) incomparable entities");
		return JBAS_BAD_COMPARE;
	}

	return JBAS_OK;
}

static jbas_error jbas_op_less(jbas_env *env, jbas_token *a, jbas_token *b, jbas_token *res)
{
	// For numbers
	if (jbas_can_cast_to_number(a) && jbas_can_cast_to_number(b))
	{
		jbas_error err = jbas_binary_math_op(env, a, b, res);
		if (err) return err;

		if (res->number_token.type == JBAS_NUM_FLOAT) // Floats
		{
			res->number_token.type = JBAS_NUM_BOOL;
			res->number_token.i = a->number_token.f < b->number_token.f;
		}
		else // Ints and bools
		{
			res->number_token.type = JBAS_NUM_BOOL;
			res->number_token.i = a->number_token.i < b->number_token.i;
		}
	}
	else
	{
		JBAS_ERROR_REASON(env, "attempted to compare (<) incomparable entities");
		return JBAS_BAD_COMPARE;
	}

	return JBAS_OK;
}

/**
	Based on < and = operators
*/
static jbas_error jbas_op_neq(jbas_env *env, jbas_token *a, jbas_token *b, jbas_token *res)
{
	jbas_error err = jbas_op_eq(env, a, b, res);
	if (err) return err;
	res->number_token.i = !res->number_token.i;
	return JBAS_OK;
}

/**
	Based on < and = operators
*/
static jbas_error jbas_op_greater(jbas_env *env, jbas_token *a, jbas_token *b, jbas_token *res)
{
	return jbas_op_less(env, b, a, res);
}

/**
	Based on < and = operators
*/
static jbas_error jbas_op_leq(jbas_env *env, jbas_token *a, jbas_token *b, jbas_token *res)
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
static jbas_error jbas_op_geq(jbas_env *env, jbas_token *a, jbas_token *b, jbas_token *res)
{
	return jbas_op_leq(env, b, a, res);
}

static jbas_error jbas_op_add(jbas_env *env, jbas_token *a, jbas_token *b, jbas_token *res)
{
	jbas_error err = jbas_binary_math_op(env, a, b, res);
	if (err) return err;

	if (res->number_token.type == JBAS_NUM_FLOAT)
		res->number_token.f = a->number_token.f + b->number_token.f;
	else
		res->number_token.i = a->number_token.i + b->number_token.i;

	return JBAS_OK;
}

static jbas_error jbas_op_sub(jbas_env *env, jbas_token *a, jbas_token *b, jbas_token *res)
{
	if (a && b) // Binary action
	{
		jbas_error err = jbas_binary_math_op(env, a, b, res);
		if (err) return err;

		if (res->number_token.type == JBAS_NUM_FLOAT)
			res->number_token.f = a->number_token.f - b->number_token.f;	
		else
			res->number_token.i = a->number_token.i - b->number_token.i;
	}
	else if (!a && b) // Unary action
	{
		jbas_error err;
		err = jbas_token_to_number(env, b);
		if (err) return err;

		res->type = JBAS_TOKEN_NUMBER;
		res->number_token.type = b->number_token.type;

		if (b->number_token.type == JBAS_NUM_FLOAT)
			res->number_token.f = -b->number_token.f;
		else
			res->number_token.i = -b->number_token.i;
	}

	return JBAS_OK;
}

static jbas_error jbas_op_mul(jbas_env *env, jbas_token *a, jbas_token *b, jbas_token *res)
{
	jbas_error err = jbas_binary_math_op(env, a, b, res);
	if (err) return err;

	if (res->number_token.type == JBAS_NUM_FLOAT)
		res->number_token.f = a->number_token.f * b->number_token.f;
	else
		res->number_token.i = a->number_token.i * b->number_token.i;

	return JBAS_OK;
}

static jbas_error jbas_op_div(jbas_env *env, jbas_token *a, jbas_token *b, jbas_token *res)
{
	jbas_error err = jbas_binary_math_op(env, a, b, res);
	if (err) return err;

	if (res->number_token.type == JBAS_NUM_FLOAT)
		res->number_token.f = a->number_token.f / b->number_token.f;
	else
		res->number_token.i = a->number_token.i / b->number_token.i;

	return JBAS_OK;
}

static jbas_error jbas_op_mod(jbas_env *env, jbas_token *a, jbas_token *b, jbas_token *res)
{
	jbas_error err = jbas_binary_math_op(env, a, b, res);
	if (err) return err;

	if (res->number_token.type == JBAS_NUM_FLOAT)
		res->number_token.f = fmodf(a->number_token.f, b->number_token.f);
	else
		res->number_token.i = a->number_token.i % b->number_token.i;

	return JBAS_OK;
}

static jbas_error jbas_op_not(jbas_env *env, jbas_token *a, jbas_token *b, jbas_token *res)
{
	// Result type
	res->type = JBAS_TOKEN_NUMBER;
	res->number_token.type = JBAS_NUM_BOOL;

	// Convert operands to bool
	jbas_error err = jbas_token_to_number_type(env, b, JBAS_NUM_BOOL);
	if (err)
	{
		JBAS_ERROR_REASON(env, "invalid NOT operand");
		return err;
	}

	res->number_token.i = !b->number_token.i;
	return JBAS_OK;
}


static jbas_error jbas_op_print(jbas_env *env, jbas_token *a, jbas_token *b, jbas_token *res)
{
	switch (b->type)
	{
		// Eval symbol and try again
		case JBAS_TOKEN_SYMBOL:
			{
				if (jbas_is_scalar_symbol(b))
				{
					jbas_error err = jbas_eval_scalar_symbol(env, b);
					if (err)
					{
						if (err == JBAS_UNINITIALIZED_SYMBOL)
							jbas_printf(env, "<NULL>");
						else return err;
					}
					else return jbas_op_print(env, a, b, res);
				}
				else
				{
					jbas_printf(env, "<NONSCALAR>");
				}
			}
			break;

		// Print a number
		case JBAS_TOKEN_NUMBER:
			{
				jbas_number_token *n = &b->number_token;
				if (n->type == JBAS_NUM_INT)
					jbas_printf(env, "%d", n->i);
				else if (n->type == JBAS_NUM_BOOL)
					jbas_printf(env, n->i ? "TRUE" : "FALSE");
				else
					jbas_printf(env, "%f", n->f);
			}
			break;

	
		// Print a constant string
		case JBAS_TOKEN_STRING:
			jbas_printf(env, "%s", b->string_token.txt->str);
			break;

		default:
			jbas_printf(env, "???");
			break;
	}

	return jbas_token_move(res, b, &env->token_pool);
}

static jbas_error jbas_op_println(jbas_env *env, jbas_token *a, jbas_token *b, jbas_token *res)
{
	jbas_error err = jbas_op_print(env, a, b, res);
	jbas_printf(env, "\n");
	return err;
}

static jbas_error jbas_op_input(jbas_env *env, jbas_token *a, jbas_token *b, jbas_token *res)
{
	return jbas_token_move(res, b, &env->token_pool);
}

// ---- END OF OPERATOR IMPLEMENTATIONS


/**
	Alternative minus sign operation
*/
static const jbas_operator jbas_op_neg_def = {.str = "-", .level = 6, .type = JBAS_OP_UNARY_PREFIX, .fallback = 0, .eval_args = 1, .handler = jbas_op_sub};

/**
	Operator table
*/
const jbas_operator jbas_operators[JBAS_OPERATOR_COUNT] = 
{
	// Assignment operators
	{.str = "=",   .level = 0, .type = JBAS_OP_BINARY_RL, .fallback = 0, .eval_args = 1, .handler = jbas_op_assign},
	
	// Commas for making tuples
	{.str = ",",   .level = 1, .type = JBAS_OP_BINARY_LR, .fallback = 0, .eval_args = 1, .handler = jbas_op_comma},

	// Binary logical operators
	{.str = "&&",  .level = 2, .type = JBAS_OP_BINARY_LR, .fallback = 0, .eval_args = 0, .handler = jbas_op_and},
	{.str = "||",  .level = 2, .type = JBAS_OP_BINARY_LR, .fallback = 0, .eval_args = 0, .handler = jbas_op_or},
	{.str = "AND", .level = 2, .type = JBAS_OP_BINARY_LR, .fallback = 0, .eval_args = 0, .handler = jbas_op_and},
	{.str = "OR",  .level = 2, .type = JBAS_OP_BINARY_LR, .fallback = 0, .eval_args = 0, .handler = jbas_op_or},

	// Comparison operators
	{.str = "==",  .level = 3, .type = JBAS_OP_BINARY_LR, .fallback = 0, .eval_args = 1, .handler = jbas_op_eq},
	{.str = "!=",  .level = 3, .type = JBAS_OP_BINARY_LR, .fallback = 0, .eval_args = 1, .handler = jbas_op_neq},
	{.str = "<",   .level = 3, .type = JBAS_OP_BINARY_LR, .fallback = 0, .eval_args = 1, .handler = jbas_op_less},
	{.str = ">",   .level = 3, .type = JBAS_OP_BINARY_LR, .fallback = 0, .eval_args = 1, .handler = jbas_op_greater},
	{.str = "<=",  .level = 3, .type = JBAS_OP_BINARY_LR, .fallback = 0, .eval_args = 1, .handler = jbas_op_leq},
	{.str = ">=",  .level = 3, .type = JBAS_OP_BINARY_LR, .fallback = 0, .eval_args = 1, .handler = jbas_op_geq},

	// Mathematical operators
	{.str = "+",   .level = 4, .type = JBAS_OP_BINARY_LR, .fallback = 0, .eval_args = 1, .handler = jbas_op_add},
	{.str = "-",   .level = 4, .type = JBAS_OP_BINARY_LR, .fallback = &jbas_op_neg_def, .eval_args = 1, .handler = jbas_op_sub},
	{.str = "*",   .level = 5, .type = JBAS_OP_BINARY_LR, .fallback = 0, .eval_args = 1, .handler = jbas_op_mul},
	{.str = "/",   .level = 5, .type = JBAS_OP_BINARY_LR, .fallback = 0, .eval_args = 1, .handler = jbas_op_div},
	{.str = "%",   .level = 5, .type = JBAS_OP_BINARY_LR, .fallback = 0, .eval_args = 1, .handler = jbas_op_mod},

	// Unary prefix operators
	{.str = "!",       .level = 6, .type = JBAS_OP_UNARY_PREFIX, .fallback = 0, .eval_args = 1, .handler = jbas_op_not},
	{.str = "NOT",     .level = 6, .type = JBAS_OP_UNARY_PREFIX, .fallback = 0, .eval_args = 1, .handler = jbas_op_not},
	{.str = "PRINT",   .level = 6, .type = JBAS_OP_UNARY_PREFIX, .fallback = 0, .eval_args = 1, .handler = jbas_op_print},
	{.str = "PRINTLN", .level = 6, .type = JBAS_OP_UNARY_PREFIX, .fallback = 0, .eval_args = 1, .handler = jbas_op_println},
	{.str = "INPUT",   .level = 6, .type = JBAS_OP_UNARY_PREFIX, .fallback = 0, .eval_args = 1, .handler = jbas_op_input},
};

int jbas_is_operator_char(char c)
{
	return isalpha(c) || strchr("=<>!,&|+-*/%", c);
}

/**
	Returns a pointer to operator definition
*/
const jbas_operator *jbas_get_operator_by_str(const char *b, const char *e)
{
	for (int i = 0; i < JBAS_OPERATOR_COUNT; i++)
	{
		if (!jbas_namecmp(jbas_operators[i].str, NULL, b, e))
			return &jbas_operators[i];
	}

	return NULL;
}

/**
	True if provided token is an binary operator
*/
bool jbas_is_binary_operator(const jbas_token *t)
{
	return t && t->type == JBAS_TOKEN_OPERATOR && (t->operator_token.op->type == JBAS_OP_BINARY_LR || t->operator_token.op->type == JBAS_OP_BINARY_RL);  
}

/**
	True if provided token is an unary operator
*/
bool jbas_is_unary_operator(const jbas_token *t)
{
	return t && t->type == JBAS_TOKEN_OPERATOR && (t->operator_token.op->type == JBAS_OP_UNARY_PREFIX || t->operator_token.op->type == JBAS_OP_UNARY_POSTFIX);  
}

/**
	Returns true if an operator can be on the left side of its operand
*/
bool jbas_can_be_prefix_operator(const jbas_token *t)
{
	if (!t) return false;

	if (t->type != JBAS_TOKEN_OPERATOR) return false;
	const jbas_operator *op = t->operator_token.op;

	// A prefix operator
	if (op->type == JBAS_OP_UNARY_PREFIX) return true;

	// A binary operator that has no left operand and can serve as prefix operator
	return jbas_is_binary_operator(t) && !jbas_has_left_operand(t) &&
		(op->fallback && op->fallback->type == JBAS_OP_UNARY_PREFIX);
}

/**
	Returns true if an operator can be on the right side of its operand
*/
bool jbas_can_be_postfix_operator(const jbas_token *t)
{
	if (!t) return false;

	// A special case - the call operator
	if (t->type == JBAS_TOKEN_PAREN && jbas_is_operand(t->l)) return true;

	// Other operators
	if (t->type != JBAS_TOKEN_OPERATOR) return false;
	const jbas_operator *op = t->operator_token.op;

	// A postfix operator
	if (op->type == JBAS_OP_UNARY_POSTFIX) return true;

	// A binary operator that has no right operand and can serve as postfix operator
	return jbas_is_binary_operator(t) && !jbas_has_right_operand(t) &&
		(op->fallback && op->fallback->type == JBAS_OP_UNARY_POSTFIX);
}

/**
	Only symbols, numbers, strings, tuples and parenthesis are pure operands.

	Prefix and suffix operators are not pure operands.
	Parenthesis are pure operands only if they do not have any *valid*
	operands on their left.
*/
bool jbas_is_pure_operand(const jbas_token *t)
{
	if (!t) return false;
	return t->type == JBAS_TOKEN_SYMBOL
		|| t->type == JBAS_TOKEN_NUMBER
		|| t->type == JBAS_TOKEN_STRING
		|| t->type == JBAS_TOKEN_TUPLE
		|| t->type == JBAS_TOKEN_RESOURCE
		|| (t->type == JBAS_TOKEN_PAREN && !jbas_has_left_operand(t));
}

/**
	Valid operands are pure operands with their prefix/postfix operators.
*/
bool jbas_is_operand(const jbas_token *t)
{
	if (!t) return false;
	return jbas_is_pure_operand(t) || jbas_is_paren(t) || (t->type == JBAS_TOKEN_OPERATOR
			&& (t->operator_token.op->type == JBAS_OP_UNARY_PREFIX
			|| t->operator_token.op->type == JBAS_OP_UNARY_POSTFIX));
}

/**
	Returns true if provided operator has left operand
*/
bool jbas_has_left_operand(const jbas_token *t)
{
	return jbas_is_operand(t->l) 
		&& (t->l->type != JBAS_TOKEN_OPERATOR || t->l->operator_token.op->type == JBAS_OP_UNARY_POSTFIX);
}

/**
	Returns true if provided operator has right operand
*/
bool jbas_has_right_operand(const jbas_token *t)
{
	return jbas_is_operand(t->r) 
		&& (t->r->type != JBAS_TOKEN_OPERATOR || t->r->operator_token.op->type == JBAS_OP_UNARY_PREFIX);
}

/**
	Removes operand - token with all its prefix and postfix operators
*/
jbas_error jbas_remove_operand(jbas_env *env, jbas_token *t)
{
	bool has_left = true, has_right = true;

	// Move the t pointer to the actual operand
	if (!jbas_is_pure_operand(t))
	{
		while (jbas_can_be_postfix_operator(t)) t = t->l;
		while (jbas_can_be_prefix_operator(t)) t = t->r;
		if (!jbas_is_pure_operand(t))
		{
			JBAS_ERROR_REASON(env, "could not find pure operand inside an operand (in remove)");
			return JBAS_OK;
		}
	}

	// Eval operators
	while (has_left || has_right)
	{
		has_left = has_left && jbas_can_be_prefix_operator(t->l);
		has_right = has_right && jbas_can_be_postfix_operator(t->r);

		jbas_error err = JBAS_OK;
		if (has_left) err = jbas_token_list_return_to_pool(t->l, &env->token_pool);
		if (err) return err;
		if (has_right) err = jbas_token_list_return_to_pool(t->r, &env->token_pool);
		if (err) return err;
	}

	// Remove the token itself
	return jbas_token_list_return_to_pool(t, &env->token_pool);
}

/**
	Evaluates all operators prefix and postfix attached to the operand.
	Also evaluates parentheses.
	\note The operand pointer is not invalidated
*/
jbas_error jbas_eval_operand(jbas_env *env, jbas_token *t, jbas_token **operand)
{
	bool has_left = true, has_right = true;
	int do_lr = 0;

	// Move the t pointer to the actual operand
	if (!jbas_is_pure_operand(t))
	{
		while (jbas_can_be_postfix_operator(t)) t = t->l;
		while (jbas_can_be_prefix_operator(t)) t = t->r;
		if (!jbas_is_pure_operand(t))
		{
			JBAS_ERROR_REASON(env, "could not find pure operand inside an operand (in eval)");
			return JBAS_OK;
		}
	}

	// Eval the parentheses first
	if (t->type == JBAS_TOKEN_PAREN)
	{
		jbas_error err = jbas_eval_paren(env, t);
		if (err) return err;
	}

	// Eval operators
	while (1)
	{
		has_left = has_left && jbas_can_be_prefix_operator(t->l);
		has_right = has_right && jbas_can_be_postfix_operator(t->r);
		if (!has_left && !has_right) break;

		// The call operator is always evaluated first
		if (has_right && t->r->type == JBAS_TOKEN_PAREN)
		{
			jbas_error err = jbas_eval_call_operator(env, t, t->r);
			if (err) return err;
			continue;
		}

		// Do left or right?
		do_lr = has_right - has_left;

		// Both operators?
		do_lr = do_lr ? do_lr : t->l->operator_token.op->level - t->r->operator_token.op->level;

		// If they have the same precedence, do right one first
		do_lr = do_lr ? do_lr : 1;

		// Eval
		jbas_error err = JBAS_OK;
		if (do_lr > 0) err = jbas_eval_unary_operator(env, t->r);
		if (do_lr < 0) err = jbas_eval_unary_operator(env, t->l);
		if (err) return err;
	}

	if (operand) *operand = t;

	return JBAS_OK;
}

/**
	Prepares and evaluates any unary operation. Only handles pure operands
	because the right order of evaluation is assured by jbas_eval_operand()
*/
jbas_error jbas_eval_unary_operator(jbas_env *env, jbas_token *t)
{
	jbas_error err;
	jbas_token *operand = t->operator_token.op->type == JBAS_OP_UNARY_PREFIX ? t->r : t->l;
	if (!jbas_is_pure_operand(operand)) return JBAS_OPERAND_MISSING;

	// Replace the operand with result
	err = t->operator_token.op->handler(env, operand == t->l ? t->l : NULL, operand == t->r ? t->r : NULL, operand);
	if (err) return err;

	// Remove the operator
	err = jbas_token_list_return_to_pool(t, &env->token_pool);
	if (err) return err;

	return JBAS_OK;
}

/**
	Prepares and evaluates any binary operation
*/
jbas_error jbas_eval_binary_operator(jbas_env *env, jbas_token *t)
{
	if (jbas_has_left_operand(t) && jbas_has_right_operand(t))
	{
		jbas_error err = JBAS_OK;
		
		// Evaluate operands
		if (t->operator_token.op->eval_args) err = jbas_eval_operand(env, t->l, NULL);
		if (err) return err;
		if (t->operator_token.op->eval_args) err = jbas_eval_operand(env, t->r, NULL);
		if (err) return err;

		// Call the operator handler and get the result
		jbas_token res;
		err = t->operator_token.op->handler(env, t->l, t->r, &res);
		if (err) return err;

		// Temporarily replace the operator with a delimiter
		// Otherwise the operator can be treated as prefix operator
		// and removed together with the right operand
		t->type = JBAS_TOKEN_DELIMITER;

		// Remove operands (before we replace the operator with
		// resulting token that could potentially be a pure operand)
		err = jbas_remove_operand(env, t->l);
		if (err) return err;
		err = jbas_remove_operand(env, t->r);
		if (err) return err;

		// Replace the operator (delimiter) with the result
		err = jbas_token_move(t, &res, &env->token_pool);
		if (err) return err;
	}
	else
		return JBAS_OPERAND_MISSING;
	return JBAS_OK;
}

/**
	Evaluates a call operation
*/
jbas_error jbas_eval_call_operator(jbas_env *env, jbas_token *fun, jbas_token *args)
{
	// Extract symbol resource
	jbas_error err = jbas_symbol_to_resource(env, fun);
	if (err) return err;

	// Eval arguments
	err = jbas_eval_paren(env, args);
	if (err) return err;

	// Only resources and tuples are callable
	jbas_token ret;
	if (fun->type == JBAS_TOKEN_RESOURCE)
	{
		jbas_resource *res = fun->resource_token.res;
		switch (res->type)
		{
			// Call a C function
			case JBAS_RESOURCE_CFUN:
				{
					jbas_error err = res->cfun(env, args, &ret);
					if (err) return err;		
				}
				break;

			default:
				JBAS_ERROR_REASON(env, "resource is not callable");
				return JBAS_BAD_CALL;
				break;
		}
		
	}
	else if (fun->type == JBAS_TOKEN_TUPLE)
	{
		// Get index
		err = jbas_token_to_number_type(env, args, JBAS_NUM_INT);
		if (err)
		{
			JBAS_ERROR_REASON(env, "invalid tuple index (not a number?)");
			return JBAS_BAD_INDEX;
		}

		int n = args->number_token.i;
		if (n < 0)
		{
			JBAS_ERROR_REASON(env, "invalid tuple index (negative)");
			return JBAS_BAD_INDEX;
		}

		jbas_token *t;
		for (t = jbas_token_list_begin(fun->tuple_token.tokens); t && n; t = t->r, n--);
		if (n || !t)
		{
			JBAS_ERROR_REASON(env, "invalid tuple index (out of range)");
			return JBAS_BAD_INDEX;
		}

		err = jbas_token_move(&ret, t, &env->token_pool);
		if (err) return err;
	}
	else
	{
		JBAS_ERROR_REASON(env, "entity is not callable!");
		return JBAS_BAD_CALL;
	}

	// Remove args
	// No need for tweaks, because we simply delete the token
	err = jbas_token_list_return_to_pool(args, &env->token_pool);
	if (err) return err;

	// Nicely replace the function symbol with the operation result
	err = jbas_token_move(fun, &ret, &env->token_pool);
	if (err) return err;

	return JBAS_OK;
}


/**
	Operator token comparison function for qsort.
	Operators that should be evaluated first will 
	appear at the beginning of the sorted array.

	Precedence type and associativity and position in the expression are all taken into account.
*/
int jbas_operator_token_compare(const void *av, const void *bv)
{
	const jbas_operator_sort_bucket *ab = (const jbas_operator_sort_bucket*)av;
	const jbas_operator_sort_bucket *bb = (const jbas_operator_sort_bucket*)bv;
	const jbas_token *at = ab->token;
	const jbas_token *bt = bb->token;
	int ap = ab->pos;
	int bp = bb->pos;

	if (at->type == JBAS_TOKEN_PAREN) return -1;
	else if (bt->type == JBAS_TOKEN_PAREN) return 1;
	else
	{
		const jbas_operator *aop = at->operator_token.op;
		const jbas_operator *bop = bt->operator_token.op;

		// Precedence level
		if (aop->level == bop->level)
		{
			// Type/associativity
			if (aop->type == bop->type)
			{
				// Sort by position in the expression
				if (aop->type == JBAS_OP_UNARY_POSTFIX || aop->type == JBAS_OP_BINARY_LR) 
					return ap - bp; // Left to right
				else
					return bp - ap; // Right to left
			}
			else return aop->type - bop->type; // Different type/associativity
		}
		else return bop->level - aop->level; // Different precedence level
	}
}

/**
	Attempts to replace provided operator with any of its fallback opertors that would
	fit the environment
*/
void jbas_try_fallback_operator(jbas_token *t)
{
	if (t->type != JBAS_TOKEN_OPERATOR) return;
	const jbas_operator *op = t->operator_token.op;
	if (!op->fallback) return;

	if (op->fallback->type == JBAS_OP_UNARY_PREFIX && !jbas_has_left_operand(t) && jbas_has_right_operand(t))
		t->operator_token.op = op->fallback;

	if (op->fallback->type == JBAS_OP_UNARY_POSTFIX && jbas_has_left_operand(t) && !jbas_has_right_operand(t))
		t->operator_token.op = op->fallback;
}


/**
	Searches provided token's neighborhood for prefix and postfix operators.
	All binary operators that can fall back to being unary are converted to
	unary operators.
*/
void jbas_attach_unary_operators(jbas_token *operand)
{
	for (jbas_token *t = operand->r; jbas_can_be_postfix_operator(t); t = t->r)
		jbas_try_fallback_operator(t);

	for (jbas_token *t = operand->l; jbas_can_be_prefix_operator(t); t = t->l)
		jbas_try_fallback_operator(t);
}