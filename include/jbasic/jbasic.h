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

#include <jbasic/defs.h>
#include <jbasic/token.h>
#include <jbasic/op.h>
#include <jbasic/text.h>
#include <jbasic/resource.h>
#include <jbasic/symbol.h>


/**
	Environment for BASIC program execution
*/
typedef struct jbas_env
{
	jbas_token_pool token_pool; //!< Common token pool
	jbas_text_manager text_manager;
	jbas_resource_manager resource_manager;
	jbas_symbol_manager symbol_manager;
	jbas_token *tokens; //!< Tokenized program

	const char *error_reason; //!< Reason for returning an error
} jbas_env;


bool jbas_is_paren(const jbas_token *t);
jbas_error jbas_remove_parenthesis(jbas_env *env, jbas_token *t);
jbas_error jbas_get_matching_paren(jbas_token *t, jbas_token **match);
jbas_error jbas_eval_paren(jbas_env *env, jbas_token *t);
jbas_number_type jbas_number_type_promotion(jbas_number_type a, jbas_number_type b);
void jbas_number_cast(jbas_number_token *n, jbas_number_type t);
bool jbas_is_scalar(jbas_token *t);
jbas_error jbas_eval_scalar_symbol(jbas_env *env, jbas_token *t);
jbas_error jbas_token_to_number(jbas_env *env, jbas_token *t);
jbas_error jbas_token_to_number_type(jbas_env *env, jbas_token *t, jbas_number_type type);
bool jbas_has_left_operand(jbas_token *t);
jbas_error jbas_remove_left_operand(jbas_env *env, jbas_token *t);
bool jbas_has_right_operand(jbas_token *t);
jbas_error jbas_remove_right_operand(jbas_env *env, jbas_token *t);
int jbas_namecmp(const char *s1, const char *end1, const char *s2, const char *end2);
int jbas_is_name_char(char c);
jbas_error jbas_print(const jbas_token *token, int count);
jbas_error jbas_get_token(jbas_env *env, const char *const str, const char **next, jbas_token *token);
jbas_error jbas_eval_unary_operator(jbas_env *env, jbas_token *t, jbas_operator_type unary_type);
jbas_error jbas_eval_binary_operator(jbas_env *env, jbas_token *t);
jbas_error jbas_eval(jbas_env *env, jbas_token *const begin, jbas_token *const end, jbas_token **result);
jbas_error jbas_run_instruction(jbas_env *env, jbas_token **next, jbas_token *const token);
jbas_error jbas_run_tokens(jbas_env *env);
jbas_error jbas_tokenize_string(jbas_env *env, const char *str);
jbas_error jbas_env_init(jbas_env *env, int token_count, int text_count, int symbol_count, int resource_count);
void jbas_env_destroy(jbas_env *env);

#define JBAS_MAX_EVAL_OPERATORS 64

#ifdef __cplusplus
}
#endif

#endif