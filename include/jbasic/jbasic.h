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
#include <jbasic/kw.h>


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


bool jbas_is_name_char(char c);
int jbas_namecmp(const char *s1, const char *end1, const char *s2, const char *end2);

jbas_error jbas_eval(jbas_env *env, jbas_token *const begin, jbas_token **result);
jbas_error jbas_eval_instruction(jbas_env *env, jbas_token *begin, jbas_token **next, jbas_token **result);
jbas_error jbas_run_step(jbas_env *env, jbas_token *begin, jbas_token **next);
jbas_error jbas_run_block(jbas_env *env, jbas_token *begin, jbas_token *end, jbas_token **next);
jbas_error jbas_run(jbas_env *env);
jbas_error jbas_get_token(jbas_env *env, const char *const str, const char **next, jbas_token ***lists, int *level);
jbas_error jbas_tokenize_string(jbas_env *env, const char *str);
jbas_error jbas_env_init(jbas_env *env, int token_count, int text_count, int symbol_count, int resource_count);
void jbas_env_destroy(jbas_env *env);

#define JBAS_MAX_EVAL_OPERATORS 64
#define JBAS_TOKENIZE_PAREN_LEVELS 64

#ifdef __cplusplus
}
#endif

#endif