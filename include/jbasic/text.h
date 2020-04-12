#ifndef JBASIC_TEXT_H
#define JBASIC_TEXT_H

#include <jbasic/defs.h>
#include <stddef.h>
#include <stdbool.h>

typedef struct
{
	char *str;
	size_t length;
} jbas_text;

typedef struct
{
	jbas_text *text_storage;
	bool *is_used;
	int *free_slots;
	int free_slot_count;
	int max_count;
} jbas_text_manager;

jbas_error jbas_text_manager_init(jbas_text_manager *tm, int text_count);
jbas_error jbas_text_create(jbas_text_manager *tm, const char *s, const char *end, jbas_text **txt);
jbas_error jbas_text_lookup(jbas_text_manager *tm, const char *s, const char *end, jbas_text **txt);
jbas_error jbas_text_lookup_create(jbas_text_manager *tm, const char *s, const char *end, jbas_text **txt);
jbas_error jbas_text_destroy(jbas_text_manager *tm, jbas_text *txt);
void jbas_text_manager_destroy(jbas_text_manager *tm);

#endif