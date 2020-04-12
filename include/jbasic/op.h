#ifndef JBASIC_OP_H
#define JBASIC_OP_H

#include <jbasic/defs.h>
#include <jbasic/token.h>

typedef enum jbas_operator_type
{
	JBAS_OP_UNDEFINED = 0,
	JBAS_OP_UNARY_PREFIX,
	JBAS_OP_UNARY_SUFFIX,
	JBAS_OP_UNARY_POSTFIX = JBAS_OP_UNARY_SUFFIX,
	JBAS_OP_BINARY_LR,
	JBAS_OP_BINARY_RL,
} jbas_operator_type;

typedef struct jbas_operator
{
	const char *str;
	int level;
	jbas_operator_type type;
	jbas_error (*handler)(jbas_env *env, jbas_token *a, jbas_token *b, jbas_token *res);
	
	jbas_operator_type fallback; // Fallback operator type
	int fallback_level;
} jbas_operator;

extern const jbas_operator jbas_operators[];

int jbas_is_operator_char(char c);


#define JBAS_MAX_OPERATOR_LEVEL 6
#define JBAS_OPERATOR_COUNT 19

#endif