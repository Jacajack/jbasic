#include <jbasic/resource.h>
#include <stdlib.h>

jbas_error jbas_resource_manager_init(jbas_resource_manager *rm, int max_count)
{
	rm->max_count = max_count;
	rm->refs = calloc(max_count, sizeof(jbas_resource*));
	rm->ref_count = 0;

	if (!rm->refs)
		return JBAS_ALLOC;

	return JBAS_OK;
}

/**
	Frees all resources and the resource manager memory
*/
void jbas_resource_manager_destroy(jbas_resource_manager *rm)
{
	while (rm->ref_count)
		jbas_resource_delete(rm, rm->refs[0]);
	free(rm->refs);
}

/**
	Deletes resources that have no references
*/
jbas_error jbas_resource_manager_garbage_collect(jbas_resource_manager *rm)
{
	for (int i = 0; i < rm->ref_count; i++)
	{
		if (rm->refs[i]->ref_count == 0)
			jbas_resource_delete(rm, rm->refs[i]);
	}

	return JBAS_OK;
}

/**
	Removes a reference to the resource
*/
jbas_error jbas_resource_remove_ref(jbas_resource *res)
{
	if (res->ref_count) res->ref_count--;
	return JBAS_OK;
}

/**
	Increments reference counter
*/
jbas_error jbas_resource_add_ref(jbas_resource *res)
{
	res->ref_count++;
	return JBAS_OK;
}

/**
	Create a resource and register it in the resource_manager
*/
jbas_error jbas_resource_create(jbas_resource_manager *rm, jbas_resource **res)
{
	jbas_resource *r = calloc(1, sizeof(jbas_resource));
	if (!r) return JBAS_ALLOC;
	
	r->ref_count = 1;

	// Register in the resource manager
	int index = rm->ref_count++;
	if (index >= rm->max_count) return JBAS_RESOURCE_MANAGER_OVERFLOW; // TODO try gc!
	r->rm_index = index;
	rm->refs[index] = r;


	*res = r;
	return JBAS_OK;
}

/**
	Frees memory used by the resource and removes it from the resource manager
*/
void jbas_resource_delete(jbas_resource_manager *rm, jbas_resource *res)
{
	switch (res->type)
	{
		case JBAS_RESOURCE_NUMBER:
		default:
			break;
	}

	// Update resource manager refs
	int index = --rm->ref_count;
	if (index < 0) return;
	jbas_resource *m = rm->refs[index];
	rm->refs[res->rm_index] = m;
	m->rm_index = res->rm_index;

	free(res);
}

/**
	Copies all resource data apart from the reference counter
*/
void jbas_resource_copy(jbas_resource *dest, jbas_resource *src)
{
	int ref_count = dest->ref_count;
	*dest = *src;
	dest->ref_count = ref_count;
}


