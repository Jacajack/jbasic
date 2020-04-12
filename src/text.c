#include <jbasic/text.h>
#include <stdlib.h>
#include <string.h>

jbas_error jbas_text_manager_init(jbas_text_manager *tm, int text_count)
{
	tm->max_count = text_count;
	tm->free_slot_count = text_count;

	// Allocate memory
	tm->text_storage = calloc(text_count, sizeof(jbas_text));
	tm->free_slots = calloc(text_count, sizeof(int));
	tm->is_used = calloc(text_count, sizeof(bool));

	// Handle calloc errors
	if (!tm->text_storage || !tm->free_slots || !tm->is_used)
	{
		free(tm->text_storage);
		free(tm->free_slots);
		free(tm->is_used);
		return JBAS_ALLOC;
	}

	for (int i = 0; i < tm->free_slot_count; i++)
		tm->free_slots[i] = i;

	return JBAS_OK;
}

jbas_error jbas_text_create(jbas_text_manager *tm, const char *s, const char *end, jbas_text **txt)
{
	if (!tm->free_slot_count) return JBAS_TEXT_MANAGER_OVERFLOW;

	int slot = tm->free_slots[--tm->free_slot_count];
	jbas_text *t = tm->text_storage + slot;
	tm->is_used[slot] = true;

	// Copy provided string
	if (end)
	{
		t->str = calloc(end - s + 1, sizeof(char));
		t->length = end - s;
		memcpy(t->str, s, t->length);
	}
	else
	{
		t->str = strdup(s);
		t->length = strlen(s);
	}


	// Return a pointer to the new text
	*txt = t;

	return JBAS_OK;
}

jbas_error jbas_text_lookup(jbas_text_manager *tm, const char *s, const char *end, jbas_text **txt)
{
	for (int i = 0; i < tm->max_count; i++)
	{
		if (!tm->is_used[i]) continue;
		if (end)
		{
			if (end - s == tm->text_storage[i].length && !strncmp(s, tm->text_storage[i].str, end - s))
			{
				*txt = &tm->text_storage[i];
				return JBAS_OK;
			}
		}
		else
		{
			if (!strncmp(s, tm->text_storage[i].str, s - end))
			{
				*txt = &tm->text_storage[i];
				return JBAS_OK;
			}
		}
	}

	*txt = NULL;

	return JBAS_OK;
}

jbas_error jbas_text_lookup_create(jbas_text_manager *tm, const char *s, const char *end, jbas_text **txt)
{
	jbas_text *t;
	jbas_error err = jbas_text_lookup(tm, s, end, &t);
	if (err) return err;
	if (!t) err = jbas_text_create(tm, s, end, &t);
	*txt = t;
	return err;
}

jbas_error jbas_text_destroy(jbas_text_manager *tm, jbas_text *txt)
{
	if (!txt) return JBAS_OK;

	int slot = txt - tm->text_storage;

	// Check if the text is managed by this text manager
	if (slot > tm->max_count) return JBAS_TEXT_MANAGER_MISMATCH;

	// Actually delete the stored text
	free(txt->str);
	txt->str = NULL;
	tm->is_used[slot] = false;

	// Free up the slot
	tm->free_slots[tm->free_slot_count++] = txt - tm->text_storage;
	
	return JBAS_OK;
}


void jbas_text_manager_destroy(jbas_text_manager *tm)
{
	// Destroy all the stored texts first
	for (int i = 0; i < tm->max_count; i++)
		if (tm->is_used[i])
			jbas_text_destroy(tm, &tm->text_storage[i]);

	free(tm->text_storage);
	free(tm->free_slots);
	free(tm->is_used);
}







