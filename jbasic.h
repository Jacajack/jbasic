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
	JBAS_ALLOC,
	JBAS_TEXT_MANAGER_OVERFLOW,
	JBAS_TEXT_MANAGER_MISMATCH,
	JBAS_RESOURCE_MANAGER_NOT_EMPTY,
	JBAS_TYPE_MISMATCH,
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

typedef struct
{
	jbas_text *txt;
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


jbas_error jbas_text_manager_destroy(jbas_text_manager *tm)
{
	// Destroy all the stored texts first
	for (int i = 0; i < tm->max_count; i++)
		if (tm->is_used[i])
			jbas_text_destroy(tm, &tm->text_storage[i]);

	free(tm->text_storage);
	free(tm->free_slots);
	free(tm->is_used);
	return JBAS_OK;
}













/*
	Resources are created dynamically during the program execution.
	Garbage collection is performed automatically as well
*/


typedef enum
{
	JBAS_RESOURCE_INT,
	JBAS_RESOURCE_FLOAT,
	JBAS_RESOURCE_INT_ARRAY,
	JBAS_RESOURCE_FLOAT_ARRAY,
	JBAS_RESOURCE_STRING,
} jbas_resource_type;


typedef struct
{
	jbas_resource_type type;
	int ref_count;
	size_t size;
	
	union
	{
		jbas_int i;
		jbas_float f;
		char *str;
		void *data;
	};
	
} jbas_resource;

typedef struct
{
	jbas_resource **refs;
	int ref_count;
	int max_count;
} jbas_resource_manager;

jbas_error jbas_resource_manager_init(jbas_resource_manager *rm, int max_count)
{
	rm->max_count = max_count;
	rm->refs = calloc(max_count, sizeof(jbas_resource*));
	rm->ref_count = 0;

	if (!rm->refs)
		return JBAS_ALLOC;

	return JBAS_OK;
}

jbas_error jbas_resource_manager_destroy(jbas_resource_manager *rm)
{
	if (rm->ref_count)
		return JBAS_RESOURCE_MANAGER_NOT_EMPTY;

	free(rm->refs);
	return JBAS_OK;
}

jbas_error jbas_resource_delete(jbas_resource *res)
{
	switch (res->type)
	{
		case JBAS_RESOURCE_INT:
		case JBAS_RESOURCE_FLOAT:
		default:
			break;
	}

	free(res);
	return JBAS_OK;
}

jbas_error jbas_resource_manager_garbage_collect(jbas_resource_manager *rm)
{
	for (int i = 0; i < rm->ref_count; i++)
	{
		if (rm->refs[i]->ref_count == 0)
			jbas_resource_delete(rm->refs[i]);
		
		rm->refs[i] = rm->refs[--rm->ref_count];
	}

	return JBAS_OK;
}

jbas_error jbas_resource_remove_ref(jbas_resource *res)
{
	if (res->ref_count) res->ref_count--;
	return JBAS_OK;
}

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

	*res = r;
	return JBAS_OK;
}

jbas_error jbas_create_int(jbas_resource_manager *rm, jbas_resource **res, jbas_int value)
{
	jbas_resource *r;
	jbas_error err = jbas_resource_create(rm, &r);
	if (err) return err;

	r->type = JBAS_RESOURCE_INT;
	r->i = value; 

	*res = r;
	return JBAS_OK;
}

jbas_error jbas_assign_int(jbas_resource *res, jbas_int value)
{
	if (res->type == JBAS_RESOURCE_INT)
		res->i = value;
	else if (res->type == JBAS_RESOURCE_FLOAT)
		res->f = value;
	else
		return JBAS_TYPE_MISMATCH;
	return JBAS_OK;
}

jbas_error jbas_create_float(jbas_resource_manager *rm, jbas_resource **res, jbas_float value)
{
	jbas_resource *r;
	jbas_error err = jbas_resource_create(rm, &r);
	if (err) return err;

	r->type = JBAS_RESOURCE_FLOAT;
	r->f = value; 

	*res = r;
	return JBAS_OK;
}











typedef enum
{
	JBAS_VARIABLE,
	JBAS_LABEL,
	JBAS_FUNCTION,	
} jbas_symbol_type;

typedef struct
{
	jbas_text name;
	jbas_symbol_type type;

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


jbas_error jbas_symbol_manager_init(jbas_symbol_manager *sm, int symbol_count)
{
	sm->max_count = symbol_count;
	sm->free_slot_count = symbol_count;

	// Allocate some memory
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

jbas_error jbas_symbol_manager_destroy(jbas_symbol_manager *sm)
{
	free(sm->symbol_storage);
	free(sm->is_used);
	free(sm->free_slots);
	return JBAS_OK;
}

jbas_error jbas_create_symbol(jbas_symbol_manager *sm, jbas_symbol *sym, const char *name)
{

}

jbas_error jbas_delete_symbol(jbas_symbol_manager *sm, const char *name)
{

}

jbas_error jbas_symbol_lookup(jbas_symbol_manager *sm, const char *name, jbas_symbol *sym)
{

}




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
void jbas_debug_dump_token(FILE *f, const jbas_token *token)
{	
	switch (token->type)
	{
		case JBAS_TOKEN_KEYWORD:
			fprintf(f, "KEYWORD");
			break;

		case JBAS_TOKEN_STRING:
			fprintf(f, "'%s'", token->u.string_token.txt->str);
			break;

		case JBAS_TOKEN_NUMBER:
			fprintf(f, "NUMBER");
			break;

		case JBAS_TOKEN_DELIMITER:
			fprintf(f, "\n");
			break;

		default:
			fprintf(f, "(%d?)", token->type );
			break;
	}

	fprintf(f, " ");
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

		jbas_error err = jbas_text_create(
			&env->text_manager,
			s + 1,
			str_end,
			&token->u.string_token.txt);

		return err;
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
		jbas_error err = jbas_get_token(env, str, &str, &t);

		if (!err) // Insert a valid token into the list
		{
			jbas_token *new_token;
			jbas_error err = jbas_token_list_push_back_from_pool(env->tokens, &env->token_pool, &t, &new_token);
			env->tokens = new_token;
			if (err) return err;

			#ifdef JBASIC_DEBUG
				jbas_debug_dump_token(stderr, &t);
			#endif
		}
		else if (err == JBAS_EMPTY_TOKEN)
			break;
		else
			return err;
	}

	return JBAS_OK;
}

jbas_error jbas_env_init(jbas_env *env, int token_count, int text_count)
{
	env->tokens = NULL;
	jbas_error err;

	err = jbas_token_pool_init(&env->token_pool, token_count);
	if (err) return err;

	err = jbas_text_manager_init(&env->text_manager, text_count);
	if (err) return err;

	return JBAS_OK;
}

jbas_error jbas_env_destroy(jbas_env *env)
{
	jbas_text_manager_destroy(&env->text_manager);
	jbas_token_pool_destroy(&env->token_pool);
	return JBAS_OK;
}

#endif

#ifdef __cplusplus
}
#endif

#endif