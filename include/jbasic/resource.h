#ifndef JBASIC_RESMGR_H
#define JBASIC_RESMGR_H

#include <jbasic/defs.h>
#include <jbasic/token.h>

/*
	Resources are created dynamically during the program execution.
	Reference counting system has been implemented in order to make
	garbare collection possible.
*/

typedef enum jbas_resource_type
{
	JBAS_RESOURCE_NUMBER,
	JBAS_RESOURCE_INT_ARRAY,
	JBAS_RESOURCE_FLOAT_ARRAY,
	JBAS_RESOURCE_INT_PTR,
	JBAS_RESOURCE_FLOAT_PTR,
	JBAS_RESOURCE_STRING,
	JBAS_RESOURCE_CFUN,
} jbas_resource_type;

typedef struct jbas_token jbas_token;

typedef struct jbas_resource
{
	int rm_index;

	jbas_resource_type type;
	int ref_count;

	size_t size;
	
	union
	{
		jbas_number_token number;
		jbas_error (*cfun)(jbas_env *env, jbas_token *arg, jbas_token *res);
		int *iptr;
		float *fptr;
		char *str;
		void *data;
	};
	
} jbas_resource;

typedef struct jbas_resource_manager
{
	jbas_resource **refs;
	int ref_count;
	int max_count;
} jbas_resource_manager;


jbas_error jbas_resource_manager_init(jbas_resource_manager *rm, int max_count);
jbas_error jbas_resource_manager_garbage_collect(jbas_resource_manager *rm, int *collected);
void jbas_resource_delete(jbas_resource_manager *rm, jbas_resource *res);
jbas_error jbas_resource_remove_ref(jbas_resource *res);
jbas_error jbas_resource_add_ref(jbas_resource *res);
jbas_error jbas_resource_create(jbas_resource_manager *rm, jbas_resource **res);
void jbas_resource_copy(jbas_resource *dest, jbas_resource *src);
void jbas_resource_manager_destroy(jbas_resource_manager *rm);

#endif