#include <jbasic/debug.h>
#include <jbasic/jbasic.h>

#define JBAS_COLOR_RED "\x1b[31m"
#define JBAS_COLOR_GREEN "\x1b[32m"
#define JBAS_COLOR_YELLOW "\x1b[33m"
#define JBAS_COLOR_BLUE "\x1b[34m"
#define JBAS_COLOR_MAGENTA "\x1b[35m"
#define JBAS_COLOR_CYAN "\x1b[36m"
#define JBAS_COLOR_RESET "\x1b[0m"

/**
	Dumps token to stderr
*/
void jbas_debug_dump_token(FILE *f, jbas_token *token)
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
			fprintf(f, JBAS_COLOR_BLUE "%s" JBAS_COLOR_RESET, token->keyword_token.kw->str);
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

		case JBAS_TOKEN_PAREN:
			fprintf(f, JBAS_COLOR_CYAN "(" JBAS_COLOR_RESET);
			jbas_debug_dump_token_list(f, token->paren_token.tokens);
			fprintf(f, JBAS_COLOR_CYAN " )" JBAS_COLOR_RESET);
			break;

		case JBAS_TOKEN_SYMBOL:
			fprintf(f, JBAS_COLOR_RESET "%s" JBAS_COLOR_RESET, token->symbol_token.sym->name->str);
			break;

		case JBAS_TOKEN_TUPLE:
			fprintf(f, JBAS_COLOR_MAGENTA "{" JBAS_COLOR_RESET);
			jbas_debug_dump_token_list(f, token->tuple_token.tokens);
			fprintf(f, JBAS_COLOR_MAGENTA " }" JBAS_COLOR_RESET);
			break;

		default:
			fprintf(f, "(%d?)", token->type );
			break;
	}
}

void jbas_debug_dump_token_list_begin_end(FILE *f, jbas_token *begin, jbas_token *end)
{
	for (; begin && begin != end; begin = begin->r)
		jbas_debug_dump_token(f, begin);
}


void jbas_debug_dump_token_list(FILE *f, jbas_token *t)
{
	jbas_debug_dump_token_list_begin_end(f, jbas_token_list_begin(t), NULL);
}

void jbas_debug_dump_resource(FILE *f, jbas_resource *res)
{
	if (!res)
	{
		fprintf(f, "NULL");
		return;
	}

	// Printing numbers
	if (res->type == JBAS_RESOURCE_NUMBER)
	{
		const jbas_number_token *n = &res->number;
		switch (n->type)
		{
			case JBAS_NUM_INT:
				fprintf(f, "%d", n->i);
				break;

			case JBAS_NUM_BOOL:
				fprintf(f, n->i ? "TRUE" : "FALSE");
				break;

			case JBAS_NUM_FLOAT:
				fprintf(f, "%f", n->f);
				break;

		}
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