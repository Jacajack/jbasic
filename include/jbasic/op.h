#ifndef JBASIC_OP_H
#define JBASIC_OP_H

#include <jbasic/defs.h>
#include <jbasic/token.h>

typedef enum jbas_operator_type
{
	JBAS_OP_UNDEFINED = 0,
	JBAS_OP_BINARY_RL,
	JBAS_OP_BINARY_LR,
	JBAS_OP_UNARY_PREFIX,
	JBAS_OP_UNARY_SUFFIX,
	JBAS_OP_UNARY_POSTFIX = JBAS_OP_UNARY_SUFFIX,
} jbas_operator_type;

typedef struct jbas_operator
{
	const char *str;
	int level;
	jbas_operator_type type;
	jbas_error (*handler)(jbas_env *env, jbas_token *a, jbas_token *b, jbas_token *res);
	const jbas_operator *fallback;
	bool eval_args;
} jbas_operator;

typedef struct jbas_operator_sort_bucket
{
	jbas_token *token;
	int pos;
} jbas_operator_sort_bucket;

extern const jbas_operator jbas_operators[];

int jbas_is_operator_char(char c);
const jbas_operator *jbas_get_operator_by_str(const char *b, const char *e);

bool jbas_is_binary_operator(jbas_token *t);
bool jbas_is_unary_operator(jbas_token *t);

bool jbas_is_pure_operand(const jbas_token *t);
bool jbas_is_valid_operand(jbas_token *t);
bool jbas_has_left_operand(jbas_token *t);
bool jbas_has_right_operand(jbas_token *t);
jbas_error jbas_remove_operand(jbas_env *env, jbas_token *t);

jbas_error jbas_eval_unary_operator(jbas_env *env, jbas_token *t);
jbas_error jbas_eval_binary_operator(jbas_env *env, jbas_token *t);
jbas_error jbas_eval_call_operator(jbas_env *env, jbas_token *fun, jbas_token *args);

int jbas_operator_token_compare(const void *av, const void *bv);
void jbas_try_fallback_operator(jbas_token *t, jbas_operator_type fallback_type);

#define JBAS_MAX_OPERATOR_LEVEL 6
#define JBAS_OPERATOR_COUNT 19

#endif