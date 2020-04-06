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

#define JBAS_DEBUG

typedef enum
{
	JBAS_OK = 0,
	JBAS_SYNTAX_UNMATCHED_QUOTE,
	JBAS_PRINT_BAD_ARGUMENT,
	JBAS_EMPTY_TOKEN,
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
typedef struct
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

} jbas_token;

/**
	Element of a bidirectional token list
*/
typedef struct jbas_token_list_node_struct
{
	struct jbas_token_list_node_struct *l, *r;
	jbas_token t;
} jbas_token_list_node;

/**
	Pool with unused list nodes
*/
typedef struct 
{
	jbas_token_list_node *nodes;
	jbas_token_list_node **unused_stack;
	int pool_size;
	int unused_count;
} jbas_token_list_node_pool;

/**
	Environment for BASIC program execution
*/
typedef struct
{
	int token_stack_size;
	jbas_token_list_node_pool token_pool;
} jbas_env;

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
	while (*s && isspace(*s)) s++;
	if (!*s)
	{
		*next = NULL;
		return JBAS_EMPTY_TOKEN;
	}

	token->begin = s;

	// If it's ';', it's a delimiter
	if (*s == ';')
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

/**
	Insert an element into the bidirectional token list.
	Returns a pointer to the newly inserted list node
*/
jbas_token_list_node *jbas_token_list_insert(jbas_token_list_node *after, jbas_token_list_node *node)
{
	if (after == NULL)
	{
		node->l = NULL;
		node->r = NULL;
	}
	else
	{
		node->r = after->r;
		node->l = after;
		if (node->l) node->l->r = node;
		if (node->r) node->r->l = node;
	}

	return node;
}

/**
	Returns a pointer to the first element of the list
*/
jbas_token_list_node *jbas_token_list_find_beginning(jbas_token_list_node *n)
{
	while (n->l)
		n = n->l;
	return n;
}

/**
	Removes a node from a token list
*/
jbas_token_list_node *jbas_token_list_remove(jbas_token_list_node *n)
{
	if (n->l) n->l->r = n->r;
	if (n->r) n->r->l = n->l;
	return n;
}

jbas_error jbas_token_list_insert_from_pool(
	jbas_token_list_node **list_handle,
	jbas_token_list_node_pool *pool,
	jbas_token *t)
{
	if (!pool->count) return JBAS_NODE_POOL_EMPTY;
	jbas_token_list_node *
}

jbas_error jbas_token_list_move_to_pool(
	jbas_token_list_node **list_handle,
	jbas_token_list_node_pool *pool)
{

}


/**
	Evaluates token list
*/
jbas_error jbas_eval_token_list(jbas_token_list_node *list)
{
	// Run recursively for parentheses

}

/**
	Performs line tokenization, parsing and executes it in the provided environment
*/
jbas_error jbas_run_line(jbas_env *env, const char *line)
{
	jbas_token_list_node node_pool[env->token_stack_size];
	int free_node_count = env->token_stack_size;

	jbas_token_list_node *token_list = NULL;
	
	// Collect all tokens on the stack
	while (free_node_count)
	{
		// Take pointer to a node from the pool
		jbas_token_list_node *new_node = &node_pool[free_node_count - 1];
		
		// Get a token from the line
		jbas_error err = jbas_get_token(line, &line, &new_node->t);

		if (!err) // Insert a valid token into the list
		{
			free_node_count--;
			token_list = jbas_token_list_insert(token_list, new_node);
		}
		else if (err == JBAS_EMPTY_TOKEN)
			break;
		else
			return err;
	}

	// "Rewind" the list
	token_list = jbas_token_list_find_beginning(token_list);

	// Dump all tokens for debug purposes
	#ifdef JBAS_DEBUG
	// for ( int i = 0; i < token_count; i++ )
	for (jbas_token_list_node *n = token_list; n; n = n->r)
		jbas_debug_dump_token(&n->t);
	#endif

	/*

	if (!token_count) return JBAS_OK;

	// If the first token is a keyword
	if (tokens[0].type == JBAS_TOKEN_KEYWORD)
	{
		jbas_error err = JBAS_OK;

		switch (tokens[0].u.keyword_token.id)
		{
			case JBAS_KW_PRINT:
				err = jbas_print(tokens + 1, token_count - 1);
				break;
		}
	}
	
	// ASSIGNMENT
	if (tokens[0].type == JBAS_TOKEN_NAME
		&& tokens[1].type == JBAS_TOKEN_OPERATOR
		&& tokens[1].u.operator_token.id == JBAS_OP_EQ)
	{
		// TODO
		printf("Assigned to %.*s\n", (int)(tokens[0].u.name_token.end - tokens[0].u.name_token.begin), tokens[0].u.name_token.begin);
	}

	*/

	return JBAS_OK;
}

/**
	Runs entire program
*/
jbas_error jbas_run_lines(jbas_env *env, const char *const str)
{
	char *lines = strdup(str);
	char *strtok_buf;

	for (char *line = strtok_r(lines, "\n", &strtok_buf);
		line;
		line = strtok_r(NULL, "\n", &strtok_buf))
	{
		jbas_error err = jbas_run_line(env, line);
		if (err)
		{
			free(lines);
			return err;
		}
	}

	free(lines);
	return JBAS_OK;	
}

int main(void)
{
	jbas_env jb =
	{
		.token_stack_size = 12
	};

	const char *lines = 
	"ASS_STR = 'dupa'\n"
	"PRINT 'heheszky'\n"
	"PRINT `This is nice!`\n";

	jbas_run_lines(&jb, lines);

	//jbas_run_line(&jb, "PRINT 'Hello \"Wo\"rld!' 12 4.4e-4 FARFUGOL 3E+12 \"I'm listening to The Cure\" ");
}