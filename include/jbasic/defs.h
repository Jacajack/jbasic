#ifndef JBASIC_DEFS
#define JBASIC_DEFS

typedef enum jbas_error
{
	JBAS_OK = 0,
	JBAS_SYNTAX_UNMATCHED_QUOTE,
	JBAS_PRINT_BAD_ARGUMENT,
	JBAS_EMPTY_TOKEN,
	JBAS_TOKEN_POOL_EMPTY,
	JBAS_TOKEN_POOL_OVERFLOW,
	JBAS_ALLOC,                      // Memory allocation problem
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


typedef enum jbas_keyword
{
	JBAS_KW_PRINT,
	JBAS_KW_IF,
	JBAS_KW_THEN,
	JBAS_KW_ENDIF
} jbas_keyword;


// Env forward declaration
typedef struct jbas_env jbas_env;

static const struct {const char *name; jbas_keyword id;} jbas_keywords[] = 
{
	{"PRINT", JBAS_KW_PRINT},
	{"IF", JBAS_KW_IF},
	{"ENDIF", JBAS_KW_ENDIF},
	{"THEN", JBAS_KW_THEN},
};
#define JBAS_KEYWORD_COUNT ((sizeof(jbas_keywords)) / (sizeof(jbas_keywords[0])))




typedef int jbas_int;
typedef float jbas_float;



#endif