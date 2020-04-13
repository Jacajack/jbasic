#include <jbasic/jbasic.h>
#include <jbasic/debug.h>

/**
	Removes parentheses and everything in between
	\warning The provided token pointer is invalidated!!
*/
/*
jbas_error jbas_remove_paren(jbas_env *env, jbas_token *t)
{
	jbas_token *match, *h;
	jbas_error err = jbas_get_matching_paren(t, &match);

	if (t->type == JBAS_TOKEN_LPAREN)
	{
		h = t;
	}
}
*/

bool jbas_is_paren(const jbas_token *t)
{
	return t && (t->type == JBAS_TOKEN_LPAREN || t->type == JBAS_TOKEN_RPAREN); 
}

/**
	Removes entire parentheses (with contents)
	The provided token iterator is obviously invalidated
*/
jbas_error jbas_remove_entire_paren(jbas_env *env, jbas_token *t)
{
	jbas_error err;
	jbas_token *h, *match;

	// Get match
	err = jbas_get_matching_paren(t, &match);
	if (err) return err;

	if (t->type == JBAS_TOKEN_LPAREN)
	{
		for (h = t->r; t->r != match; h = t->r)
		{
			err = jbas_token_list_return_to_pool(&h, &env->token_pool);
			if (err) return err;
		}
		
		h = t->r;
		err = jbas_token_list_return_to_pool(&h, &env->token_pool);
		if (err) return err;
	}
	else if (t->type == JBAS_TOKEN_RPAREN)
	{
		for (h = t->l; t->l != match; h = t->l)
		{
			err = jbas_token_list_return_to_pool(&h, &env->token_pool);
			if (err) return err;
		}
		
		h = t->l;
		err = jbas_token_list_return_to_pool(&h, &env->token_pool);
		if (err) return err;
	}

	h = t;
	err = jbas_token_list_return_to_pool(&h, &env->token_pool);
	if (err) return err;

	return JBAS_OK;
}

/**
	Removes parenthesis if there's only one token inside.
	\note Provided list pointer remains valid as in other operations
*/
jbas_error jbas_remove_paren(jbas_env *env, jbas_token *t)
{
	if (t->type == JBAS_TOKEN_LPAREN)
	{
		if (!t->r) return JBAS_SYNTAX_UNMATCHED_PARENTHESIS;
		jbas_token *r = t->r->r;
		if (r->type != JBAS_TOKEN_RPAREN) return JBAS_CANNOT_REMOVE_PARENTHESIS;
		
		// Replace left parenthesis with the value from the inside
		jbas_token_swap(t, t->r);

		jbas_error err;
		err = jbas_remove_entire_paren(env, t->r);
		if (err) return err;
	}
	else if (t->type == JBAS_TOKEN_RPAREN)
	{
		if (!t->l) return JBAS_SYNTAX_UNMATCHED_PARENTHESIS;
		jbas_token *l = t->l->l;
		if (l->type != JBAS_TOKEN_LPAREN) return JBAS_CANNOT_REMOVE_PARENTHESIS;
		
		// Replace right parenthesis with the value from the inside
		jbas_token_swap(t, t->l);

		jbas_error err;
		err = jbas_remove_entire_paren(env, t->l);
		if (err) return err;

	}

	return JBAS_OK;
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


jbas_number_type jbas_number_type_promotion(jbas_number_type a, jbas_number_type b)
{
	if (a >= b) return a;
	else return b;
}

void jbas_number_cast(jbas_number_token *n, jbas_number_type t)
{
	if (n->type == t) return;
	n->type = t;
	if (t == JBAS_NUM_INT)
		n->i = n->f;
	else if (t == JBAS_NUM_FLOAT)
		n->f = n->i;
	else if (t == JBAS_NUM_BOOL)
	{
		if (n->type == JBAS_NUM_FLOAT)
			n->i = n->f != 0;
		else
			n->i = n->i != 0;
	}
}

/**
	Returns true if provided token is a scalar
*/
bool jbas_is_scalar(jbas_token *t)
{
	if (t->type != JBAS_TOKEN_SYMBOL) return true;
	else
	{
		jbas_resource *res = t->symbol_token.sym->resource;
		if (!res) return true;

		switch (res->type)
		{
			case JBAS_RESOURCE_NUMBER:
				return true;

			case JBAS_RESOURCE_STRING:
				return true;
				break;

			default:
				return false;
				break;
		}
	}
}

/*
	Evaluates a scalar symbol
*/
jbas_error jbas_eval_scalar_symbol(jbas_env *env, jbas_token *t)
{
	if (t->type != JBAS_TOKEN_SYMBOL) return JBAS_OK;
	if (!jbas_is_scalar(t)) return JBAS_OK;
	jbas_symbol *sym = t->symbol_token.sym;
	jbas_token res;

	if (!sym->resource) return JBAS_UNINITIALIZED_SYMBOL;
	switch (sym->resource->type)
	{
		case JBAS_RESOURCE_NUMBER:
			res.type = JBAS_TOKEN_NUMBER;
			res.number_token = sym->resource->number;
			break;

		default:
			return JBAS_CANNOT_EVAL_RESOURCE;
			break;
	}

	jbas_token_copy(t, &res);
	return JBAS_OK;
}

jbas_error jbas_token_to_number(jbas_env *env, jbas_token *t)
{
	if (t->type == JBAS_TOKEN_NUMBER) return JBAS_OK;
	else if (t->type == JBAS_TOKEN_LPAREN || t->type == JBAS_TOKEN_RPAREN)
	{
		jbas_error err;
		err = jbas_eval_paren(env, t);
		if (err) return err;
		return jbas_token_to_number(env, t);
	}
	else if (t->type == JBAS_TOKEN_SYMBOL)
	{
		jbas_error err;
		err = jbas_eval_scalar_symbol(env, t);
		if (err) return err;
		return jbas_token_to_number(env, t);
	}

	return JBAS_CAST_FAILED;
}

jbas_error jbas_token_to_number_type(jbas_env *env, jbas_token *t, jbas_number_type type)
{
	jbas_error err = jbas_token_to_number(env, t);
	if (err) return err;
	jbas_number_cast(&t->number_token, type);
	return JBAS_OK;
}



bool jbas_is_valid_operand(jbas_token *t)
{
	if (!t) return false;
	return t->type == JBAS_TOKEN_SYMBOL
		|| t->type == JBAS_TOKEN_NUMBER
		|| t->type == JBAS_TOKEN_STRING
		|| t->type == JBAS_TOKEN_LPAREN
		|| t->type == JBAS_TOKEN_RPAREN
		|| t->type == JBAS_TOKEN_TUPLE;
}

/**
	Returns true if provided operator has left operand
*/
bool jbas_has_left_operand(jbas_token *t)
{
	return jbas_is_valid_operand(t->l) && t->l->type != JBAS_TOKEN_LPAREN;
}

/**
	Returns true if provided operator has right operand
*/
bool jbas_has_right_operand(jbas_token *t)
{
	return jbas_is_valid_operand(t->r) && t->r->type != JBAS_TOKEN_RPAREN;
}

jbas_error jbas_remove_left_operand(jbas_env *env, jbas_token *t)
{
	if (!t->l) return JBAS_OPERAND_MISSING;
	if (t->l->type == JBAS_TOKEN_RPAREN) // Remove entire parenthesis
	{
		// Remove the parentheses
		jbas_token *h = t->l;
		jbas_error err = jbas_token_list_return_to_pool(&h, &env->token_pool);
		if (err) return err;

		// Remove everything up to the matching parentheses
		int level = 0;
		for (h = t->l; h && (h->type != JBAS_TOKEN_LPAREN || level); h = t->l)
		{
			level += h->type == JBAS_TOKEN_RPAREN;
			level -= h->type == JBAS_TOKEN_LPAREN;
			jbas_error err = jbas_token_list_return_to_pool(&h, &env->token_pool);
			if (err) return err;
		}

		if (!h) return JBAS_SYNTAX_UNMATCHED_PARENTHESIS;
		h = t->l;
		return jbas_token_list_return_to_pool(&h, &env->token_pool);	
	}
	else
	{
		jbas_token *h = t->l;
		return jbas_token_list_return_to_pool(&h, &env->token_pool);	
	}
	
	return JBAS_OK;
}


jbas_error jbas_remove_right_operand(jbas_env *env, jbas_token *t)
{
	if (!t->r) return JBAS_OPERAND_MISSING;
	if (t->r->type == JBAS_TOKEN_LPAREN) // Remove entire parenthesis
	{
		// Remove the parentheses
		jbas_token *h = t->r;
		jbas_error err = jbas_token_list_return_to_pool(&h, &env->token_pool);
		if (err) return err;

		// Remove everything up to the matching parentheses
		int level = 0;
		for (h = t->r; h && (h->type != JBAS_TOKEN_RPAREN || level); h = t->r)
		{
			level += h->type == JBAS_TOKEN_LPAREN;
			level -= h->type == JBAS_TOKEN_RPAREN;
			jbas_error err = jbas_token_list_return_to_pool(&h, &env->token_pool);
			if (err) return err;
		}

		if (!h) return JBAS_SYNTAX_UNMATCHED_PARENTHESIS;
		h = t->r;
		return jbas_token_list_return_to_pool(&h, &env->token_pool);	
	}
	else
	{
		jbas_token *h = t->r;
		return jbas_token_list_return_to_pool(&h, &env->token_pool);	
	}
	
	return JBAS_OK;
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

jbas_error jbas_eval_unary_operator(jbas_env *env, jbas_token *t, jbas_operator_type unary_type)
{
	if (!unary_type) unary_type = t->operator_token.op->type;
	if (unary_type == JBAS_OP_UNARY_PREFIX)
	{
		if (!jbas_has_left_operand(t) && jbas_has_right_operand(t))
		{
			// Evaluate parenthesis
			if (t->r->type == JBAS_TOKEN_LPAREN) jbas_eval_paren(env, t->r);

			// Call the operator handler and have it replaced with operation result
			t->operator_token.op->handler(env, NULL, t->r, t);

			// Remove operand
			jbas_error err;
			err = jbas_remove_right_operand(env, t);
			if (err) return err;
		}
		else
			return JBAS_OPERAND_MISSING;
	}
	else if (unary_type == JBAS_OP_UNARY_POSTFIX)
	{
		if (jbas_has_left_operand(t) && !jbas_has_right_operand(t))
		{
			// Evaluate parenthesis
			if (t->l->type == JBAS_TOKEN_RPAREN) jbas_eval_paren(env, t->l);

			// Call the operator handler and have it replaced with operation result
			t->operator_token.op->handler(env, t->l, NULL, t);

			// Remove operand
			jbas_error err;
			err = jbas_remove_left_operand(env, t);
			if (err) return err;
		}
		else
			return JBAS_OPERAND_MISSING;
	}

	return JBAS_OK;
}

jbas_error jbas_eval_binary_operator(jbas_env *env, jbas_token *t)
{
	if (jbas_has_left_operand(t) && jbas_has_right_operand(t))
	{
		// Evaluate parenthesis
		if (t->l->type == JBAS_TOKEN_RPAREN) jbas_eval_paren(env, t->l);
		if (t->r->type == JBAS_TOKEN_LPAREN) jbas_eval_paren(env, t->r);

		// Call the operator handler and have it replaced with operation result
		t->operator_token.op->handler(env, t->l, t->r, t);

		// Remove operands
		jbas_error err;
		err = jbas_remove_left_operand(env, t);
		if (err) return err;
		err = jbas_remove_right_operand(env, t);
		if (err) return err;
	}
	else
		return JBAS_OPERAND_MISSING;
	return JBAS_OK;
}

jbas_error jbas_eval_call_operator(jbas_env *env, jbas_token *fun, jbas_token *args)
{
	JBAS_ERROR_REASON(env, "object is not callable!");
	return JBAS_BAD_CALL;

	// Remove args after call
	jbas_error err;
	err = jbas_remove_right_operand(env, fun);
	if (err) return err;
	return JBAS_OK;
}

/**
	Operator token comparison function for qsort.
	Operators that should be evaluated first will 
	appear at the beginning of the sorted array.
*/
int jbas_operator_token_compare(const void *av, const void *bv)
{
	const jbas_token *a = *(const jbas_token**)av;
	const jbas_token *b = *(const jbas_token**)bv;

	if (a->type == JBAS_TOKEN_LPAREN) return -1;
	else if (b->type == JBAS_TOKEN_LPAREN) return 1;
	else
	{
		const jbas_operator *aop = a->operator_token.op;
		const jbas_operator *bop = b->operator_token.op;
		return aop->level == bop->level ? bop->type - aop->type : bop->level - aop->level;
	}
}

jbas_error jbas_eval(jbas_env *env, jbas_token *const begin, jbas_token *const end, jbas_token **result)
{
	jbas_token *operators[JBAS_MAX_EVAL_OPERATORS];
	size_t opcnt = 0;

	// Find all operators (including opening parentheses)
	for (jbas_token *t = begin; t && t != end; t = t->r)
	{
		// Overflow
		if (opcnt == JBAS_MAX_EVAL_OPERATORS)
		{
			JBAS_ERROR_REASON(env, "too many opeartors in chunk passed to jbas_eval(). Try changing JBAS_MAX_EVAL_OPERATORS");
			return JBAS_EVAL_OVERFLOW;
		}

		// Operators
		if (t->type == JBAS_TOKEN_OPERATOR)
		{
			const jbas_operator *op = t->operator_token.op;
			bool has_left = jbas_has_left_operand(t);
			bool has_right = jbas_has_right_operand(t);

			// Binary operator has both operands
			if (op->type == JBAS_OP_BINARY_LR || op->type == JBAS_OP_BINARY_RL)
			{
				// Has both operands
				if (has_left && has_right)
				{
					operators[opcnt++] = t;
					continue;
				}
				else if (has_left != has_right) // Fallback unary
					op = t->operator_token.op = op->fallback;
			}

			// Unary operator with matching operand
			if (op && ((op->type == JBAS_OP_UNARY_PREFIX && has_right) || (op->type == JBAS_OP_UNARY_POSTFIX && has_left)))
			{
				operators[opcnt++] = t;
				continue;
			}

			// Fail
			JBAS_ERROR_REASON(env, "operand missing (at operator search level)");
			return JBAS_OPERAND_MISSING;
		}

		// Parenthesis skipping and call operator registration
		if (t->type == JBAS_TOKEN_LPAREN )
		{
			if (jbas_has_left_operand(t)) operators[opcnt++] = t;
			jbas_get_matching_paren(t, &t);
		}
	}

	// Sort all the operators
	qsort(operators, opcnt, sizeof(operators[0]), jbas_operator_token_compare);

	// Evaluate all operators
	for (int i = 0; i < opcnt; i++)
	{
		jbas_token *t = operators[i];

		if (t->type == JBAS_TOKEN_OPERATOR)
		{
			jbas_error err;
			const jbas_operator *op = t->operator_token.op;
			if (op->type == JBAS_OP_BINARY_LR || op->type == JBAS_OP_BINARY_RL)
				err = jbas_eval_binary_operator(env, t);
			else
				err = jbas_eval_unary_operator(env, t, op->type);
			if (err) return err;

		}
		else // Call operator (LPAREN)
		{
			jbas_error err = jbas_eval_paren(env, t);
			if (err) return err;
			err = jbas_eval_call_operator(env, t->l, t);
			if (err) return err;	
		}
	}

	if (result) *result = opcnt ? operators[opcnt - 1] : begin;
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
	fprintf(stderr, "Will evaluate: ");
	for (jbas_token *t = jbas_token_list_begin(expr); t; t = t->r)
		jbas_debug_dump_token(stderr, t);
	fprintf(stderr, "\n");



	jbas_error eval_err = jbas_eval(env, jbas_token_list_begin(expr), NULL, NULL);
	fprintf(stderr, "eval err: %d\n", eval_err);


	// DEBUG
	fprintf(stderr, "After eval: ");
	for (jbas_token *t = jbas_token_list_begin(expr); t; t = t->r)
		jbas_debug_dump_token(stderr, t);
	fprintf(stderr, "\n");

	// Delete what's left
	expr = jbas_token_list_begin(expr);
	while (expr)
		jbas_token_list_return_to_pool(&expr, &env->token_pool);

	return JBAS_OK;
}

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


	// If it's ';', it's a delimiter
	if (*s == ';' || *s == '\n')
	{
		*next = s + 1;
		token->type = JBAS_TOKEN_DELIMITER;
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

	// An operator
	const char *operator_end = s;
	while (jbas_is_operator_char(*operator_end)) operator_end++;
	*next = operator_end;

	for (int i = 0; i < JBAS_OPERATOR_COUNT; i++)
	{
		if (!jbas_namecmp(s, operator_end, jbas_operators[i].str, NULL))
		{
			token->type = JBAS_TOKEN_OPERATOR;
			token->operator_token.op = &jbas_operators[i];
			return JBAS_OK;
		}
	}

	// A symbol/keyword
	const char *name_end = s;
	while (jbas_is_name_char(*name_end)) name_end++;
	*next = name_end;
	
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

	// It must be a symbol token
	{
		token->type = JBAS_TOKEN_SYMBOL;	
		jbas_symbol *sym;
		jbas_error err;
		
		err = jbas_symbol_create(env, &sym, s, name_end);
		if (err == JBAS_OK || err == JBAS_SYMBOL_COLLISION)
			token->symbol_token.sym = sym;
		else
			return err;
		
		
	}


	// Todo failed token!!!!

	return JBAS_OK;
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
