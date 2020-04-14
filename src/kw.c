#include <jbasic/kw.h>
#include <jbasic/jbasic.h>
#include <jbasic/cast.h>
#include <stdarg.h>
#include <stdio.h>

/**
	Executes an if statement
*/

static jbas_error jbas_kw_if(jbas_env *env, jbas_token *begin, jbas_token **next)
{
	jbas_error err;
	jbas_token *t_end;
	err = jbas_get_block_end(env, begin, &t_end);
	if (err) return err;

	// Evaluate the condition
	jbas_token *t_cond = begin->r;
	jbas_token *t_cond_result = NULL;
	jbas_token *t_true = NULL;
	bool cond_true = false;
	err = jbas_eval_instruction(env, t_cond, &t_true, &t_cond_result);
	if (err) return err;

	// Check if the condition is true
	err = jbas_token_to_number_type(env, t_cond_result, JBAS_NUM_BOOL);	
	if (err)
	{
		// Delete the evaluation result
		jbas_token_list_destroy(t_cond_result, &env->token_pool);
		JBAS_ERROR_REASON(env, "could not convert IF condition to BOOL");
		return err;
	}
	cond_true = t_cond_result->number_token.i;

	// Delete the evaluation result
	jbas_token_list_destroy(t_cond_result, &env->token_pool);

	// Look for the matching ELSE keyword
	jbas_token *t_else;
	jbas_token *t;
	int level = 0;
	for (t = begin->r; t && (level || !(t->type == JBAS_TOKEN_KEYWORD && t->keyword_token.kw->id == JBAS_KW_ELSE)); t = t->r)
	{
		level += t->type == JBAS_TOKEN_KEYWORD && t->keyword_token.kw->id == JBAS_KW_IF;
		level -= t->type == JBAS_TOKEN_KEYWORD && t->keyword_token.kw->id == JBAS_KW_ENDIF;
		
		// No ELSE
		if (level < 0)
		{
			t = NULL;
			break;
		}
	}
	t_else = t;

	// The actual if statement :')
	if (cond_true)
	{
		err = jbas_run_block(env, t_true, t_else ? t_else : t_end, NULL);
		if (err) return err;
	}
	else if (t_else)
	{
		err = jbas_run_block(env, t_else->r, t_end, NULL);
		if (err) return err;
	}
	
	*next = t_end;
	return JBAS_OK;
}

static jbas_error jbas_kw_while(jbas_env *env, jbas_token *begin, jbas_token **next)
{
	jbas_error err;
	jbas_token *t_end;
	err = jbas_get_block_end(env, begin, &t_end);
	if (err) return err;

	while (1)
	{
		// Evaluate the condition
		jbas_token *t_cond = begin->r;
		jbas_token *t_cond_result = NULL;
		jbas_token *t_body = NULL;
		bool cond_true = false;
		err = jbas_eval_instruction(env, t_cond, &t_body, &t_cond_result);
		if (err) return err;

		// Check if the condition is true
		err = jbas_token_to_number_type(env, t_cond_result, JBAS_NUM_BOOL);	
		if (err)
		{
			// Delete the evaluation result
			jbas_token_list_destroy(t_cond_result, &env->token_pool);
			JBAS_ERROR_REASON(env, "could not convert IF condition to BOOL");
			return err;
		}
		cond_true = t_cond_result->number_token.i;

		// Delete the evaluation result
		jbas_token_list_destroy(t_cond_result, &env->token_pool);

		// Break if the condition is not true
		if (!cond_true) break;

		// Run the loop
		err = jbas_run_block(env, t_body->r, t_end, NULL);
		if (err) return err;	
	}

	*next = t_end;
	return JBAS_OK;
}

int jbas_printf(const char *format, ...)
{
	va_list ap;
	va_start(ap, format);
	int ret = vfprintf(stdout, format, ap);
	va_end(ap);
	return ret;
}

jbas_error jbas_print_token(jbas_token *t)
{

}

static jbas_error jbas_kw_print(jbas_env *env, jbas_token *begin, jbas_token **next)
{
	jbas_error err;
	jbas_token *t_next;
	jbas_token *t_val;
	err = jbas_eval_instruction(env, begin->r, &t_next, &t_val);
	if (err) return err;

	switch (t_val->type)
	{
		// Print a number
		case JBAS_TOKEN_NUMBER:
		{
			jbas_number_token *n = &t_val->number_token;
			if (n->type == JBAS_NUM_INT)
				jbas_printf("%d", n->i);
			else if (n->type == JBAS_NUM_BOOL)
				jbas_printf("%s", n->i ? "TRUE" : "FALSE");
			else
				jbas_printf("%f", n->f);
		}
		break;

		default:
			jbas_printf("???");
			break;
	}

	jbas_printf("\n");

	// Destroy the result
	jbas_token_list_destroy(t_val, &env->token_pool);

	*next = t_next;
	return JBAS_OK;
}


/**
	The keyword table
*/
const jbas_keyword jbas_keywords[JBAS_KEYWORD_COUNT] =
{
	{ 0, "NOP",   JBAS_KW_NOP,   NULL, NULL},
	{-1, "END",   JBAS_KW_END,   NULL, NULL},

	{ 1, "WHILE", JBAS_KW_WHILE, jbas_kw_while, NULL},
	{ 1, "IF",    JBAS_KW_IF,    jbas_kw_if,    NULL},
	{ 0, "ELSE",  JBAS_KW_ELSE,  NULL,          NULL},

	{ 0, "PRINT", JBAS_KW_PRINT, jbas_kw_print, NULL},
};

/**
	Return matching keyword. This function does alias resolving.
*/
const jbas_keyword *jbas_get_keyword_by_str(const char *b, const char *e)
{
	const jbas_keyword *kw = NULL;
	for (int i = 0; i < JBAS_KEYWORD_COUNT; i++)
		if (!jbas_namecmp(b, e, jbas_keywords[i].str, NULL))
		{
			kw = &jbas_keywords[i];
			break;
		}

	// Resolve alias chain
	while (kw && kw->alias)
		kw = kw->alias;
	
	return kw;
}


/**
	Returns 1 if provided token opens is an opening of a new block and
	-1 if it closes a block.
	Returns 0 otherwise.
*/
int jbas_block_level_diff(const jbas_token *t)
{
	if (!t || t->type != JBAS_TOKEN_KEYWORD) return 0;
	return t->keyword_token.kw->level_change;
}

/**
	Finds where code block ends (IF - END, FOR - END, etc.)
*/
jbas_error jbas_get_block_end(jbas_env *env, jbas_token *begin, jbas_token **match)
{
	jbas_token *t;
	int level = 1;
	for (t = begin->r; t && level; t = t->r)
		level += jbas_block_level_diff(t);
	
	*match = t;

	if (!t)
	{
		JBAS_ERROR_REASON(env, "could not find matching END for instructions block");
		return JBAS_MISSING_END;
	}

	return JBAS_OK;
}

/**
	Evaluates any keyword
*/
jbas_error jbas_eval_keyword(jbas_env *env, jbas_token *token, jbas_token **next)
{
	*next = token->r;

	if (token->type == JBAS_TOKEN_KEYWORD)
	{
		const jbas_keyword *kw = token->keyword_token.kw;
		if (kw->handler)
			return kw->handler(env, token, next);
	}

	return JBAS_OK;
}