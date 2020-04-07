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



typedef enum
{
	JBAS_OK = 0,
	JBAS_SYNTAX_UNMATCHED_QUOTE,
	JBAS_PRINT_BAD_ARGUMENT,
	JBAS_EMPTY_TOKEN,
	JBAS_TOKEN_POOL_EMPTY,
	JBAS_TOKEN_POOL_OVERFLOW,
	JBAS_ALLOC
} jbas_error;

typedef enum
{
 	JBAS_TOKEN_NAME,
 	JBAS_TOKEN_KEYWORD,
 	JBAS_TOKEN_OPERATOR,
 	JBAS_TOKEN_LPAREN,
 	JBAS_TOKEN_RPAREN,
 	JBAS_TOKEN_NUMBER,
	JBAS_TOKEN_STRING,
	JBAS_TOKEN_DELIMITER,
} jbas_token_type;

typedef enum
{
	JBAS_KW_PRINT
} jbas_keyword;

typedef enum
{
	JBAS_OP_NOT,
	JBAS_OP_AND,
	JBAS_OP_OR,
	JBAS_OP_EQ,
	JBAS_OP_NEQ,
	JBAS_OP_LESS,
	JBAS_OP_GREATER,
	JBAS_OP_GEQ,
	JBAS_OP_LEQ,
	JBAS_OP_ADD,
	JBAS_OP_SUB,
	JBAS_OP_MUL,
	JBAS_OP_DIV,
	JBAS_OP_MOD,
} jbas_operator;

static const struct {const char *name; jbas_keyword id;} jbas_keywords[] = 
{
	{"PRINT", JBAS_KW_PRINT}
};
#define JBAS_KEYWORD_COUNT ((sizeof jbas_keywords) / (sizeof jbas_keywords[0]))

/*
	Token structures
*/

typedef int jbas_int;
typedef float jbas_float;

typedef struct
{
	const char *begin;
	const char *end;
} jbas_string_token;

typedef struct
{
	int is_integer;
	union
	{
		jbas_int i;
		jbas_float f;
	} value;
} jbas_number_token;

typedef struct
{
	jbas_operator id;
} jbas_operator_token;

typedef struct
{
	jbas_keyword id;
} jbas_keyword_token;

typedef struct
{
	const char *begin;
	const char *end;
} jbas_name_token;

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
		jbas_name_token name_token;
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

/**
	Environment for BASIC program execution
*/
typedef struct
{
	jbas_token_pool token_pool; //!< Common token pool
	jbas_token *tokens; //!< Tokenized program
} jbas_env;



typedef struct
{
	int length;
	const char *str;
} jbas_text;





#ifdef JBASIC_IMPLEMENTATION

/**
	Returns true or false depending on whether the character
	can be a part of program variable or sub
*/
int jbas_is_name_char(char c)
{
	return isalpha(c) || (c == '_');
}

/**
	Dumps token to stderr
*/
void jbas_debug_dump_token(const jbas_token *token)
{	
	fprintf(stderr, "TOKEN: `%.*s` is a ", (int)(token->end - token->begin), token->begin);

	switch (token->type)
	{
		case JBAS_TOKEN_KEYWORD:
			fprintf(stderr, "KEYWORD");
			break;

		case JBAS_TOKEN_STRING:
			fprintf(stderr, "STRING (%p, %p)",
				token->u.string_token.begin,
				token->u.string_token.end);
			break;

		case JBAS_TOKEN_NUMBER:
			fprintf(stderr, "NUMBER");
			break;

		default:
			fprintf(stderr, "UNKNOWN (%d)", token->type );
			break;
	}

	fprintf(stderr, "\n");
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
				printf("%.*s", (int)(t->end - t->begin), t->begin);
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
jbas_error jbas_get_token(const char *const str, const char **next, jbas_token *token)
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
		token->u.number_token.is_integer = is_int;
		if (is_int)
			sscanf(s, "%d", &token->u.number_token.value.i);
		else
			sscanf(s, "%f", &token->u.number_token.value.f);

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

		token->u.string_token.begin = token->begin + 1;
		token->u.string_token.end = str_end;

		return JBAS_OK;
	}

	// If it's one of '+-*/=<>' then it's an operator
	if (strchr("+-*/=<>%!", *s))
	{
		token->end = *next = s + 1;
		token->type = JBAS_TOKEN_OPERATOR;

		switch (*s)
		{
			case '!':
				token->u.operator_token.id = JBAS_OP_NOT;
				if (s[1] == '=')
					token->u.operator_token.id = JBAS_OP_NEQ;
				break;

			case '+':
				token->u.operator_token.id = JBAS_OP_ADD;
				break;

			case '-':
				token->u.operator_token.id = JBAS_OP_SUB;
				break;

			case '*':
				token->u.operator_token.id = JBAS_OP_MUL;
				break;

			case '/':
				token->u.operator_token.id = JBAS_OP_DIV;
				break;

			case '%':
				token->u.operator_token.id = JBAS_OP_MOD;
				break;

			case '=':
				token->u.operator_token.id = JBAS_OP_EQ;
				break;

			case '<':
				token->u.operator_token.id = JBAS_OP_LESS;
				if (s[1] == '=')
					token->u.operator_token.id = JBAS_OP_LEQ;
				break;

			case '>':
				token->u.operator_token.id = JBAS_OP_GREATER;
				if (s[1] == '=')
					token->u.operator_token.id = JBAS_OP_GEQ;
				break;
		}

		return JBAS_OK;
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

	// A variable/keyword
	const char *name_end = s;
	while (jbas_is_name_char(*name_end)) name_end++;
	token->type = JBAS_TOKEN_NAME;
	token->u.name_token.begin = s;
	token->end = *next = token->u.name_token.end = name_end;

	for (int i = 0; i < JBAS_KEYWORD_COUNT; i++)
	{
		if (name_end - s == strlen(jbas_keywords[i].name)
			&& !strncasecmp(jbas_keywords[i].name, s, name_end - s))
		{
			token->type = JBAS_TOKEN_KEYWORD;
			token->u.keyword_token.id = jbas_keywords[i].id;
			return JBAS_OK;
		}
	}

	return JBAS_OK;
}



// -------------------------------------- TOKEN POOL

jbas_error jbas_token_pool_get(
	jbas_token_pool *pool,
	jbas_token **t)
{
	if (!pool->unused_count) return JBAS_TOKEN_POOL_EMPTY;
	*t = pool->unused_stack[--pool->unused_count];
	return JBAS_OK;
}

jbas_error jbas_token_pool_return(
	jbas_token_pool *pool,
	jbas_token *t)
{
	if (pool->unused_count >= pool->pool_size) return JBAS_TOKEN_POOL_OVERFLOW;
	pool->unused_stack[pool->unused_count++] = t;
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
		if(inserted) *inserted = t;
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



/**
	Performs line tokenization, parsing and executes it in the provided environment
*/
jbas_error jbas_tokenize_string(jbas_env *env, const char *str)
{
	while (1)
	{
		// Get a token from the line
		jbas_token t;
		jbas_error err = jbas_get_token(str, &str, &t);

		if (!err) // Insert a valid token into the list
		{
			jbas_token *new_token;
			jbas_error err = jbas_token_list_push_back_from_pool(env->tokens, &env->token_pool, &t, &new_token);
			env->tokens = new_token;
			if (err) return err;

			#ifdef JBASIC_DEBUG
				jbas_debug_dump_token(&t);
			#endif
		}
		else if (err == JBAS_EMPTY_TOKEN)
			break;
		else
			return err;
	}

	return JBAS_OK;
}

jbas_error jbas_env_init(jbas_env *env, int token_count)
{
	env->tokens = NULL;
	return jbas_token_pool_init(&env->token_pool, token_count);
}

jbas_error jbas_env_destroy(jbas_env *env)
{
	return jbas_token_pool_destroy(&env->token_pool);
}

#endif

#ifdef __cplusplus
}
#endif

#endif