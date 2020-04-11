#ifndef JBASIC_H
#define JBASIC_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdio.h>
#include <inttypes.h>
#include <ctype.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <math.h>

/*
	=== JBASIC ===
	
	1. Token types
		- Keyword
		- Variable/sub name
		- Number
		- String
	
	2. Keywords
		- PRINT
*/

#define JBAS_COLOR_RED "\x1b[31m"
#define JBAS_COLOR_GREEN "\x1b[32m"
#define JBAS_COLOR_YELLOW "\x1b[33m"
#define JBAS_COLOR_BLUE "\x1b[34m"
#define JBAS_COLOR_MAGENTA "\x1b[35m"
#define JBAS_COLOR_CYAN "\x1b[36m"
#define JBAS_COLOR_RESET "\x1b[0m"

typedef enum
{
	JBAS_OK = 0,
	JBAS_SYNTAX_UNMATCHED_QUOTE,
	JBAS_PRINT_BAD_ARGUMENT,
	JBAS_EMPTY_TOKEN,
	JBAS_TOKEN_POOL_EMPTY,
	JBAS_TOKEN_POOL_OVERFLOW,
	JBAS_ALLOC,
	JBAS_TEXT_MANAGER_OVERFLOW,
	JBAS_TEXT_MANAGER_MISMATCH,
	JBAS_RESOURCE_MANAGER_NOT_EMPTY,
	JBAS_RESOURCE_MANAGER_OVERFLOW,
	JBAS_SYMBOL_MANAGER_OVERFLOW,
	JBAS_SYMBOL_COLLISION,
	JBAS_TYPE_MISMATCH,
	JBAS_CAST_FAILED,
	JBAS_SYNTAX_UNMATCHED_PARENTHESIS,
	JBAS_CANNOT_REMOVE_PARENTHESIS,
	JBAS_OPERAND_MISSING,
	JBAS_UNINITIALIZED_SYMBOL, // When evaluated symbol links to no resource
	JBAS_CANNOT_EVAL_RESOURCE, // When you try to eval an e.g. an array symbol
	JBAS_BAD_ASSIGN,
} jbas_error;

typedef enum
{
 	JBAS_TOKEN_SYMBOL,
 	JBAS_TOKEN_KEYWORD,
 	JBAS_TOKEN_OPERATOR,
 	JBAS_TOKEN_LPAREN,
 	JBAS_TOKEN_RPAREN,
 	JBAS_TOKEN_NUMBER,
	JBAS_TOKEN_STRING,
	JBAS_TOKEN_COMMA,
	JBAS_TOKEN_DELIMITER,
} jbas_token_type;

typedef enum
{
	JBAS_KW_PRINT,
	JBAS_KW_IF,
	JBAS_KW_THEN,
	JBAS_KW_ENDIF
} jbas_keyword;


static const struct {const char *name; jbas_keyword id;} jbas_keywords[] = 
{
	{"PRINT", JBAS_KW_PRINT},
	{"IF", JBAS_KW_IF},
	{"ENDIF", JBAS_KW_ENDIF},
	{"THEN", JBAS_KW_THEN},
};
#define JBAS_KEYWORD_COUNT ((sizeof(jbas_keywords)) / (sizeof(jbas_keywords[0])))

/*
	Token structures
*/

typedef int jbas_int;
typedef float jbas_float;

typedef struct
{
	char *str;
	size_t length;
} jbas_text;

typedef struct
{
	jbas_text *text_storage;
	bool *is_used;
	int *free_slots;
	int free_slot_count;
	int max_count;
} jbas_text_manager;



typedef enum
{
	JBAS_RESOURCE_NUMBER,
	JBAS_RESOURCE_INT_ARRAY,
	JBAS_RESOURCE_FLOAT_ARRAY,
	JBAS_RESOURCE_STRING,
} jbas_resource_type;


typedef enum
{
	JBAS_NUM_BOOL,
	JBAS_NUM_INT,
	JBAS_NUM_FLOAT,
} jbas_number_type;

typedef struct
{
	jbas_number_type type;
	union
	{
		jbas_int i;
		jbas_float f;
	};
} jbas_number_token;

typedef struct jbas_resource_manager_s jbas_resource_manager;

typedef struct
{
	int rm_index;

	jbas_resource_type type;
	int ref_count;

	size_t size;
	
	union
	{
		jbas_number_token number;
		char *str;
		void *data;
	};
	
} jbas_resource;

struct jbas_resource_manager_s
{
	jbas_resource **refs;
	int ref_count;
	int max_count;
};



/**
	Symbols are links between names in the code and resoruces
*/
typedef struct
{
	jbas_text *name;
	jbas_resource *resource;
} jbas_symbol;

/*
	Symbol manager translates text ids to 
*/
typedef struct
{
	jbas_symbol *symbol_storage;
	bool *is_used;
	int *free_slots;	
	int free_slot_count;
	int max_count;
} jbas_symbol_manager;




typedef struct jbas_operator_s jbas_operator;

typedef struct
{
	jbas_text *txt;
} jbas_string_token;


typedef struct
{
	const jbas_operator *op;
} jbas_operator_token;

typedef struct
{
	jbas_keyword id;
} jbas_keyword_token;

typedef struct
{
	jbas_symbol *sym;
} jbas_symbol_token;

/**
	Polymorphic token
*/
typedef struct jbas_token_s
{
	jbas_token_type type;
	const char *begin, *end;

	union
	{
		jbas_keyword_token keyword_token;
		jbas_operator_token operator_token;
		jbas_number_token number_token;
		jbas_string_token string_token;
		jbas_symbol_token symbol_token;
	} u;

	// For bidirectional linking
	struct jbas_token_s *l, *r;

} jbas_token;


/**
	Pool with unused list nodes
*/
typedef struct 
{
	jbas_token *tokens;
	jbas_token **unused_stack;
	int pool_size;
	int unused_count;
} jbas_token_pool;




typedef enum
{
	JBAS_OP_UNDEFINED = 0,
	JBAS_OP_UNARY_PREFIX,
	JBAS_OP_UNARY_SUFFIX,
	JBAS_OP_UNARY_POSTFIX = JBAS_OP_UNARY_SUFFIX,
	JBAS_OP_BINARY_LR,
	JBAS_OP_BINARY_RL,
} jbas_operator_type;


/**
	Environment for BASIC program execution
*/
typedef struct
{
	jbas_token_pool token_pool; //!< Common token pool
	jbas_text_manager text_manager;
	jbas_resource_manager resource_manager;
	jbas_symbol_manager symbol_manager;
	jbas_token *tokens; //!< Tokenized program
} jbas_env;


struct jbas_operator_s
{
	const char *str;
	int level;
	jbas_operator_type type;
	jbas_error (*handler)(jbas_env *env, jbas_token *a, jbas_token *b, jbas_token *res);
	
	jbas_operator_type fallback; // Can binary operator become an unary one?
	int fallback_level;
};


// Forward declarations

jbas_error jbas_token_list_return_to_pool(
	jbas_token **list_handle,
	jbas_token_pool *pool);

jbas_error jbas_eval(jbas_env *env, jbas_token *const begin, jbas_token *const end);
jbas_error jbas_resource_remove_ref(jbas_resource *res);
jbas_error jbas_resource_add_ref(jbas_resource *res);
void jbas_resource_copy(jbas_resource *dest, jbas_resource *src);
jbas_error jbas_resource_create(jbas_resource_manager *rm, jbas_resource **res);



void jbas_token_copy(jbas_token *dest, jbas_token *src)
{
	jbas_token *l = dest->l, *r = dest->r;
	*dest = *src;
	dest->l = l;
	dest->r = r;
}

/**
	Removes parenthesis if there's only one token inside.
	\note Provided list pointer remains valid as in other operations
*/
jbas_error jbas_remove_parenthesis(jbas_env *env, jbas_token *t)
{
	if (t->type == JBAS_TOKEN_LPAREN)
	{
		if (!t->r) return JBAS_SYNTAX_UNMATCHED_PARENTHESIS;
		jbas_token *r = t->r->r;
		if (r->type != JBAS_TOKEN_RPAREN) return JBAS_CANNOT_REMOVE_PARENTHESIS;
		
		// Replace left parentheses with the value from the inside
		jbas_token_copy(t, t->r);

		jbas_token *h;
		jbas_error err;

		h = t->r;
		err = jbas_token_list_return_to_pool(&h, &env->token_pool);
		if (err) return err;

		h = t->r;
		err = jbas_token_list_return_to_pool(&h, &env->token_pool);
		if (err) return err;
	}
	else if (t->type == JBAS_TOKEN_RPAREN)
	{
		if (!t->l) return JBAS_SYNTAX_UNMATCHED_PARENTHESIS;
		jbas_token *l = t->l->l;
		if (l->type != JBAS_TOKEN_LPAREN) return JBAS_CANNOT_REMOVE_PARENTHESIS;
		
		// Replace right parentheses with the value from the inside
		jbas_token_copy(t, t->l);

		jbas_token *h;
		jbas_error err;

		h = t->l;
		err = jbas_token_list_return_to_pool(&h, &env->token_pool);
		if (err) return err;

		h = t->l;
		err = jbas_token_list_return_to_pool(&h, &env->token_pool);
		if (err) return err;
	}

	return JBAS_OK;
}

jbas_error jbas_find_matching_parenthesis(jbas_token *t, jbas_token **match)
{
	if (t->type == JBAS_TOKEN_LPAREN)
	{
		int level = 0;
		for (t = t->r; t && (t->type != JBAS_TOKEN_RPAREN || level); t = t->r )
		{
			level += t->type == JBAS_TOKEN_LPAREN;
			level -= t->type == JBAS_TOKEN_RPAREN;
		}

		if (!t) return JBAS_SYNTAX_UNMATCHED_PARENTHESIS;
		*match = t;
		return JBAS_OK;
	}
	else if (t->type == JBAS_TOKEN_RPAREN)
	{
		int level = 0;
		for (t = t->l; t && (t->type != JBAS_TOKEN_LPAREN || level); t = t->l )
		{
			level += t->type == JBAS_TOKEN_RPAREN;
			level -= t->type == JBAS_TOKEN_LPAREN;
		}

		if (!t) return JBAS_SYNTAX_UNMATCHED_PARENTHESIS;
		*match = t;
		return JBAS_OK;
	}

	return JBAS_OK;
}

/**
	Evaluates everything inside the parenthesis (and removes it)
*/
jbas_error jbas_eval_parenthesis(jbas_env *env, jbas_token *t)
{
	jbas_token *begin, *end, *match;
	jbas_error err;

	err = jbas_find_matching_parenthesis(t, &match);
	if (err) return err;

	if (t->type == JBAS_TOKEN_LPAREN)
	{
		begin = t->r;
		end = match;
	}
	else
	{
		begin = match->r;
		end = t;
	}

	err = jbas_eval(env, begin, end);
	if (err) return err;
	
	return jbas_remove_parenthesis(env, t);
}

/*
	TOKEN OPERATIONS MAY *NOT* INVALIDATE ITERATOR (POINTER)
	POINTING TO THE OPERATOR ITSELF
*/

jbas_number_type jbas_number_type_promotion(jbas_number_type a, jbas_number_type b)
{
	if (a >= b) return a;
	else return b;
}

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
	Returns true if provided token is a scalar
*/
bool jbas_is_scalar(jbas_token *t)
{
	if (t->type != JBAS_TOKEN_SYMBOL) return true;
	else
	{
		jbas_resource *res = t->u.symbol_token.sym->resource;
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

/*
	Evaluates a scalar symbol
*/
jbas_error jbas_eval_scalar_symbol(jbas_env *env, jbas_token *t)
{
	if (t->type != JBAS_TOKEN_SYMBOL) return JBAS_OK;
	if (!jbas_is_scalar(t)) return JBAS_OK;
	jbas_symbol *sym = t->u.symbol_token.sym;
	jbas_token res;

	if (!sym->resource) return JBAS_UNINITIALIZED_SYMBOL;
	switch (sym->resource->type)
	{
		case JBAS_RESOURCE_NUMBER:
			res.type = JBAS_TOKEN_NUMBER;
			res.u.number_token = sym->resource->number;
			break;

		default:
			return JBAS_CANNOT_EVAL_RESOURCE;
			break;
	}

	jbas_token_copy(t, &res);
	return JBAS_OK;
}

jbas_error jbas_token_to_number(jbas_env *env, jbas_token *t)
{
	if (t->type == JBAS_TOKEN_NUMBER) return JBAS_OK;
	else if (t->type == JBAS_TOKEN_LPAREN || t->type == JBAS_TOKEN_RPAREN)
	{
		jbas_error err;
		err = jbas_eval_parenthesis(env, t);
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

	return JBAS_CAST_FAILED;
}

jbas_error jbas_token_to_number_type(jbas_env *env, jbas_token *t, jbas_number_type type)
{
	jbas_error err = jbas_token_to_number(env, t);
	if (err) return err;
	jbas_number_cast(&t->u.number_token, type);
	return JBAS_OK;
}




jbas_error jbas_op_assign(jbas_env *env, jbas_token *a, jbas_token *b, jbas_token *res)
{
	if (a->type != JBAS_TOKEN_SYMBOL) return JBAS_BAD_ASSIGN;
	jbas_symbol *asym = a->u.symbol_token.sym;
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
			jbas_resource_copy(dest, b->u.symbol_token.sym->resource);
			break;

		// Number assignment
		case JBAS_TOKEN_NUMBER:
			dest->type = JBAS_RESOURCE_NUMBER;
			dest->number = b->u.number_token;
			break;

		default:
			return JBAS_BAD_ASSIGN;
			break;

	}

	jbas_token_copy(res, b);
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
	jbas_number_token *an = &a->u.number_token, *bn = &b->u.number_token;
	res->type = JBAS_TOKEN_NUMBER;

	// Convert both operands and result to the same type resulting from promotion
	jbas_number_type prom_type = jbas_number_type_promotion(an->type, bn->type);
	jbas_number_cast(an, prom_type);
	jbas_number_cast(bn, prom_type);
	res->u.number_token.type = prom_type;
	
	return JBAS_OK;
}

jbas_error jbas_op_and(jbas_env *env, jbas_token *a, jbas_token *b, jbas_token *res)
{
	// Result type
	res->type = JBAS_TOKEN_NUMBER;
	res->u.number_token.type = JBAS_NUM_BOOL;

	// Convert both operands to numbers
	jbas_error err = JBAS_OK;
	err = jbas_token_to_number_type(env, a, JBAS_NUM_BOOL);
	if (err) return err;

	if (!a->u.number_token.i)
	{
		res->u.number_token.i = 0;
		return JBAS_OK;
	}

	err = jbas_token_to_number_type(env, b, JBAS_NUM_BOOL);
	if (err) return err;

	res->u.number_token.i = a->u.number_token.i && b->u.number_token.i;
	return JBAS_OK;
}

jbas_error jbas_op_or(jbas_env *env, jbas_token *a, jbas_token *b, jbas_token *res)
{
	// Result type
	res->type = JBAS_TOKEN_NUMBER;
	res->u.number_token.type = JBAS_NUM_BOOL;

	// Convert both operands to numbers
	jbas_error err = JBAS_OK;
	err = jbas_token_to_number_type(env, a, JBAS_NUM_BOOL);
	if (err) return err;

	if (a->u.number_token.i)
	{
		res->u.number_token.i = 1;
		return JBAS_OK;
	}

	err = jbas_token_to_number_type(env, b, JBAS_NUM_BOOL);
	if (err) return err;

	res->u.number_token.i = a->u.number_token.i || b->u.number_token.i;
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
	res->u.number_token.i = !res->u.number_token.i;
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
	res->u.number_token.i = res->u.number_token.i || tmp.u.number_token.i;
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

	if (res->u.number_token.type == JBAS_NUM_INT)
		res->u.number_token.i = a->u.number_token.i + b->u.number_token.i;
	else
		res->u.number_token.f = a->u.number_token.f + b->u.number_token.f;

	return JBAS_OK;
}

jbas_error jbas_op_sub(jbas_env *env, jbas_token *a, jbas_token *b, jbas_token *res)
{
	if (a && b) // Binary action
	{
		jbas_error err = jbas_binary_math_op(env, a, b, res);
		if (err) return err;

		if (res->u.number_token.type == JBAS_NUM_INT)
			res->u.number_token.i = a->u.number_token.i - b->u.number_token.i;
		else
			res->u.number_token.f = a->u.number_token.f - b->u.number_token.f;	
	}
	else if (!a && b) // Unary action
	{
		jbas_error err;
		err = jbas_token_to_number(env, b);
		if (err) return err;

		res->type = JBAS_TOKEN_NUMBER;
		res->u.number_token.type = b->u.number_token.type;

		if (b->u.number_token.type == JBAS_NUM_INT)
			res->u.number_token.i = -b->u.number_token.i;
		else
			res->u.number_token.f = -b->u.number_token.f;
	}

	return JBAS_OK;
}

jbas_error jbas_op_mul(jbas_env *env, jbas_token *a, jbas_token *b, jbas_token *res)
{
	jbas_error err = jbas_binary_math_op(env, a, b, res);
	if (err) return err;

	if (res->u.number_token.type == JBAS_NUM_INT)
		res->u.number_token.i = a->u.number_token.i * b->u.number_token.i;
	else
		res->u.number_token.f = a->u.number_token.f * b->u.number_token.f;

	return JBAS_OK;
}

jbas_error jbas_op_div(jbas_env *env, jbas_token *a, jbas_token *b, jbas_token *res)
{
	jbas_error err = jbas_binary_math_op(env, a, b, res);
	if (err) return err;

	if (res->u.number_token.type == JBAS_NUM_INT)
		res->u.number_token.i = a->u.number_token.i / b->u.number_token.i;
	else
		res->u.number_token.f = a->u.number_token.f / b->u.number_token.f;

	return JBAS_OK;
}

jbas_error jbas_op_mod(jbas_env *env, jbas_token *a, jbas_token *b, jbas_token *res)
{
	jbas_error err = jbas_binary_math_op(env, a, b, res);
	if (err) return err;

	if (res->u.number_token.type == JBAS_NUM_INT)
		res->u.number_token.i = a->u.number_token.i % b->u.number_token.i;
	else
		res->u.number_token.f = fmodf(a->u.number_token.f, b->u.number_token.f);

	return JBAS_OK;
}

jbas_error jbas_op_not(jbas_env *env, jbas_token *a, jbas_token *b, jbas_token *res)
{
	return JBAS_OK;
}

/**
	Returns true if provided operator has left operand
*/
bool jbas_has_left_operand(jbas_token *t)
{
	if (t->type != JBAS_TOKEN_OPERATOR) return false;
	if (!t->l) return false;
	return t->l->type == JBAS_TOKEN_SYMBOL
		|| t->l->type == JBAS_TOKEN_NUMBER
		|| t->l->type == JBAS_TOKEN_STRING
		|| t->l->type == JBAS_TOKEN_RPAREN;
}

jbas_error jbas_remove_left_operand(jbas_env *env, jbas_token *t)
{
	if (!t->l) return JBAS_OPERAND_MISSING;
	if (t->l->type == JBAS_TOKEN_RPAREN) // Remove entire parenthesis
	{
		// Remove the parentheses
		jbas_token *h = t->l;
		jbas_error err = jbas_token_list_return_to_pool(&h, &env->token_pool);
		if (err) return err;

		// Remove everything up to the matching parentheses
		int level = 0;
		for (h = t->l; h && (h->type != JBAS_TOKEN_LPAREN || level); h = t->l)
		{
			level += h->type == JBAS_TOKEN_RPAREN;
			level -= h->type == JBAS_TOKEN_LPAREN;
			jbas_error err = jbas_token_list_return_to_pool(&h, &env->token_pool);
			if (err) return err;
		}

		if (!h) return JBAS_SYNTAX_UNMATCHED_PARENTHESIS;
		h = t->l;
		return jbas_token_list_return_to_pool(&h, &env->token_pool);	
	}
	else
	{
		jbas_token *h = t->l;
		return jbas_token_list_return_to_pool(&h, &env->token_pool);	
	}
	
	return JBAS_OK;
}

/**
	Returns true if provided operator has right operand
*/
bool jbas_has_right_operand(jbas_token *t)
{
	if (t->type != JBAS_TOKEN_OPERATOR) return false;
	if (!t->r) return false;
	return t->r->type == JBAS_TOKEN_SYMBOL
		|| t->r->type == JBAS_TOKEN_NUMBER
		|| t->r->type == JBAS_TOKEN_STRING
		|| t->r->type == JBAS_TOKEN_LPAREN;
}


jbas_error jbas_remove_right_operand(jbas_env *env, jbas_token *t)
{
	if (!t->r) return JBAS_OPERAND_MISSING;
	if (t->r->type == JBAS_TOKEN_LPAREN) // Remove entire parenthesis
	{
		// Remove the parentheses
		jbas_token *h = t->r;
		jbas_error err = jbas_token_list_return_to_pool(&h, &env->token_pool);
		if (err) return err;

		// Remove everything up to the matching parentheses
		int level = 0;
		for (h = t->r; h && (h->type != JBAS_TOKEN_RPAREN || level); h = t->r)
		{
			level += h->type == JBAS_TOKEN_LPAREN;
			level -= h->type == JBAS_TOKEN_RPAREN;
			jbas_error err = jbas_token_list_return_to_pool(&h, &env->token_pool);
			if (err) return err;
		}

		if (!h) return JBAS_SYNTAX_UNMATCHED_PARENTHESIS;
		h = t->r;
		return jbas_token_list_return_to_pool(&h, &env->token_pool);	
	}
	else
	{
		jbas_token *h = t->r;
		return jbas_token_list_return_to_pool(&h, &env->token_pool);	
	}
	
	return JBAS_OK;
}

static const jbas_operator jbas_operators[] = 
{
	// Assignment operators
	{.str = "=",   .level = 0, .type = JBAS_OP_BINARY_RL, .handler = jbas_op_assign},
	
	// Binary logical operators
	{.str = "&&",  .level = 1, .type = JBAS_OP_BINARY_LR, .fallback = 0, .handler = jbas_op_and},
	{.str = "||",  .level = 1, .type = JBAS_OP_BINARY_LR, .fallback = 0, .handler = jbas_op_or},
	{.str = "AND", .level = 1, .type = JBAS_OP_BINARY_LR, .fallback = 0, .handler = jbas_op_and},
	{.str = "OR",  .level = 1, .type = JBAS_OP_BINARY_LR, .fallback = 0, .handler = jbas_op_or},

	// Comparison operators
	{.str = "==",  .level = 2, .type = JBAS_OP_BINARY_LR, .fallback = 0, .handler = jbas_op_eq},
	{.str = "!=",  .level = 2, .type = JBAS_OP_BINARY_LR, .fallback = 0, .handler = jbas_op_neq},
	{.str = "<",   .level = 2, .type = JBAS_OP_BINARY_LR, .fallback = 0, .handler = jbas_op_less},
	{.str = ">",   .level = 2, .type = JBAS_OP_BINARY_LR, .fallback = 0, .handler = jbas_op_greater},
	{.str = "<=",  .level = 2, .type = JBAS_OP_BINARY_LR, .fallback = 0, .handler = jbas_op_leq},
	{.str = ">=",  .level = 2, .type = JBAS_OP_BINARY_LR, .fallback = 0, .handler = jbas_op_geq},

	// Mathematical operators
	{.str = "+",   .level = 3, .type = JBAS_OP_BINARY_LR, .fallback = 0, .handler = jbas_op_add},
	{.str = "-",   .level = 3, .type = JBAS_OP_BINARY_LR, .handler = jbas_op_sub, .fallback = JBAS_OP_UNARY_PREFIX, .fallback_level = 5},
	{.str = "*",   .level = 4, .type = JBAS_OP_BINARY_LR, .fallback = 0, .handler = jbas_op_mul},
	{.str = "/",   .level = 4, .type = JBAS_OP_BINARY_LR, .fallback = 0, .handler = jbas_op_div},
	{.str = "%",   .level = 4, .type = JBAS_OP_BINARY_LR, .fallback = 0, .handler = jbas_op_mod},

	// Unary prefix operators
	{.str = "!",   .level = 5, .type = JBAS_OP_UNARY_PREFIX, .handler = jbas_op_not},
	{.str = "NOT", .level = 5, .type = JBAS_OP_UNARY_PREFIX, .handler = jbas_op_not},

};
#define JBAS_OPERATOR_COUNT ((sizeof(jbas_operators)) / (sizeof(jbas_operators[0])))

int jbas_is_operator_char(char c)
{
	return isalpha(c) || strchr("=<>!&|+-*/%", c);
}

#define JBAS_MAX_OPERATOR_LEVEL 5




#ifdef JBASIC_IMPLEMENTATION


int jbas_namecmp(const char *s1, const char *end1, const char *s2, const char *end2)
{
	if (!end1 && !end2) return strcasecmp(s1, s2);
	if (end1 && end2)
	{
		size_t l1 = end1 - s1;
		size_t l2 = end2 - s2;

		if (l1 != l2) return (end1 - s1) - (end2 - s2);
		else return strncasecmp(s1, s2, l1);
	}

	size_t l1, l2;
	if (end1)
	{
		l1 = end1 - s1;
		l2 = strlen(s2);
		if (l1 != l2) return (end1 - s1) - (end2 - s2);
		else return strncasecmp(s1, s2, l1);
	}
	else
	{
		l1 = strlen(s1);
		l2 = end2 - s2;
		if (l1 != l2) return (end1 - s1) - (end2 - s2);
		else return strncasecmp(s1, s2, l1);
	}
}


jbas_error jbas_text_manager_init(jbas_text_manager *tm, int text_count)
{
	tm->max_count = text_count;
	tm->free_slot_count = text_count;

	// Allocate memory
	tm->text_storage = calloc(text_count, sizeof(jbas_text));
	tm->free_slots = calloc(text_count, sizeof(int));
	tm->is_used = calloc(text_count, sizeof(bool));

	// Handle calloc errors
	if (!tm->text_storage || !tm->free_slots || !tm->is_used)
	{
		free(tm->text_storage);
		free(tm->free_slots);
		free(tm->is_used);
		return JBAS_ALLOC;
	}

	for (int i = 0; i < tm->free_slot_count; i++)
		tm->free_slots[i] = i;

	return JBAS_OK;
}

jbas_error jbas_text_create(jbas_text_manager *tm, const char *s, const char *end, jbas_text **txt)
{
	if (!tm->free_slot_count) return JBAS_TEXT_MANAGER_OVERFLOW;

	int slot = tm->free_slots[--tm->free_slot_count];
	jbas_text *t = tm->text_storage + slot;
	tm->is_used[slot] = true;

	// Copy provided string
	if (end)
	{
		t->str = calloc(end - s + 1, sizeof(char));
		t->length = end - s;
		memcpy(t->str, s, t->length);
	}
	else
	{
		t->str = strdup(s);
		t->length = strlen(s);
	}


	// Return a pointer to the new text
	*txt = t;

	return JBAS_OK;
}

jbas_error jbas_text_lookup(jbas_text_manager *tm, const char *s, const char *end, jbas_text **txt)
{
	for (int i = 0; i < tm->max_count; i++)
	{
		if (!tm->is_used[i]) continue;
		if (end)
		{
			if (end - s == tm->text_storage[i].length && !strncmp(s, tm->text_storage[i].str, end - s))
			{
				*txt = &tm->text_storage[i];
				return JBAS_OK;
			}
		}
		else
		{
			if (!strncmp(s, tm->text_storage[i].str, s - end))
			{
				*txt = &tm->text_storage[i];
				return JBAS_OK;
			}
		}
	}

	*txt = NULL;

	return JBAS_OK;
}

jbas_error jbas_text_lookup_create(jbas_text_manager *tm, const char *s, const char *end, jbas_text **txt)
{
	jbas_text *t;
	jbas_error err = jbas_text_lookup(tm, s, end, &t);
	if (err) return err;
	if (!t) err = jbas_text_create(tm, s, end, &t);
	*txt = t;
	return err;
}

jbas_error jbas_text_destroy(jbas_text_manager *tm, jbas_text *txt)
{
	if (!txt) return JBAS_OK;

	int slot = txt - tm->text_storage;

	// Check if the text is managed by this text manager
	if (slot > tm->max_count) return JBAS_TEXT_MANAGER_MISMATCH;

	// Actually delete the stored text
	free(txt->str);
	txt->str = NULL;
	tm->is_used[slot] = false;

	// Free up the slot
	tm->free_slots[tm->free_slot_count++] = txt - tm->text_storage;
	
	return JBAS_OK;
}


void jbas_text_manager_destroy(jbas_text_manager *tm)
{
	// Destroy all the stored texts first
	for (int i = 0; i < tm->max_count; i++)
		if (tm->is_used[i])
			jbas_text_destroy(tm, &tm->text_storage[i]);

	free(tm->text_storage);
	free(tm->free_slots);
	free(tm->is_used);
}













/*
	Resources are created dynamically during the program execution.
	Garbage collection is performed automatically as well
*/


jbas_error jbas_resource_manager_init(jbas_resource_manager *rm, int max_count)
{
	rm->max_count = max_count;
	rm->refs = calloc(max_count, sizeof(jbas_resource*));
	rm->ref_count = 0;

	if (!rm->refs)
		return JBAS_ALLOC;

	return JBAS_OK;
}

/**
	Frees memory used by the resource and removes it from the resource manager
*/
void jbas_resource_delete(jbas_resource_manager *rm, jbas_resource *res)
{
	switch (res->type)
	{
		case JBAS_RESOURCE_NUMBER:
		default:
			break;
	}

	// Update resource manager refs
	jbas_resource *m = rm->refs[--rm->ref_count];
	rm->refs[res->rm_index] = m;
	m->rm_index = res->rm_index;

	free(res);
}

/**
	Frees all resources and the resource manager memory
*/
void jbas_resource_manager_destroy(jbas_resource_manager *rm)
{
	for (int i = 0; i < rm->ref_count; i++)
		jbas_resource_delete(rm, rm->refs[i]);
	free(rm->refs);
}

/**
	Deletes resources that have no references
*/
jbas_error jbas_resource_manager_garbage_collect(jbas_resource_manager *rm)
{
	for (int i = 0; i < rm->ref_count; i++)
	{
		if (rm->refs[i]->ref_count == 0)
			jbas_resource_delete(rm, rm->refs[i]);
		
		rm->refs[i] = rm->refs[--rm->ref_count];
	}

	return JBAS_OK;
}

/**
	Removes a reference to the resource
*/
jbas_error jbas_resource_remove_ref(jbas_resource *res)
{
	if (res->ref_count) res->ref_count--;
	return JBAS_OK;
}

/**
	Increments reference counter
*/
jbas_error jbas_resource_add_ref(jbas_resource *res)
{
	res->ref_count++;
	return JBAS_OK;
}

/**
	Create a resource and register it in the resource_manager
*/
jbas_error jbas_resource_create(jbas_resource_manager *rm, jbas_resource **res)
{
	jbas_resource *r = calloc(1, sizeof(jbas_resource));
	if (!r) return JBAS_ALLOC;
	
	r->ref_count = 1;

	// Register in the resource manager
	int index = rm->ref_count++;
	if (index >= rm->max_count) return JBAS_RESOURCE_MANAGER_OVERFLOW;
	r->rm_index = index;
	rm->refs[index] = r;


	*res = r;
	return JBAS_OK;
}

/**
	Copies all resource data apart from the reference counter
*/
void jbas_resource_copy(jbas_resource *dest, jbas_resource *src)
{
	int ref_count = dest->ref_count;
	*dest = *src;
	dest->ref_count = ref_count;
}



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
	Returns true or false depending on whether the character
	can be a part of program variable or sub
*/
int jbas_is_name_char(char c)
{
	return isalpha(c) || (c == '_');
}


const char *jbas_debug_keyword_str(jbas_keyword id)
{
	for (int i = 0; i < JBAS_KEYWORD_COUNT; i++)
		if (jbas_keywords[i].id == id)
			return jbas_keywords[i].name;
	return NULL;
}


/**
	Dumps token to stderr
*/
void jbas_debug_dump_token(FILE *f, const jbas_token *token)
{	
	fprintf(f, " ");

	if (!token)
	{
		fprintf(f, JBAS_COLOR_RED "<NULL>" JBAS_COLOR_RESET);
		return;
	}

	switch (token->type)
	{
		case JBAS_TOKEN_KEYWORD:
			fprintf(f, JBAS_COLOR_BLUE "%s" JBAS_COLOR_RESET, jbas_debug_keyword_str(token->u.keyword_token.id));
			break;

		case JBAS_TOKEN_STRING:
			fprintf(f, JBAS_COLOR_GREEN "'%s'" JBAS_COLOR_RESET, token->u.string_token.txt->str);
			break;

		case JBAS_TOKEN_NUMBER:
			if (token->u.number_token.type == JBAS_NUM_INT)
				fprintf(f, JBAS_COLOR_RED "%d" JBAS_COLOR_RESET, token->u.number_token.i);
			else if (token->u.number_token.type == JBAS_NUM_BOOL)
				fprintf(f, JBAS_COLOR_RED "%s" JBAS_COLOR_RESET, token->u.number_token.i ? "TRUE" : "FALSE");
			else
				fprintf(f, JBAS_COLOR_RED "%f" JBAS_COLOR_RESET, token->u.number_token.f);
			break;

		case JBAS_TOKEN_OPERATOR:
			fprintf(f, JBAS_COLOR_YELLOW "%s" JBAS_COLOR_RESET, token->u.operator_token.op->str);
			break;

		case JBAS_TOKEN_DELIMITER:
			fprintf(f, ";\n");
			break;

		case JBAS_TOKEN_LPAREN:
		case JBAS_TOKEN_RPAREN:
			fprintf(f, JBAS_COLOR_CYAN "%c" JBAS_COLOR_RESET, token->type == JBAS_TOKEN_LPAREN ? '(' : ')');
			break;

		case JBAS_TOKEN_SYMBOL:
			fprintf(f, JBAS_COLOR_RESET "%s" JBAS_COLOR_RESET, token->u.symbol_token.sym->name->str);
			break;

		default:
			fprintf(f, "(%d?)", token->type );
			break;
	}
}

void jbas_debug_dump_resource(FILE *f, jbas_resource *res)
{

}

void jbas_debug_dump_symbol(FILE *f, jbas_symbol *sym)
{
	fprintf(f, "`%s` ", sym->name->str);
	jbas_debug_dump_resource(f, sym->resource);
}

jbas_error jbas_print(const jbas_token *token, int count)
{
	while (count--)
	{
		switch (token->type)
		{
			// Print a string
			case JBAS_TOKEN_STRING:
			{
				const jbas_string_token *t = &token->u.string_token;
				printf("%s", t->txt->str);
			}
			break;

			default:
				return JBAS_PRINT_BAD_ARGUMENT;
				break;
		}
		
		if (count) printf("\t");
		token++;
	}

	printf("\n");
	return JBAS_OK;
}

/**
	Returns next token from a code line.
	If there are no more tokens in the current line, address of the next token is returned as NULL

	\todo split this function into more, each handling one type of tokens
*/
jbas_error jbas_get_token(jbas_env *env, const char *const str, const char **next, jbas_token *token)
{
	const char *s = str;

	// Skip preceding whitespace
	while (*s && isspace(*s) && *s != '\n') s++;
	if (!*s)
	{
		*next = NULL;
		return JBAS_EMPTY_TOKEN;
	}

	token->begin = s;

	// If it's ';', it's a delimiter
	if (*s == ';' || *s == '\n')
	{
		*next = s + 1;
		token->type = JBAS_TOKEN_DELIMITER;
		return JBAS_OK;
	}

	// If the token starts with a number, it is a number
	if (isdigit(*s))
	{
		const char *num_end = s;
		char is_int = 1;
		
		// Consume the number
		while (*num_end && isdigit(*num_end)) num_end++;
		if (*num_end && *num_end == '.')
		{
			is_int = 0;
			num_end++;
		}
		while (*num_end && isdigit(*num_end)) num_end++;
		if (*num_end && toupper(*num_end) == 'E')
		{
			is_int = 0;
			num_end++;
		}
		if (*num_end && (*num_end == '-' || *num_end == '+')) num_end++;
		while (*num_end && isdigit(*num_end)) num_end++;

		// TODO use custom interpreter
		token->u.number_token.type = is_int ? JBAS_NUM_INT : JBAS_NUM_FLOAT;
		if (is_int)
			sscanf(s, "%d", &token->u.number_token.i);
		else
			sscanf(s, "%f", &token->u.number_token.f);

		*next = num_end;
		token->type = JBAS_TOKEN_NUMBER;
		return JBAS_OK;		
	}

	// If the token starts with ', " or `, it is a string
	if (*s == '\'' || *s == '\"' || *s == '`')
	{
		const char delimiter = *s;
		const char *str_end = s + 1;
		while (*str_end && *str_end != delimiter) str_end++;
		
		// If we reached end of the line
		if (!*str_end)
		{
			return JBAS_SYNTAX_UNMATCHED_QUOTE;
		}

		token->end = *next = str_end + 1;
		token->type = JBAS_TOKEN_STRING;

		jbas_error err = jbas_text_create(
			&env->text_manager,
			s + 1,
			str_end,
			&token->u.string_token.txt);

		return err;
	}

	// Left parenthesis
	if (*s == '(')
	{
		token->end = *next = s + 1;
		token->type = JBAS_TOKEN_LPAREN;
		return JBAS_OK;
	}

	// Right parenthesis
	if (*s == ')')
	{
		token->end = *next = s + 1;
		token->type = JBAS_TOKEN_RPAREN;
		return JBAS_OK;
	}

	// An operator
	const char *operator_end = s;
	while (jbas_is_operator_char(*operator_end)) operator_end++;
	*next = operator_end;

	for (int i = 0; i < JBAS_OPERATOR_COUNT; i++)
	{
		if (!jbas_namecmp(s, operator_end, jbas_operators[i].str, NULL))
		{
			token->type = JBAS_TOKEN_OPERATOR;
			token->u.operator_token.op = &jbas_operators[i];
			return JBAS_OK;
		}
	}

	// A symbol/keyword
	const char *name_end = s;
	while (jbas_is_name_char(*name_end)) name_end++;
	*next = name_end;
	
	// Keyword check
	for (int i = 0; i < JBAS_KEYWORD_COUNT; i++)
	{
		if (!jbas_namecmp(s, name_end, jbas_keywords[i].name, NULL))
		{
			// Found matching keyword
			token->type = JBAS_TOKEN_KEYWORD;
			token->u.keyword_token.id = jbas_keywords[i].id;
			return JBAS_OK;
		}
	}

	// It must be a symbol token
	{
		token->type = JBAS_TOKEN_SYMBOL;	
		jbas_symbol *sym;
		jbas_error err;
		
		err = jbas_symbol_create(env, &sym, s, name_end);
		if (err == JBAS_OK || err == JBAS_SYMBOL_COLLISION)
			token->u.symbol_token.sym = sym;
		else
			return err;
		
		
	}


	// Todo failed token!!!!

	return JBAS_OK;
}



// -------------------------------------- TOKEN POOL

jbas_error jbas_token_pool_get(
	jbas_token_pool *pool,
	jbas_token **t)
{
	if (!pool->unused_count) return JBAS_TOKEN_POOL_EMPTY;
	*t = pool->unused_stack[--pool->unused_count];
	// fprintf(stderr, "\ngot %p from pool\n", *t);
	return JBAS_OK;
}

jbas_error jbas_token_pool_return(
	jbas_token_pool *pool,
	jbas_token *t)
{
	if (pool->unused_count >= pool->pool_size) return JBAS_TOKEN_POOL_OVERFLOW;
	pool->unused_stack[pool->unused_count++] = t;
	// fprintf(stderr, "\nreturned %p to pool\n", t);
	return JBAS_OK;
}


jbas_error jbas_token_pool_init(jbas_token_pool *pool, int size)
{
	pool->pool_size = pool->unused_count = size;
	pool->tokens = calloc(size, sizeof(jbas_token));
	pool->unused_stack = calloc(size, sizeof(jbas_token*));
	
	if (!pool->tokens || !pool->unused_stack)
	{
		free(pool->tokens);
		free(pool->unused_stack);
		return JBAS_ALLOC;
	}

	for (int i = 0; i < size; i++)
	{
		pool->unused_stack[i] = &pool->tokens[i];
		pool->tokens[i].l = pool->tokens[i].r = NULL;
	}

	return JBAS_OK;
}

jbas_error jbas_token_pool_destroy(jbas_token_pool *pool)
{
	free(pool->tokens);
	free(pool->unused_stack);
	return JBAS_ALLOC;
}

// --------------------------------------


/**
	Returns a pointer to the first element of the list
*/
jbas_token *jbas_token_list_begin(jbas_token *t)
{
	while (t && t->l)
		t = t->l;
	return t;
}

/**
	Returns a pointer to the last element of the list
*/
jbas_token *jbas_token_list_end(jbas_token *t)
{
	while (t && t->r)
		t = t->r;
	return t;
}

/**
	Insert a new element into a token list. The new token is inserted after
	the element pointed by `after` pointer.

	Pointer to the newly inserted element is optionally returned through the `inserted` pointer.
*/
jbas_error jbas_token_list_insert_from_pool(
	jbas_token *after,
	jbas_token_pool *pool,
	jbas_token *token,
	jbas_token **inserted)
{
	jbas_token *t;
	jbas_error err = jbas_token_pool_get(pool, &t);
	if (err) return err;
	
	*t = *token;

	// Insert into empty list?
	if (after == NULL)
	{
		t->l = NULL;
		t->r = NULL;
		if (inserted) *inserted = t;
		return JBAS_OK;
	}
	else
	{
		t->l = after;
		t->r = after->r;
		if (after->r) after->r->l = t;
		after->r = t;

		if (inserted) *inserted = t;
		return JBAS_OK;
	}
}

jbas_error jbas_token_list_insert_before_from_pool(
		jbas_token *before,
		jbas_token_pool *pool,
		jbas_token *token,
		jbas_token **inserted)
{
	// Insert at the beginning?
	if (!before->l)
	{
		jbas_token *t;
		jbas_error err = jbas_token_pool_get(pool, &t);
		if (err) return err;
		
		*t = *token;

		t->l = NULL;
		t->r = before;
		before->l = t;

		*inserted = t;
		return JBAS_OK;
	}
	else
	{
		return jbas_token_list_insert_from_pool(before->l, pool, token, inserted);
	}
}


jbas_error jbas_token_list_push_front_from_pool(
	jbas_token *list,
	jbas_token_pool *pool,
	jbas_token *token,
	jbas_token **inserted)
{
	return jbas_token_list_insert_before_from_pool(jbas_token_list_begin(list), pool, token, inserted);
}

jbas_error jbas_token_list_push_back_from_pool(
	jbas_token *list,
	jbas_token_pool *pool,
	jbas_token *token,
	jbas_token **inserted)
{
	return jbas_token_list_insert_from_pool(jbas_token_list_end(list), pool, token, inserted);
}

/**
	Removes a node from the linked list and moves it to the pool

	The handle is moved either to the left or to the right.
	If removed element is the last one in the list, the handle is set to NULL.
*/
jbas_error jbas_token_list_return_to_pool(
	jbas_token **list_handle,
	jbas_token_pool *pool)
{
	jbas_token *t = *list_handle;
	if (t->l) t->l->r = t->r;
	if (t->r) t->r->l = t->l;

	// Move the handle to a valid element
	if (t->l) *list_handle = t->l;
	else if (t->r) *list_handle = t->r;
	else *list_handle = NULL;

	return jbas_token_pool_return(pool, t);
}

jbas_error jbas_eval_unary_operator(jbas_env *env, jbas_token *t, jbas_operator_type unary_type)
{
	if (!unary_type) unary_type = t->u.operator_token.op->type;
	if (unary_type == JBAS_OP_UNARY_PREFIX)
	{
		if (!jbas_has_left_operand(t) && jbas_has_right_operand(t))
		{
			// Evaluate parenthesis
			if (t->r->type == JBAS_TOKEN_LPAREN) jbas_remove_parenthesis(env, t->r);

			// Call the operator handler and have it replaced with operation result
			t->u.operator_token.op->handler(env, NULL, t->r, t);

			// Remove operand
			jbas_error err;
			err = jbas_remove_right_operand(env, t);
			if (err) return err;
		}
		else
			return JBAS_OPERAND_MISSING;
	}
	else if (unary_type == JBAS_OP_UNARY_POSTFIX)
	{
		if (jbas_has_left_operand(t) && !jbas_has_right_operand(t))
		{
			// Evaluate parenthesis
			if (t->l->type == JBAS_TOKEN_RPAREN) jbas_remove_parenthesis(env, t->l);

			// Call the operator handler and have it replaced with operation result
			t->u.operator_token.op->handler(env, t->l, NULL, t);

			// Remove operand
			jbas_error err;
			err = jbas_remove_left_operand(env, t);
			if (err) return err;
		}
		else
			return JBAS_OPERAND_MISSING;
	}

	return JBAS_OK;
}

jbas_error jbas_eval_binary_operator(jbas_env *env, jbas_token *t)
{
	if (jbas_has_left_operand(t) && jbas_has_right_operand(t))
	{
		// Evaluate parenthesis
		if (t->l->type == JBAS_TOKEN_RPAREN) jbas_remove_parenthesis(env, t->l);
		if (t->r->type == JBAS_TOKEN_LPAREN) jbas_remove_parenthesis(env, t->r);

		// Call the operator handler and have it replaced with operation result
		t->u.operator_token.op->handler(env, t->l, t->r, t);

		// Remove operands
		jbas_error err;
		err = jbas_remove_left_operand(env, t);
		if (err) return err;
		err = jbas_remove_right_operand(env, t);
		if (err) return err;
	}
	else
		return JBAS_OPERAND_MISSING;
	return JBAS_OK;
}


jbas_error jbas_eval(jbas_env *env, jbas_token *const begin, jbas_token *const end)
{
	// Leave commas untouched
	jbas_token *com = begin;
	while (com != end && com->type != JBAS_TOKEN_COMMA)
		com = com->r;

	// Evaluate two separate parts if commas are present
	if (com != end)
	{
		jbas_error err;
		err = jbas_eval(env, begin, com);
		if (err) return err;
		return jbas_eval(env, com->r, end);
	}

	// Evaluate all operators - starting with high-level ones
	for (int level = JBAS_MAX_OPERATOR_LEVEL; level >= 0; level--)
	{
		jbas_token *t;
		int paren;

		// Unary operators forward pass (posfix)
		paren = 0;
		for (t = begin; t && t != end; t = t->r)
		{
			paren += t->type == JBAS_TOKEN_LPAREN;
			paren -= t->type == JBAS_TOKEN_RPAREN;

			if (!paren && t->type == JBAS_TOKEN_OPERATOR)
			{
				const jbas_operator *op = t->u.operator_token.op;
				bool has_left = jbas_has_left_operand(t);
				bool has_right = jbas_has_right_operand(t);

				// Accept unary operators and binary operators that have fallback operation set as prefix
				// and have only right argument
				if (has_left && !has_right 
					&& ((op->type == JBAS_OP_UNARY_POSTFIX && level == op->level )
					|| (op->fallback == JBAS_OP_UNARY_POSTFIX && level == op->fallback_level
						&& (op->type == JBAS_OP_BINARY_LR || op->type == JBAS_OP_BINARY_RL))))
				{
					jbas_error err = jbas_eval_unary_operator(env, t, JBAS_OP_UNARY_POSTFIX);
					if (err) return err;
				}
			}
		}

		// Unary operators backward pass (prefix)
		paren = 0;
		for (t = jbas_token_list_end(begin); t && t != begin->l; t = t->l)
		{
			paren += t->type == JBAS_TOKEN_RPAREN;
			paren -= t->type == JBAS_TOKEN_LPAREN;

			if (!paren && t->type == JBAS_TOKEN_OPERATOR)
			{
				const jbas_operator *op = t->u.operator_token.op;
				bool has_left = jbas_has_left_operand(t);
				bool has_right = jbas_has_right_operand(t);

				// Accept unary operators and binary operators that have fallback operation set as prefix
				// and have only right argument
				if (!has_left && has_right 
					&& ((op->type == JBAS_OP_UNARY_PREFIX && level == op->level )
					|| (op->fallback == JBAS_OP_UNARY_PREFIX && level == op->fallback_level
						&& (op->type == JBAS_OP_BINARY_LR || op->type == JBAS_OP_BINARY_RL))))
				{
					jbas_error err = jbas_eval_unary_operator(env, t, JBAS_OP_UNARY_PREFIX);
					if (err) return err;
				}
			}
		}

		// Binary operators forward pass
		paren = 0;
		for (t = begin; t && t != end; t = t->r)
		{
			paren += t->type == JBAS_TOKEN_LPAREN;
			paren -= t->type == JBAS_TOKEN_RPAREN;

			if (!paren && t->type == JBAS_TOKEN_OPERATOR
				&& t->u.operator_token.op->type == JBAS_OP_BINARY_LR
				&& t->u.operator_token.op->level == level)
			{
				jbas_error err = jbas_eval_binary_operator(env, t);
				if (err) return err;
			}
		}

		// Binary operators backward pass
		paren = 0;
		for (t = jbas_token_list_end(begin); t && t != begin->l; t = t->l)
		{
			paren += t->type == JBAS_TOKEN_RPAREN;
			paren -= t->type == JBAS_TOKEN_LPAREN;

			if (!paren && t->type == JBAS_TOKEN_OPERATOR
				&& t->u.operator_token.op->type == JBAS_OP_BINARY_RL
				&& t->u.operator_token.op->level == level)
			{
				jbas_error err = jbas_eval_binary_operator(env, t);
				if (err) return err;
			}
		}

		fprintf(stderr, "eval %d: ", level);
		for (jbas_token *t = jbas_token_list_begin(begin); t; t = t->r)
			jbas_debug_dump_token(stderr, t);
		fprintf(stderr, "\n");
	}

	return JBAS_OK;
}


/**
	Executes one instruction - starting at the provided token and ending at matching delimiter
	(; for normal instructions, ENDIF for IFs and so on...)
*/
jbas_error jbas_run_instruction(jbas_env *env, jbas_token **next, jbas_token *const token)
{
	// Skip delimiters
	jbas_token *begin_token = token;
	while (begin_token && begin_token->type == JBAS_TOKEN_DELIMITER)
		begin_token = begin_token->r;

	// Create copy of the entire expression
	//! \todo keyword delimiters
	jbas_token *t, *expr = NULL;
	for (t = begin_token; t && t->type != JBAS_TOKEN_DELIMITER; t = t->r)
	{
		jbas_token *new_token = NULL;
		jbas_error err = jbas_token_list_push_back_from_pool(expr, &env->token_pool, t, &new_token);
		if (err) return err;
		expr = new_token;
	}
	*next = t;

	// DEBUG
	fprintf(stderr, "Will evaluate: ");
	for (jbas_token *t = jbas_token_list_begin(expr); t; t = t->r)
		jbas_debug_dump_token(stderr, t);
	fprintf(stderr, "\n");



	jbas_error eval_err = jbas_eval(env, jbas_token_list_begin(expr), NULL);
	fprintf(stderr, "eval err: %d\n", eval_err);


	// DEBUG
	fprintf(stderr, "After eval: ");
	for (jbas_token *t = jbas_token_list_begin(expr); t; t = t->r)
		jbas_debug_dump_token(stderr, t);
	fprintf(stderr, "\n");

	// Delete what's left
	expr = jbas_token_list_begin(expr);
	while (expr)
		jbas_token_list_return_to_pool(&expr, &env->token_pool);

	return JBAS_OK;
}

jbas_error jbas_run_tokens(jbas_env *env)
{
	jbas_token *token = jbas_token_list_begin(env->tokens);
	while (token)
	{
		jbas_error err = jbas_run_instruction(env, &token, token);
		if (err) return err;
	}

	return JBAS_OK;
}

/**
	Performs line tokenization, parsing and executes it in the provided environment
*/
jbas_error jbas_tokenize_string(jbas_env *env, const char *str)
{
	while (1)
	{
		// Get a token from the line
		jbas_token t;
		jbas_error err = jbas_get_token(env, str, &str, &t);

		if (!err) // Insert a valid token into the list
		{
			jbas_token *new_token;
			jbas_error err = jbas_token_list_push_back_from_pool(env->tokens, &env->token_pool, &t, &new_token);
			env->tokens = new_token;
			if (err) return err;
		}
		else if (err == JBAS_EMPTY_TOKEN)
			break;
		else
			return err;
	}

	return JBAS_OK;
}

jbas_error jbas_env_init(jbas_env *env, int token_count, int text_count, int symbol_count, int resource_count)
{
	env->tokens = NULL;
	jbas_error err;

	err = jbas_token_pool_init(&env->token_pool, token_count);
	if (err) return err;

	err = jbas_text_manager_init(&env->text_manager, text_count);
	if (err) return err;

	err = jbas_symbol_manager_init(&env->symbol_manager, symbol_count);
	if (err) return err;

	err = jbas_resource_manager_init(&env->resource_manager, resource_count);
	if (err) return err;

	return JBAS_OK;
}

void jbas_env_destroy(jbas_env *env)
{
	jbas_token_pool_destroy(&env->token_pool);
	jbas_text_manager_destroy(&env->text_manager);
	jbas_symbol_manager_destroy(&env->symbol_manager);
	jbas_resource_manager_destroy(&env->resource_manager);
}

#endif

#ifdef __cplusplus
}
#endif

#endif