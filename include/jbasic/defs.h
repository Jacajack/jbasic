#ifndef JBASIC_DEFS
#define JBASIC_DEFS

typedef enum jbas_error
{
	JBAS_OK = 0,
	JBAS_SYNTAX_UNMATCHED_QUOTE,
	JBAS_SYNTAX_ERROR,
	JBAS_MISSING_END,
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
	JBAS_BAD_CALL,
	JBAS_BAD_COMPARE,
	JBAS_EVAL_OVERFLOW, // Operator stack overflow
	JBAS_EVAL_NON_SCALAR, // Attempt to evaluate non-scalar token
} jbas_error;


typedef int jbas_int;
typedef float jbas_float;


// Env forward declaration
typedef struct jbas_env jbas_env;


#ifdef JBAS_ERROR_REASONS
	#define JBAS_ERROR_REASON(env, s) ((env)->error_reason = (__FILE__ ": " s)); 
#else
	#define JBAS_ERROR_REASON(env, s) (void);
#endif

#endif

/*
	Hej, przepraszam, że tak późno, ale chciałem Ci życzyć wszystkiego najlepszego z okazji urodzin.
*/