#include <jbasic/debug.h>
#include <jbasic/jbasic.h>

#define JBAS_COLOR_RED "\x1b[31m"
#define JBAS_COLOR_GREEN "\x1b[32m"
#define JBAS_COLOR_YELLOW "\x1b[33m"
#define JBAS_COLOR_BLUE "\x1b[34m"
#define JBAS_COLOR_MAGENTA "\x1b[35m"
#define JBAS_COLOR_CYAN "\x1b[36m"
#define JBAS_COLOR_RESET "\x1b[0m"

const char *jbas_debug_keyword_str(jbas_keyword id)
{
	for (int i = 0; i < JBAS_KEYWORD_COUNT; i++)
		if (jbas_keywords[i].id == id)
			return jbas_keywords[i].name;
	return NULL;
}

/**
	Dumps token to stderr
*/
void jbas_debug_dump_token(FILE *f, const jbas_token *token)
{	
	fprintf(f, " ");

	if (!token)
	{
		fprintf(f, JBAS_COLOR_RED "<NULL>" JBAS_COLOR_RESET);
		return;
	}

	switch (token->type)
	{
		case JBAS_TOKEN_KEYWORD:
			fprintf(f, JBAS_COLOR_BLUE "%s" JBAS_COLOR_RESET, jbas_debug_keyword_str(token->keyword_token.id));
			break;

		case JBAS_TOKEN_STRING:
			fprintf(f, JBAS_COLOR_GREEN "'%s'" JBAS_COLOR_RESET, token->string_token.txt->str);
			break;

		case JBAS_TOKEN_NUMBER:
			if (token->number_token.type == JBAS_NUM_INT)
				fprintf(f, JBAS_COLOR_RED "%d" JBAS_COLOR_RESET, token->number_token.i);
			else if (token->number_token.type == JBAS_NUM_BOOL)
				fprintf(f, JBAS_COLOR_RED "%s" JBAS_COLOR_RESET, token->number_token.i ? "TRUE" : "FALSE");
			else
				fprintf(f, JBAS_COLOR_RED "%f" JBAS_COLOR_RESET, token->number_token.f);
			break;

		case JBAS_TOKEN_OPERATOR:
			fprintf(f, JBAS_COLOR_YELLOW "%s" JBAS_COLOR_RESET, token->operator_token.op->str);
			break;

		case JBAS_TOKEN_DELIMITER:
			fprintf(f, ";\n");
			break;

		case JBAS_TOKEN_LPAREN:
		case JBAS_TOKEN_RPAREN:
			fprintf(f, JBAS_COLOR_CYAN "%c" JBAS_COLOR_RESET, token->type == JBAS_TOKEN_LPAREN ? '(' : ')');
			break;

		case JBAS_TOKEN_SYMBOL:
			fprintf(f, JBAS_COLOR_RESET "%s" JBAS_COLOR_RESET, token->symbol_token.sym->name->str);
			break;

		default:
			fprintf(f, "(%d?)", token->type );
			break;
	}
}

void jbas_debug_dump_resource(FILE *f, jbas_resource *res)
{
	if (!res)
	{
		fprintf(f, "NULL");
		return;
	}

	if (res->type == JBAS_RESOURCE_NUMBER)
	{
		if (res->number.type == JBAS_NUM_INT || res->number.type == JBAS_NUM_BOOL)
			fprintf(f, "%d", res->number.i);
		
		return;
	}

	fprintf(f, "???");
}

void jbas_debug_dump_symbol(FILE *f, jbas_symbol *sym)
{
	fprintf(f, "`%s` = ", sym->name->str);
	jbas_debug_dump_resource(f, sym->resource);
}

void jbas_debug_dump_symbol_table(FILE *f, jbas_env *env)
{
	jbas_symbol_manager *sm = &env->symbol_manager;
	fprintf(f, JBAS_COLOR_MAGENTA "== SYMBOL TABLE DUMP BEGIN\n" JBAS_COLOR_RESET);
	for (int i = 0; i < sm->max_count; i++)
		if (sm->is_used[i])
		{
			jbas_debug_dump_symbol(f, &sm->symbol_storage[i]);
			fprintf(f, "\n");
		}
	fprintf(f, JBAS_COLOR_MAGENTA "== SYMBOL TABLE DUMP END\n" JBAS_COLOR_RESET);
}