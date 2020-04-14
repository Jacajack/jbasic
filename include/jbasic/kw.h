#ifndef JBAS_KW_H
#define JBAS_KW_H

#include <jbasic/defs.h>
#include <jbasic/token.h>

typedef enum jbas_keyword_id
{
	JBAS_KW_NOP,
	JBAS_KW_END,

	JBAS_KW_IF,
	JBAS_KW_ELSE,
	JBAS_KW_THEN = JBAS_KW_NOP,
	JBAS_KW_ENDIF = JBAS_KW_END,

	JBAS_KW_WHILE,
	JBAS_KW_DO = JBAS_KW_NOP,
	JBAS_KW_ENDWHILE = JBAS_KW_END,

	// JBAS_KW_FOR,
	// JBAS_KW_
	// JBAS_KW_ENDFOR = JBAS_KW_END,

	JBAS_KW_PRINT,
} jbas_keyword_id;

typedef struct jbas_keyword
{
	int level_change;
	const char *str;
	jbas_keyword_id id;
	jbas_error (*handler)(jbas_env *env, jbas_token *begin, jbas_token **next);
	struct jbas_keyword *alias;
} jbas_keyword;

extern const jbas_keyword jbas_keywords[];

#define JBAS_KEYWORD_COUNT 6

const jbas_keyword *jbas_get_keyword_by_str(const char *b, const char *e);

int jbas_block_level_diff(const jbas_token *t);
jbas_error jbas_get_block_end(jbas_env *env, jbas_token *begin, jbas_token **match);


jbas_error jbas_eval_keyword(jbas_env *env, jbas_token *token, jbas_token **next);

#endif