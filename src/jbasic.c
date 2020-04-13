#include <jbasic/jbasic.h>
#include <jbasic/paren.h>
#include <jbasic/cast.h>
#include <jbasic/debug.h>



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
	Returns true or false depending on whether the character
	can be a part of program variable or sub
*/
int jbas_is_name_char(char c)
{
	return isalpha(c) || (c == '_');
}



jbas_error jbas_print(const jbas_token *token, int count)
{
	while (count--)
	{
		switch (token->type)
		{
			// Print a string
			case JBAS_TOKEN_STRING:
			{
				const jbas_string_token *t = &token->string_token;
				printf("%s", t->txt->str);
			}
			break;

			default:
				return JBAS_PRINT_BAD_ARGUMENT;
				break;
		}
		
		if (count) printf("\t");
		token++;
	}

	printf("\n");
	return JBAS_OK;
}




/**
	Evaluates expression (no keywords allowed)
*/
jbas_error jbas_eval(jbas_env *env, jbas_token *const begin, jbas_token *const end, jbas_token **result)
{
	jbas_operator_sort_bucket operators[JBAS_MAX_EVAL_OPERATORS];
	size_t opcnt = 0;

	// Find all operators (including opening parenthesis)
	for (jbas_token *t = begin; t && t != end; t = t->r)
	{
		// Operators
		if (t->type == JBAS_TOKEN_OPERATOR)
		{
			// Overflow
			if (opcnt == JBAS_MAX_EVAL_OPERATORS)
			{
				JBAS_ERROR_REASON(env, "too many opeartors in chunk passed to jbas_eval(). Try changing JBAS_MAX_EVAL_OPERATORS");
				return JBAS_EVAL_OVERFLOW;
			}

			operators[opcnt].token = t;
			operators[opcnt].pos = opcnt;
			opcnt++;
		}

		// Skip parenthesis and register parentheses
		// Call operator has the highest priority so it must be next
		// to a callable token straight away
		if (t->type == JBAS_TOKEN_LPAREN)
		{
			if (jbas_has_left_operand(t))
			{
				operators[opcnt].token = t;
				operators[opcnt].pos = opcnt;
				opcnt++;
			}
			jbas_get_matching_paren(t, &t);
		}
	}

	// Find operators that are actually unary - forward pass - postfix operators
	for (int i = 0; i < opcnt; i++)
		jbas_try_fallback_operator(operators[i].token, JBAS_OP_UNARY_POSTFIX);

	// Find operators that are actually unary - backward pass - prefix operators
	for (int i = opcnt - 1; i >= 0; i--)
		jbas_try_fallback_operator(operators[i].token, JBAS_OP_UNARY_PREFIX);

	// Sort the operators
	qsort(operators, opcnt, sizeof(operators[0]), jbas_operator_token_compare);

	// Evaluate operators
	for (int i = 0; i < opcnt; i++)
	{
		jbas_token *t = operators[i].token;

		// Binary and unary operators
		if (t->type == JBAS_TOKEN_OPERATOR)
		{
			jbas_error err;
			if (jbas_is_binary_operator(t)) err = jbas_eval_binary_operator(env, t);
			else err = jbas_eval_unary_operator(env, t);
			if (err) return err;

		}
		else // Call operator (LPAREN)
		{
			jbas_error err = jbas_eval_call_operator(env, t->l, t);
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
	Executes one instruction - starting at the provided token and ending at matching delimiter
	(; for normal instructions, ENDIF for IFs and so on...)
*/
jbas_error jbas_run_instruction(jbas_env *env, jbas_token **next, jbas_token *const token)
{
	// Skip delimiters
	jbas_token *begin_token = token;
	while (begin_token && begin_token->type == JBAS_TOKEN_DELIMITER)
		begin_token = begin_token->r;

	// Create copy of the entire expression
	//! \todo keyword delimiters
	jbas_token *t, *expr = NULL;
	for (t = begin_token; t && t->type != JBAS_TOKEN_DELIMITER; t = t->r)
	{
		jbas_token *new_token = NULL;
		jbas_error err = jbas_token_list_push_back_from_pool(expr, &env->token_pool, t, &new_token);
		if (err) return err;
		expr = new_token;
	}
	*next = t;

	// DEBUG
	#ifdef JBAS_DEBUG
	fprintf(stderr, "Will evaluate: ");
	for (jbas_token *t = jbas_token_list_begin(expr); t; t = t->r)
		jbas_debug_dump_token(stderr, t);
	fprintf(stderr, "\n");
	#endif

	jbas_error eval_err = jbas_eval(env, jbas_token_list_begin(expr), NULL, NULL);
	
	// DEBUG
	#ifdef JBAS_DEBUG
	fprintf(stderr, "Eval err: %d\n", eval_err);
	fprintf(stderr, "After eval: ");
	for (jbas_token *t = jbas_token_list_begin(expr); t; t = t->r)
		jbas_debug_dump_token(stderr, t);
	fprintf(stderr, "\n");
	#endif

	// Delete what's left
	jbas_token_list_destroy(expr, &env->token_pool);
	return JBAS_OK;
}

/**
	Runs entire loaded program
*/
jbas_error jbas_run_tokens(jbas_env *env)
{
	jbas_token *token = jbas_token_list_begin(env->tokens);
	while (token)
	{
		jbas_error err = jbas_run_instruction(env, &token, token);
		if (err) return err;
	}

	return JBAS_OK;
}

/**
	Returns next token from a code line.
	If there are no more tokens in the current line, address of the next token is returned as NULL

	\todo split this function into more, each handling one type of tokens
*/
jbas_error jbas_get_token(jbas_env *env, const char *const str, const char **next, jbas_token *token)
{
	const char *s = str;

	// Skip preceding whitespace
	while (*s && isspace(*s) && *s != '\n') s++;
	if (!*s)
	{
		*next = NULL;
		return JBAS_EMPTY_TOKEN;
	}

	// If it's ';' or a newline, it's a delimiter
	if (*s == ';' || *s == '\n')
	{
		*next = s + 1;
		token->type = JBAS_TOKEN_DELIMITER;
		return JBAS_OK;
	}

	// Left parenthesis
	if (*s == '(')
	{
		*next = s + 1;
		token->type = JBAS_TOKEN_LPAREN;
		token->paren_token.match = NULL;
		return JBAS_OK;
	}

	// Right parenthesis
	if (*s == ')')
	{
		*next = s + 1;
		token->type = JBAS_TOKEN_RPAREN;
		token->paren_token.match = NULL;
		return JBAS_OK;
	}

	// If the token starts with a number, it is a number
	if (isdigit(*s))
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
		token->number_token.type = is_int ? JBAS_NUM_INT : JBAS_NUM_FLOAT;
		if (is_int)
			sscanf(s, "%d", &token->number_token.i);
		else
			sscanf(s, "%f", &token->number_token.f);

		*next = num_end;
		token->type = JBAS_TOKEN_NUMBER;
		return JBAS_OK;		
	}

	// If the token starts with ', " or `, it is a string
	if (*s == '\'' || *s == '\"' || *s == '`')
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
		token->type = JBAS_TOKEN_STRING;

		jbas_error err = jbas_text_create(
			&env->text_manager,
			s + 1,
			str_end,
			&token->string_token.txt);

		return err;
	}

	// If the token starts with a letter, it must be treated as a keyword/operator/symbol name
	if (jbas_is_name_char(*s))
	{
		const char *name_end = s;
		while (jbas_is_name_char(*name_end)) name_end++;
		*next = name_end;

		// Operator check
		const jbas_operator *op = jbas_get_operator_by_str(s, name_end);
		if (op)
		{
			token->type = JBAS_TOKEN_OPERATOR;
			token->operator_token.op = op;
			return JBAS_OK;
		}

		// Keyword check
		for (int i = 0; i < JBAS_KEYWORD_COUNT; i++)
		{
			if (!jbas_namecmp(s, name_end, jbas_keywords[i].name, NULL))
			{
				// Found matching keyword
				token->type = JBAS_TOKEN_KEYWORD;
				token->keyword_token.id = jbas_keywords[i].id;
				return JBAS_OK;
			}
		}

		// Symbol
		token->type = JBAS_TOKEN_SYMBOL;	
		jbas_symbol *sym;
		jbas_error err;
		
		err = jbas_symbol_create(env, &sym, s, name_end);
		if (err == JBAS_OK || err == JBAS_SYMBOL_COLLISION)
			token->symbol_token.sym = sym;
		else
			return err;

		return JBAS_OK;
	}

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
		token->type = JBAS_TOKEN_OPERATOR;
		token->operator_token.op = match_op;
		*next = match_end;
		return JBAS_OK;
	} 

	// Bad token!
	JBAS_ERROR_REASON(env, "bad syntax! could not tokenize!");
	return JBAS_SYNTAX_ERROR;
}


/**
	Performs line tokenization, parsing and executes it in the provided environment
*/
jbas_error jbas_tokenize_string(jbas_env *env, const char *str)
{
	while (1)
	{
		// Get a token from the line
		jbas_token t;
		jbas_error err = jbas_get_token(env, str, &str, &t);

		if (!err) // Insert a valid token into the list
		{
			jbas_token *new_token;
			jbas_error err = jbas_token_list_push_back_from_pool(env->tokens, &env->token_pool, &t, &new_token);
			env->tokens = new_token;
			if (err) return err;
		}
		else if (err == JBAS_EMPTY_TOKEN)
			break;
		else
			return err;
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
