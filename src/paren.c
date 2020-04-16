#include <jbasic/paren.h>
#include <jbasic/jbasic.h>

bool jbas_is_paren(const jbas_token *t)
{
	return t && (t->type == JBAS_TOKEN_PAREN); 
}

/**
	Removes entire parentheses (with contents)
	\warning The provided token iterator is obviously invalidated
*/
jbas_error jbas_remove_entire_paren(jbas_env *env, jbas_token *t)
{
	return jbas_token_list_return_to_pool(t, &env->token_pool);
}

/**
	Evaluates everything inside the parenthesis and replaces it with the resulting token
	\note Provided token pointer is not invalidated and will point to the parenthesis contents
*/
jbas_error jbas_eval_paren(jbas_env *env, jbas_token *t)
{
	jbas_token *res;
	jbas_error err;

	if (!t || t->type != JBAS_TOKEN_PAREN) return JBAS_OK;

	// Evaluate contents
	err = jbas_eval(env, jbas_token_list_begin(t->paren_token.tokens), &res);
	if (err) return err;

	// Replace the parentheses token with the result or a number 0 if there's no result 
	// Ingenious workaround <3
	if (res)
	{
		return jbas_token_move(t, res, &env->token_pool);
	}
	{
		jbas_token zero = {.type = JBAS_TOKEN_NUMBER, .number_token = {.type = JBAS_NUM_INT, .i = 0}};
		return jbas_token_copy(t, &zero, &env->token_pool);
	}
}
