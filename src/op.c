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

	res->number_token.i = a->number_token.i && b->number_token.i;
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

	res->number_token.i = a->number_token.i || b->number_token.i;
	return JBAS_OK;
}

static jbas_error jbas_op_eq(jbas_env *env, jbas_token *a, jbas_token *b, jbas_token *res)
{
	// For numbers
	return JBAS_OK;
}

static jbas_error jbas_op_less(jbas_env *env, jbas_token *a, jbas_token *b, jbas_token *res)
{
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

	if (res->number_token.type == JBAS_NUM_INT)
		res->number_token.i = a->number_token.i + b->number_token.i;
	else
		res->number_token.f = a->number_token.f + b->number_token.f;

	return JBAS_OK;
}

static jbas_error jbas_op_sub(jbas_env *env, jbas_token *a, jbas_token *b, jbas_token *res)
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

static jbas_error jbas_op_mul(jbas_env *env, jbas_token *a, jbas_token *b, jbas_token *res)
{
	jbas_error err = jbas_binary_math_op(env, a, b, res);
	if (err) return err;

	if (res->number_token.type == JBAS_NUM_INT)
		res->number_token.i = a->number_token.i * b->number_token.i;
	else
		res->number_token.f = a->number_token.f * b->number_token.f;

	return JBAS_OK;
}

static jbas_error jbas_op_div(jbas_env *env, jbas_token *a, jbas_token *b, jbas_token *res)
{
	jbas_error err = jbas_binary_math_op(env, a, b, res);
	if (err) return err;

	if (res->number_token.type == JBAS_NUM_INT)
		res->number_token.i = a->number_token.i / b->number_token.i;
	else
		res->number_token.f = a->number_token.f / b->number_token.f;

	return JBAS_OK;
}

static jbas_error jbas_op_mod(jbas_env *env, jbas_token *a, jbas_token *b, jbas_token *res)
{
	jbas_error err = jbas_binary_math_op(env, a, b, res);
	if (err) return err;

	if (res->number_token.type == JBAS_NUM_INT)
		res->number_token.i = a->number_token.i % b->number_token.i;
	else
		res->number_token.f = fmodf(a->number_token.f, b->number_token.f);

	return JBAS_OK;
}

static jbas_error jbas_op_not(jbas_env *env, jbas_token *a, jbas_token *b, jbas_token *res)
{
	return JBAS_OK;
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
	{.str = "!",   .level = 6, .type = JBAS_OP_UNARY_PREFIX, .fallback = 0, .eval_args = 1, .handler = jbas_op_not},
	{.str = "NOT", .level = 6, .type = JBAS_OP_UNARY_PREFIX, .fallback = 0, .eval_args = 1, .handler = jbas_op_not},
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
bool jbas_is_binary_operator(jbas_token *t)
{
	return t && t->type == JBAS_TOKEN_OPERATOR && (t->operator_token.op->type == JBAS_OP_BINARY_LR || t->operator_token.op->type == JBAS_OP_BINARY_RL);  
}

/**
	True if provided token is an unary operator
*/
bool jbas_is_unary_operator(jbas_token *t)
{
	return t && t->type == JBAS_TOKEN_OPERATOR && (t->operator_token.op->type == JBAS_OP_UNARY_PREFIX || t->operator_token.op->type == JBAS_OP_UNARY_POSTFIX);  
}

/**
	Only symbols, numbers, strings, tuples and parenthesis are pure operands
*/
bool jbas_is_pure_operand(const jbas_token *t)
{
	if (!t) return false;
	return t->type == JBAS_TOKEN_SYMBOL
		|| t->type == JBAS_TOKEN_NUMBER
		|| t->type == JBAS_TOKEN_STRING
		|| t->type == JBAS_TOKEN_LPAREN
		|| t->type == JBAS_TOKEN_RPAREN
		|| t->type == JBAS_TOKEN_TUPLE;
}

/**
	Valid operands are pure operands with optional prefix/postfix operators
*/
bool jbas_is_valid_operand(jbas_token *t)
{
	if (!t) return false;
	return jbas_is_pure_operand(t) || (t->type == JBAS_TOKEN_OPERATOR
			&& (t->operator_token.op->type == JBAS_OP_UNARY_PREFIX
			|| t->operator_token.op->type == JBAS_OP_UNARY_POSTFIX));
}

/**
	Returns true if provided operator has left operand
*/
bool jbas_has_left_operand(jbas_token *t)
{
	return jbas_is_valid_operand(t->l) && t->l->type != JBAS_TOKEN_LPAREN
		&& (t->l->type != JBAS_TOKEN_OPERATOR || t->l->operator_token.op->type == JBAS_OP_UNARY_POSTFIX);
}

/**
	Returns true if provided operator has right operand
*/
bool jbas_has_right_operand(jbas_token *t)
{
	return jbas_is_valid_operand(t->r) && t->r->type != JBAS_TOKEN_RPAREN
		&& (t->r->type != JBAS_TOKEN_OPERATOR || t->r->operator_token.op->type == JBAS_OP_UNARY_PREFIX);
}

/**
	Removes operand - one token or entire parentheses
*/
jbas_error jbas_remove_operand(jbas_env *env, jbas_token *t)
{
	if (!t) return JBAS_OPERAND_MISSING;
	if (jbas_is_paren(t)) return jbas_remove_entire_paren(env, t);
	else return jbas_token_list_return_to_pool(t, &env->token_pool);
}

/**
	Prepares and evaluates any unary operation
*/
jbas_error jbas_eval_unary_operator(jbas_env *env, jbas_token *t)
{
	jbas_error err;
	jbas_token *operand = t->operator_token.op->type == JBAS_OP_UNARY_PREFIX ? t->r : t->l;
	if (!jbas_is_valid_operand(operand)) return JBAS_OPERAND_MISSING;
	if (jbas_is_paren(operand)) jbas_eval_paren(env, operand);
	err = t->operator_token.op->handler(env, operand == t->l ? t->l : NULL, operand == t->r ? t->r : NULL, t);
	if (err) return err;
	err = jbas_remove_operand(env, t->r);
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
		// Evaluate parenthesis
		if (t->operator_token.op->eval_args)
		{
			if (jbas_is_paren(t->l)) jbas_eval_paren(env, t->l);
			if (jbas_is_paren(t->r)) jbas_eval_paren(env, t->r);
		}

		// Call the operator handler and have it replaced with operation result
		t->operator_token.op->handler(env, t->l, t->r, t);

		// Remove operands
		jbas_error err;
		err = jbas_remove_operand(env, t->l);
		if (err) return err;
		err = jbas_remove_operand(env, t->r);
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
	JBAS_ERROR_REASON(env, "object is not callable!");
	return JBAS_BAD_CALL;

	// Eval args parentheses
	jbas_error err = jbas_eval_paren(env, args);
	if (err) return err;

	// Remove args after call
	err = jbas_remove_operand(env, args);
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

	if (at->type == JBAS_TOKEN_LPAREN) return -1;
	else if (bt->type == JBAS_TOKEN_LPAREN) return 1;
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
void jbas_try_fallback_operator(jbas_token *t, jbas_operator_type fallback_type)
{
	if (t->type != JBAS_TOKEN_OPERATOR) return;
	const jbas_operator *op = t->operator_token.op;

	if ((fallback_type == JBAS_OP_UNARY_PREFIX && op->type != fallback_type && !jbas_has_left_operand(t) && jbas_has_right_operand(t))
		|| (fallback_type == JBAS_OP_UNARY_POSTFIX && op->type != fallback_type && jbas_has_left_operand(t) && !jbas_has_right_operand(t)))
	{
		// Traverse fallback chain
		for (; op; op = op->fallback)
			if (op->type == fallback_type)
			{
				t->operator_token.op = op;
				break;
			}
	}
}
