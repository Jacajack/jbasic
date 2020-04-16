#include <jbasic/jbasic.h>
#include <jbasic/paren.h>
#include <jbasic/cast.h>
#include <jbasic/kw.h>
#include <jbasic/debug.h>

/**
	Returns true or false depending on whether the character
	can be a part of program variable or sub
*/
bool jbas_is_name_char(char c)
{
	return isalpha(c) || (c == '_');
}


int jbas_namecmp(const char *s1, const char *end1, const char *s2, const char *end2)
{
	if (!end1 && !end2) return strcasecmp(s1, s2);
	if (end1 && end2)
	{
		size_t l1 = end1 - s1;
		size_t l2 = end2 - s2;

		if (l1 != l2) return (end1 - s1) - (end2 - s2);
		else return strncasecmp(s1, s2, l1);
	}

	size_t l1, l2;
	if (end1)
	{
		l1 = end1 - s1;
		l2 = strlen(s2);
		if (l1 != l2) return (end1 - s1) - (end2 - s2);
		else return strncasecmp(s1, s2, l1);
	}
	else
	{
		l1 = strlen(s1);
		l2 = end2 - s2;
		if (l1 != l2) return (end1 - s1) - (end2 - s2);
		else return strncasecmp(s1, s2, l1);
	}
}


/**
	Evaluates expression (keywords are not handled here)
	The resulting tokens are returned through the `result` argument
*/
jbas_error jbas_eval(jbas_env *env, jbas_token *const begin, jbas_token *const end, jbas_token **result)
{
	if (!begin)
	{
		if (result) *result = NULL;
		return JBAS_OK;
	}

	jbas_operator_sort_bucket operators[JBAS_MAX_EVAL_OPERATORS];
	size_t opcnt = 0;

	// Find all operands (includes operator fallback)
	for (jbas_token *t = begin; t && t != end; t = t->r)
	{
		if (jbas_is_pure_operand(t))
			jbas_attach_unary_operators(t);
	}

	// Find all binary operators
	for (jbas_token *t = begin; t && t != end; t = t->r)
	{
		// Operators
		if (jbas_is_binary_operator(t))
		{
			// Overflow
			if (opcnt == JBAS_MAX_EVAL_OPERATORS)
			{
				JBAS_ERROR_REASON(env, "too many binary opeartors in chunk passed to jbas_eval(). Try changing JBAS_MAX_EVAL_OPERATORS");
				return JBAS_EVAL_OVERFLOW;
			}

			operators[opcnt].token = t;
			operators[opcnt].pos = opcnt;
			opcnt++;
		}
	}

	// Sort the binary operators
	qsort(operators, opcnt, sizeof(operators[0]), jbas_operator_token_compare);

	// If there are no binary operators and the expression itself is only
	// an operand, evaluate it anyway. This ensures that function calls
	// are evaluated and prevents overly aggressive optimization
	//! \todo fix infinite loop when this is uncommented
	/*
	if (!opcnt && jbas_is_operand(begin))
	{
		jbas_error err = jbas_eval_operand(env, begin);
		if (err) return err;
	}
	*/

	// Evaluate operators
	for (int i = 0; i < opcnt; i++)
	{
		jbas_token *t = operators[i].token;

		// Binary operators
		if (t->type == JBAS_TOKEN_OPERATOR)
		{
			jbas_error err = jbas_eval_binary_operator(env, t);
			if (err) return err;
		}

		#ifdef JBAS_DEBUG
		fprintf(stderr, " >> ");
		jbas_debug_dump_token_list(stderr, t);
		fprintf(stderr, "\n");
		#endif 
	}

	if (result) *result = opcnt ? operators[opcnt - 1].token : begin;
	return JBAS_OK;
}


/**
	Evaluates instruction (up to a delimiter) and optionally returns the result
	\warning The returned tokens have to be returned to the pool by the user.
*/
jbas_error jbas_eval_instruction(jbas_env *env, jbas_token *begin, jbas_token **next, jbas_token **result)
{
	// Skip delimiters
	while (begin && begin->type == JBAS_TOKEN_DELIMITER)
		begin = begin->r;
	
	// Reached the end?
	if (!begin)
	{
		*next = NULL;
		return JBAS_OK;
	}

	// Create a deep copy of the entire instruction
	jbas_token *t, *expr = NULL;
	for (t = begin; t && t->type != JBAS_TOKEN_DELIMITER; t = t->r)
	{
		jbas_error err;
		jbas_token new_token = {.type = JBAS_TOKEN_DELIMITER};
		err = jbas_token_copy(&new_token, t, &env->token_pool);
		if (err) return err;
		err = jbas_token_list_push_back_from_pool(expr, &env->token_pool, &new_token, &expr);
		if (err) return err;
	}
	*next = t;

	// DEBUG
	#ifdef JBAS_DEBUG
	fprintf(stderr, "Will evaluate: ");
	for (jbas_token *t = jbas_token_list_begin(expr); t; t = t->r)
		jbas_debug_dump_token(stderr, t);
	fprintf(stderr, "\n");
	#endif

	jbas_error eval_err = jbas_eval(env, jbas_token_list_begin(expr), NULL, &expr);
	
	// DEBUG
	#ifdef JBAS_DEBUG
	fprintf(stderr, "Eval err: %d\n", eval_err);
	fprintf(stderr, "After eval: ");
	for (jbas_token *t = jbas_token_list_begin(expr); t; t = t->r)
		jbas_debug_dump_token(stderr, t);
	fprintf(stderr, "\n");
	#endif

	// Eval error
	if (eval_err)
	{
		if (result) *result = NULL;
		jbas_token_list_destroy(expr, &env->token_pool);
		return eval_err;
	}

	// Either return or delete the result
	if (result)
		*result = jbas_token_list_begin(expr);
	else
		jbas_token_list_destroy(expr, &env->token_pool);

	return JBAS_OK;
}

/**
	Executes either an enitre block or a single instruction.
*/
jbas_error jbas_run_step(jbas_env *env, jbas_token *begin, jbas_token **next)
{
	// Skip delimiters
	while (begin && begin->type == JBAS_TOKEN_DELIMITER)
		begin = begin->r;

	// Check the token we're on
	if (!begin)
	{
		*next = NULL;
		return JBAS_OK;
	}

	// Handle keywords
	if (begin->type == JBAS_TOKEN_KEYWORD)
	{
		return jbas_eval_keyword(env, begin, next);
	}
	else
	{
		// Handle normal instruction and discard the result
		return jbas_eval_instruction(env, begin, next, NULL);
	}

	return JBAS_OK;
}

/**
	Runs program block
*/
jbas_error jbas_run_block(jbas_env *env, jbas_token *begin, jbas_token *end, jbas_token **next)
{
	// Create fake list end where we want the execution to halt
	if (end && end->l) end->l->r = NULL;

	// Run step by step
	jbas_token *t;
	jbas_error err = JBAS_OK;
	for (t = begin; t; err = jbas_run_step(env, t, &t))
		if (err) break;

	// Restore list continuity
	if (end && end->l) end->l->r = end;
	if (next) *next = end;
	return err;
}


/**
	Runs entire loaded program
*/
jbas_error jbas_run(jbas_env *env)
{
	return jbas_run_block(env, jbas_token_list_begin(env->tokens), NULL, NULL);
}

/**
	Returns next token from a code line.
	If there are no more tokens in the current line, address of the next token is returned as NULL

	\todo split this function into more, each handling one type of tokens
*/
jbas_error jbas_get_token(jbas_env *env, const char *const str, const char **next, jbas_token ***lists, int *level)
{
	const char *s = str;
	bool ok = false;
	jbas_token token;

	// Skip preceding whitespace
	while (*s && isspace(*s) && *s != '\n') s++;
	if (!*s)
	{
		*next = NULL;
		return JBAS_OK;
	}

	// Skip comments and treat them as delimiters
	if (*s == '#')
	{
		while (*s && *s != '\n') s++;
		*next = *s ? s + 1 : NULL;
		token.type = JBAS_TOKEN_DELIMITER;
		ok = true;
	}

	// If it's ';' or a newline, it's a delimiter
	if (!ok && (*s == ';' || *s == '\n'))
	{
		*next = s + 1;
		token.type = JBAS_TOKEN_DELIMITER;
		ok = true;
	}

	// Left parenthesis - return pointer to a new list
	if (!ok && (*s == '('))
	{
		*next = s + 1;
		token.type = JBAS_TOKEN_PAREN;
		token.paren_token.tokens = NULL;

		jbas_error err = jbas_token_list_push_back_from_pool(*(lists[*level - 1]),
			&env->token_pool,
			&token,
			lists[*level - 1]);
		
		// Push new list onto the stack
		lists[*level] = &(*(lists[*level -1]))->paren_token.tokens;
		(*level)++;
		return err;
	}

	// Right parenthesis
	if (!ok && (*s == ')'))
	{
		// Actually exit here
		*next = s + 1;

		// Stack pop
		(*level)--;
		return JBAS_OK;
	}

	// If the token starts with a number, it is a number
	if (!ok && isdigit(*s))
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
		token.number_token.type = is_int ? JBAS_NUM_INT : JBAS_NUM_FLOAT;
		if (is_int)
			sscanf(s, "%d", &token.number_token.i);
		else
			sscanf(s, "%f", &token.number_token.f);

		*next = num_end;
		token.type = JBAS_TOKEN_NUMBER;
		ok = true;	
	}

	// If the token starts with ', " or `, it is a string
	if (!ok && (*s == '\'' || *s == '\"' || *s == '`'))
	{
		const char delimiter = *s;
		const char *str_end = s + 1;
		while (*str_end && *str_end != delimiter) str_end++;
		
		// If we reached end of the line
		if (!*str_end)
		{
			return JBAS_SYNTAX_UNMATCHED_QUOTE;
		}

		*next = str_end + 1;
		token.type = JBAS_TOKEN_STRING;

		jbas_error err = jbas_text_create(
			&env->text_manager,
			s + 1,
			str_end,
			&token.string_token.txt);
		if (err) return err;

		ok = true;
	}

	// If the token starts with a letter, it must be treated as a keyword/operator/symbol name
	if (!ok && (jbas_is_name_char(*s)))
	{
		const char *name_end = s;
		while (jbas_is_name_char(*name_end)) name_end++;
		*next = name_end;

		// Operator check
		const jbas_operator *op = jbas_get_operator_by_str(s, name_end);
		if (op)
		{
			token.type = JBAS_TOKEN_OPERATOR;
			token.operator_token.op = op;
			ok = true;
		}

		// Keyword check
		const jbas_keyword *kw = jbas_get_keyword_by_str(s, name_end);
		if (!ok && kw)
		{
			token.type = JBAS_TOKEN_KEYWORD;
			token.keyword_token.kw = kw;
			ok = true;
		}

		if (!ok)
		{
			// Symbol
			token.type = JBAS_TOKEN_SYMBOL;	
			jbas_symbol *sym;
			jbas_error err;
			
			err = jbas_symbol_create(env, &sym, s, name_end);
			if (err == JBAS_OK || err == JBAS_SYMBOL_COLLISION)
				token.symbol_token.sym = sym;
			else
				return err;

			ok = true;
		}
	}

	if (!ok)
	{
		// Max crunch operator
		const char *match_end = NULL;
		const jbas_operator *match_op = NULL;
		for (const char *op_end = s + 1; *op_end && jbas_is_operator_char(*(op_end - 1)); op_end++)
		{
			const jbas_operator *op = jbas_get_operator_by_str(s, op_end);
			if (op)
			{
				match_op = op;
				match_end = op_end;
			}
		}
		
		// Found a valid operator
		if (match_op)
		{
			token.type = JBAS_TOKEN_OPERATOR;
			token.operator_token.op = match_op;
			*next = match_end;
			ok = true;
		}
	}

	// If we have a token
	if (ok)
	{
		// Add the token to the list and update the list pointer
		return jbas_token_list_push_back_from_pool(*(lists[*level - 1]),
			&env->token_pool,
			&token,
			lists[*level - 1]);
	}

	// Bad token!
	JBAS_ERROR_REASON(env, "bad syntax! could not tokenize!");
	return JBAS_SYNTAX_ERROR;
}


/**
	Performs line tokenization
*/
jbas_error jbas_tokenize_string(jbas_env *env, const char *str)
{
	jbas_token **paren[JBAS_TOKENIZE_PAREN_LEVELS];
	paren[0] = &env->tokens;
	int level = 1;

	while (1)
	{
		// Get a token from the line
		jbas_error err = jbas_get_token(env, str, &str, paren, &level);
		if (!str) break;
		if (err) return err;

		if (!level || level >= JBAS_TOKENIZE_PAREN_LEVELS)
		{
			return JBAS_TOO_MANY_PAREN;
		}
	}

	env->tokens = jbas_token_list_end(env->tokens);

	// Make sure there's a delimiter at the end
	if (env->tokens && env->tokens->type != JBAS_TOKEN_DELIMITER)
	{
		jbas_token t = {.type = JBAS_TOKEN_DELIMITER};
		jbas_error err = jbas_token_list_push_back_from_pool(env->tokens, &env->token_pool, &t, &env->tokens);
		if (err) return err;
	}

	return JBAS_OK;
}



jbas_error jbas_env_init(jbas_env *env, int token_count, int text_count, int symbol_count, int resource_count)
{
	env->tokens = NULL;
	env->error_reason = NULL;
	jbas_error err;

	err = jbas_token_pool_init(&env->token_pool, token_count);
	if (err) return err;

	err = jbas_text_manager_init(&env->text_manager, text_count);
	if (err) return err;

	err = jbas_symbol_manager_init(&env->symbol_manager, symbol_count);
	if (err) return err;

	err = jbas_resource_manager_init(&env->resource_manager, resource_count);
	if (err) return err;

	return JBAS_OK;
}

void jbas_env_destroy(jbas_env *env)
{
	jbas_token_pool_destroy(&env->token_pool);
	jbas_text_manager_destroy(&env->text_manager);
	jbas_symbol_manager_destroy(&env->symbol_manager);
	jbas_resource_manager_destroy(&env->resource_manager);
}
