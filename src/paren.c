#include <jbasic/paren.h>
#include <jbasic/jbasic.h>

bool jbas_is_paren(const jbas_token *t)
{
	return t && (t->type == JBAS_TOKEN_LPAREN || t->type == JBAS_TOKEN_RPAREN); 
}

/**
	Removes entire parentheses (with contents)
	\warning The provided token iterator is obviously invalidated
*/
jbas_error jbas_remove_entire_paren(jbas_env *env, jbas_token *t)
{
	jbas_error err;
	jbas_token *match;

	// Get match
	err = jbas_get_matching_paren(t, &match);
	if (err) return err;

	if (t->type == JBAS_TOKEN_LPAREN)
	{
		while (t->r != match)
		{
			err = jbas_token_list_return_to_pool(t->r, &env->token_pool);
			if (err) return err;
		}
		
		err = jbas_token_list_return_to_pool(t->r, &env->token_pool);
		if (err) return err;
	}
	else if (t->type == JBAS_TOKEN_RPAREN)
	{
		while (t->l != match)
		{
			err = jbas_token_list_return_to_pool(t->l, &env->token_pool);
			if (err) return err;
		}
		
		err = jbas_token_list_return_to_pool(t->l, &env->token_pool);
		if (err) return err;
	}

	err = jbas_token_list_return_to_pool(t, &env->token_pool);
	if (err) return err;

	return JBAS_OK;
}

/**
	Removes parenthesis if there's only one token inside.
	\note Provided list pointer remains valid as in other operations
*/
jbas_error jbas_remove_paren(jbas_env *env, jbas_token *t)
{
	jbas_token *paren = NULL;

	if (t->type == JBAS_TOKEN_LPAREN)
	{
		if (!t->r || !t->r->r) return JBAS_SYNTAX_UNMATCHED_PARENTHESIS;
		jbas_token *r = t->r->r;
		if (r->type != JBAS_TOKEN_RPAREN) return JBAS_CANNOT_REMOVE_PARENTHESIS;
		
		// Replace left parenthesis with the value from the inside
		jbas_token_swap(t, t->r);
		paren = t->r;
	}
	else if (t->type == JBAS_TOKEN_RPAREN)
	{
		if (!t->l || !t->l->l) return JBAS_SYNTAX_UNMATCHED_PARENTHESIS;
		jbas_token *l = t->l->l;
		if (l->type != JBAS_TOKEN_LPAREN) return JBAS_CANNOT_REMOVE_PARENTHESIS;
		
		// Replace right parenthesis with the value from the inside
		jbas_token_swap(t, t->l);
		paren = t->l;
	}

	// Remove the parentheses
	return jbas_remove_entire_paren(env, paren);
}

/**
	Returns a pointer to a token with matching parenthesis.
	Initially, the search is linear. Afterwards, the matching
	token is stored and the complexity is constant.

	All encountered parentheses are matched as well
*/
jbas_error jbas_get_matching_paren(jbas_token *const begin, jbas_token **match)
{
	jbas_token *t = begin;

	if (begin->paren_token.match)
	{
		if (match) *match = begin->paren_token.match;
		return JBAS_OK;
	}
	else if (t->type == JBAS_TOKEN_LPAREN)
	{
		for (t = t->r; t && t->type != JBAS_TOKEN_RPAREN; t = t->r)
			if (t->type == JBAS_TOKEN_LPAREN)
				jbas_get_matching_paren(t, &t);
	}
	else if (t->type == JBAS_TOKEN_RPAREN)
	{
		for (t = t->l; t && t->type != JBAS_TOKEN_LPAREN; t = t->l)
			if (t->type == JBAS_TOKEN_RPAREN)
				jbas_get_matching_paren(t, &t);
	}

	if (!t) return JBAS_SYNTAX_UNMATCHED_PARENTHESIS;
	begin->paren_token.match = t;
	t->paren_token.match = begin;
	if (match) *match = t;
	return JBAS_OK;
}


/**
	Evaluates everything inside the parenthesis (and removes it if there's only one token in between)
	\note Provided token pointer is not invalidated and will point to the parenthesis contents
*/
jbas_error jbas_eval_paren(jbas_env *env, jbas_token *t)
{
	jbas_token *begin, *end, *match;
	jbas_error err;

	err = jbas_get_matching_paren(t, &match);
	if (err) return err;

	if (t->type == JBAS_TOKEN_LPAREN)
	{
		begin = t->r;
		end = match;
	}
	else
	{
		begin = match->r;
		end = t;
	}

	err = jbas_eval(env, begin, end, NULL);
	if (err) return err;
	
	return jbas_remove_paren(env, t);
}
