#include <jbasic/token.h>
#include <stdlib.h>

/**	
	Copies token data.
	For tuples it acts like moving.
	Updates parentheses refs too.
*/
void jbas_token_copy(jbas_token *dest, jbas_token *src)
{
	jbas_token *l = dest->l, *r = dest->r;
	*dest = *src;
	dest->l = l;
	dest->r = r;

	// Invalidate source tuple
	if (src->type == JBAS_TOKEN_TUPLE)
	{
		src->tuple_token.tokens = NULL;
	}

	// Update parentheses refs
	if (src->type == JBAS_TOKEN_LPAREN || src->type == JBAS_TOKEN_RPAREN)
	{
		jbas_token *match = dest->paren_token.match;
		if (match) match->paren_token.match = dest;
		src->paren_token.match = NULL;
	}
}

/**	
	Swaps two tokens by doing pointer magic
	This may actually be less efficient than copying...
*/
void jbas_token_swap(jbas_token *a, jbas_token *b)
{
	jbas_token tmp;
	jbas_token_copy(&tmp, a);
	jbas_token_copy(a, b);
	jbas_token_copy(b, &tmp);

	/*
	// A's and B's left pointers (avoid loops)
	tmp = a->l;
	a->l = b->l == a ? b : b->l;
	b->l = tmp == b ? a : tmp;

	// A's and B's right pointers (avoid loops)
	tmp = a->r;
	a->r = b->r == a ? b : b->r;
	b->r = tmp == b ? a : tmp;

	// External pointers
	if (a->l) a->l->r = a;
	if (a->r) a->r->l = a;
	if (b->l) b->l->r = b;
	if (b->r) b->r->l = b;
	*/
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
	Removes a node from the linked list and moves it to the pool
	\warning The provided pointer is invalidated
*/
jbas_error jbas_token_list_return_to_pool(jbas_token *t, jbas_token_pool *pool)
{
	// Tuple tokens contain sub-lits
	if (t->type == JBAS_TOKEN_TUPLE && t->tuple_token.tokens)
	{
		jbas_error err = jbas_token_list_destroy(t->tuple_token.tokens, pool);
		if (err) return err;
	}

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