#include <jbasic/token.h>
#include <jbasic/resource.h>
#include <stdlib.h>

/**
	Moves token data - the source token is invalidated
*/
jbas_error jbas_token_move(jbas_token *dest, jbas_token *src, jbas_token_pool *pool)
{
	jbas_token *l = dest->l, *r = dest->r;
	jbas_token tmp;

	tmp = *src;

	// Invalidate source tuple
	if (src->type == JBAS_TOKEN_TUPLE)
	{
		src->tuple_token.tokens = NULL;
	}

	// Invalidate source prentheses
	if (src->type == JBAS_TOKEN_PAREN)
	{
		src->paren_token.tokens = NULL;
	}

	// Invalidate source's reference to the resource
	if (src->type == JBAS_TOKEN_RESOURCE)
	{
		src->resource_token.res = NULL;
	}

	// Empty destination token (the source token can be contained in dest token!!!)
	jbas_error err = jbas_empty_token(dest, pool);
	
	*dest = tmp;
	dest->l = l;
	dest->r = r;

	return err;
}

/**	
	Copies token data - perfroms deep copy if necessary
	For tuples it acts like moving.
	Updates parentheses refs too.
*/
jbas_error jbas_token_copy(jbas_token *dest, jbas_token *src, jbas_token_pool *pool)
{
	jbas_token *l = dest->l, *r = dest->r;
	jbas_error err;
	jbas_token tmp;

	tmp = *src;

	// Copy source tuple - recursively
	if (src->type == JBAS_TOKEN_TUPLE)
	{
		tmp.tuple_token.tokens = NULL;
		for (jbas_token *t = jbas_token_list_begin(src->tuple_token.tokens); t; t = t->r)
		{
			jbas_token u = {.type = JBAS_TOKEN_DELIMITER};
			err = jbas_token_copy(&u, t, pool);
			if (err) return err;
			err = jbas_token_list_push_back_from_pool(tmp.tuple_token.tokens, pool, &u, &tmp.tuple_token.tokens);
			if (err) return err;
		}
	}

	// Copy source parentheses contents - recursively
	if (src->type == JBAS_TOKEN_PAREN)
	{
		tmp.paren_token.tokens = NULL;
		for (jbas_token *t = jbas_token_list_begin(src->paren_token.tokens); t; t = t->r)
		{
			jbas_token u = {.type = JBAS_TOKEN_DELIMITER};
			err = jbas_token_copy(&u, t, pool);
			if (err) return err;
			err = jbas_token_list_push_back_from_pool(tmp.paren_token.tokens, pool, &u, &tmp.paren_token.tokens);
			if (err) return err;
		}
	}

	// Increase ref count when copying a resource token
	if (src->type == JBAS_TOKEN_RESOURCE)
	{
		jbas_resource_add_ref(src->resource_token.res);
	}

	// Empty destination token (the source token can be contained in dest token!!!)
	err = jbas_empty_token(dest, pool);
	if (err) return err;
	
	*dest = tmp;
	dest->l = l;
	dest->r = r;

	
	return JBAS_OK;
}

/**	
	Swaps two tokens by doing pointer magic
	This may actually be less efficient than copying...
*/
jbas_error jbas_token_swap(jbas_token *a, jbas_token *b, jbas_token_pool *pool)
{
	jbas_token tmp = {.type = JBAS_TOKEN_DELIMITER};
	jbas_error err = jbas_token_move(&tmp, a, pool);
	if (err) return err;
	err = jbas_token_move(a, b, pool);
	if (err) return err;
	err = jbas_token_move(b, &tmp, pool);
	if (err) return err;
	return JBAS_OK;
}

// -------------------------------------- TOKEN POOL

jbas_error jbas_token_pool_get(jbas_token_pool *pool, jbas_token **t)
{
	if (!pool->unused_count) return JBAS_TOKEN_POOL_EMPTY;
	*t = pool->unused_stack[--pool->unused_count];
	// fprintf(stderr, "\ngot %p from pool\n", *t);
	return JBAS_OK;
}

jbas_error jbas_token_pool_return(jbas_token_pool *pool, jbas_token *t)
{
	if (pool->unused_count >= pool->pool_size) return JBAS_TOKEN_POOL_OVERFLOW;
	pool->unused_stack[pool->unused_count++] = t;
	// fprintf(stderr, "\nreturned %p to pool\n", t);
	return JBAS_OK;
}


jbas_error jbas_token_pool_init(jbas_token_pool *pool, int size)
{
	pool->pool_size = pool->unused_count = size;
	pool->tokens = calloc(size, sizeof(jbas_token));
	pool->unused_stack = calloc(size, sizeof(jbas_token*));
	
	if (!pool->tokens || !pool->unused_stack)
	{
		free(pool->tokens);
		free(pool->unused_stack);
		return JBAS_ALLOC;
	}

	for (int i = 0; i < size; i++)
	{
		pool->unused_stack[i] = &pool->tokens[i];
		pool->tokens[i].l = pool->tokens[i].r = NULL;
	}

	return JBAS_OK;
}

jbas_error jbas_token_pool_destroy(jbas_token_pool *pool)
{
	free(pool->tokens);
	free(pool->unused_stack);
	return JBAS_ALLOC;
}

// --------------------------------------


/**
	Returns a pointer to the first element of the list
*/
jbas_token *jbas_token_list_begin(jbas_token *t)
{
	while (t && t->l)
		t = t->l;
	return t;
}

/**
	Returns a pointer to the last element of the list
*/
jbas_token *jbas_token_list_end(jbas_token *t)
{
	while (t && t->r)
		t = t->r;
	return t;
}

/**
	Insert a new element into a token list. The new token is inserted after
	the element pointed by `after` pointer.

	Pointer to the newly inserted element is optionally returned through the `inserted` pointer.
*/
jbas_error jbas_token_list_insert_from_pool(
	jbas_token *after,
	jbas_token_pool *pool,
	jbas_token *token,
	jbas_token **inserted)
{
	jbas_token *t;
	jbas_error err = jbas_token_pool_get(pool, &t);
	if (err) return err;
	
	*t = *token;

	// Insert into empty list?
	if (after == NULL)
	{
		t->l = NULL;
		t->r = NULL;
		if (inserted) *inserted = t;
		return JBAS_OK;
	}
	else
	{
		t->l = after;
		t->r = after->r;
		if (after->r) after->r->l = t;
		after->r = t;

		if (inserted) *inserted = t;
		return JBAS_OK;
	}
}

jbas_error jbas_token_list_insert_before_from_pool(
		jbas_token *before,
		jbas_token_pool *pool,
		jbas_token *token,
		jbas_token **inserted)
{
	// Insert at the beginning?
	if (!before->l)
	{
		jbas_token *t;
		jbas_error err = jbas_token_pool_get(pool, &t);
		if (err) return err;
		
		*t = *token;

		t->l = NULL;
		t->r = before;
		before->l = t;

		if (inserted) *inserted = t;
		return JBAS_OK;
	}
	else
	{
		return jbas_token_list_insert_from_pool(before->l, pool, token, inserted);
	}
}


jbas_error jbas_token_list_push_front_from_pool(
	jbas_token *list,
	jbas_token_pool *pool,
	jbas_token *token,
	jbas_token **inserted)
{
	return jbas_token_list_insert_before_from_pool(jbas_token_list_begin(list), pool, token, inserted);
}

jbas_error jbas_token_list_push_back_from_pool(
	jbas_token *list,
	jbas_token_pool *pool,
	jbas_token *token,
	jbas_token **inserted)
{
	return jbas_token_list_insert_from_pool(jbas_token_list_end(list), pool, token, inserted);
}

/**
	Removes a node from the linked list and moves it to the pool

	The handle is moved either to the left or to the right.
	If removed element is the last one in the list, the handle is set to NULL.
*/
jbas_error jbas_token_list_return_handle_to_pool(jbas_token **list_handle, jbas_token_pool *pool)
{
	jbas_token *t = *list_handle;

	// Move the handle to a valid element
	if (t->l) *list_handle = t->l;
	else if (t->r) *list_handle = t->r;
	else *list_handle = NULL;

	return jbas_token_list_return_to_pool(t, pool);
}

/**
	Empties a container token (parentheses, tuple) out
*/
jbas_error jbas_empty_token(jbas_token *t, jbas_token_pool *pool)
{
	// Tuple tokens contain sub-lits
	if (t->type == JBAS_TOKEN_TUPLE && t->tuple_token.tokens)
	{
		jbas_error err = jbas_token_list_destroy(t->tuple_token.tokens, pool);
		t->tuple_token.tokens = NULL;
		if (err) return err;
	}

	// Parentheses tokens contain sub-lits
	if (t->type == JBAS_TOKEN_PAREN && t->paren_token.tokens)
	{
		jbas_error err = jbas_token_list_destroy(t->paren_token.tokens, pool);
		t->paren_token.tokens = NULL;
		if (err) return err;
	}

	// Resources have reference counting
	if (t->type == JBAS_TOKEN_RESOURCE)
	{
		jbas_resource_remove_ref(t->resource_token.res);
	}

	return JBAS_OK;
}

/**
	Removes a node from the linked list and moves it to the pool
	\warning The provided pointer is invalidated
*/
jbas_error jbas_token_list_return_to_pool(jbas_token *t, jbas_token_pool *pool)
{
	jbas_error err = jbas_empty_token(t, pool);
	if (err) return err;

	if (t->l) t->l->r = t->r;
	if (t->r) t->r->l = t->l;

	return jbas_token_pool_return(pool, t);
}

/**
	Returns entire token list to pool
*/
jbas_error jbas_token_list_destroy(jbas_token *list, jbas_token_pool *pool)
{
	jbas_token *t = jbas_token_list_begin(list);
	while (t)
	{
		jbas_error err = jbas_token_list_return_handle_to_pool(&t, pool);
		if (err) return err;
	}
	
	return JBAS_OK;
}