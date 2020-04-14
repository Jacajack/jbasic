#ifndef JBASIC_TOKEN_H
#define JBASIC_TOKEN_H

#include <jbasic/defs.h>
#include <jbasic/text.h>

typedef enum
{
 	JBAS_TOKEN_SYMBOL,
 	JBAS_TOKEN_KEYWORD,
 	JBAS_TOKEN_OPERATOR,
 	JBAS_TOKEN_LPAREN,
 	JBAS_TOKEN_RPAREN,
 	JBAS_TOKEN_NUMBER,
	JBAS_TOKEN_STRING,
	JBAS_TOKEN_TUPLE,
	JBAS_TOKEN_DELIMITER,
} jbas_token_type;

typedef struct jbas_symbol jbas_symbol;
typedef struct jbas_operator jbas_operator;
typedef struct jbas_keyword jbas_keyword;
typedef struct jbas_token jbas_token;

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
	const jbas_keyword *kw;
} jbas_keyword_token;

typedef struct
{
	jbas_symbol *sym;
} jbas_symbol_token;

typedef struct jbas_tuple_token
{
	jbas_token *tokens;
} jbas_tuple_token;

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

typedef struct jbas_paren_token
{
	jbas_token *match;
} jbas_paren_token;

/**
	Polymorphic token
*/
typedef struct jbas_token
{
	jbas_token_type type;
	union
	{
		jbas_keyword_token keyword_token;
		jbas_operator_token operator_token;
		jbas_number_token number_token;
		jbas_string_token string_token;
		jbas_symbol_token symbol_token;
		jbas_tuple_token tuple_token;
		jbas_paren_token paren_token;
	};

	// For bidirectional linking
	struct jbas_token *l, *r;

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

void jbas_token_copy(jbas_token *dest, jbas_token *src);
void jbas_token_swap(jbas_token *dest, jbas_token *src);
jbas_error jbas_token_pool_get(jbas_token_pool *pool, jbas_token **t);
jbas_error jbas_token_pool_return(jbas_token_pool *pool, jbas_token *t);
jbas_error jbas_token_pool_init(jbas_token_pool *pool, int size);
jbas_error jbas_token_pool_destroy(jbas_token_pool *pool);
jbas_token *jbas_token_list_begin(jbas_token *t);
jbas_token *jbas_token_list_end(jbas_token *t);
jbas_error jbas_token_list_insert_from_pool(
	jbas_token *after,
	jbas_token_pool *pool,
	jbas_token *token,
	jbas_token **inserted);
jbas_error jbas_token_list_insert_before_from_pool(
		jbas_token *before,
		jbas_token_pool *pool,
		jbas_token *token,
		jbas_token **inserted);
jbas_error jbas_token_list_push_front_from_pool(
	jbas_token *list,
	jbas_token_pool *pool,
	jbas_token *token,
	jbas_token **inserted);
jbas_error jbas_token_list_push_back_from_pool(
	jbas_token *list,
	jbas_token_pool *pool,
	jbas_token *token,
	jbas_token **inserted);
jbas_error jbas_token_list_return_handle_to_pool(jbas_token **list_handle, jbas_token_pool *pool);
jbas_error jbas_token_list_return_to_pool(jbas_token *t, jbas_token_pool *pool);
jbas_error jbas_token_list_destroy(jbas_token *list, jbas_token_pool *pool);

#endif