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
			fprintf(f, JBAS_COLOR_BLUE "%s" JBAS_COLOR_RESET, jbas_debug_keyword_str(token->u.keyword_token.id));
			break;

		case JBAS_TOKEN_STRING:
			fprintf(f, JBAS_COLOR_GREEN "'%s'" JBAS_COLOR_RESET, token->u.string_token.txt->str);
			break;

		case JBAS_TOKEN_NUMBER:
			if (token->u.number_token.type == JBAS_NUM_INT)
				fprintf(f, JBAS_COLOR_RED "%d" JBAS_COLOR_RESET, token->u.number_token.i);
			else if (token->u.number_token.type == JBAS_NUM_BOOL)
				fprintf(f, JBAS_COLOR_RED "%s" JBAS_COLOR_RESET, token->u.number_token.i ? "TRUE" : "FALSE");
			else
				fprintf(f, JBAS_COLOR_RED "%f" JBAS_COLOR_RESET, token->u.number_token.f);
			break;

		case JBAS_TOKEN_OPERATOR:
			fprintf(f, JBAS_COLOR_YELLOW "%s" JBAS_COLOR_RESET, token->u.operator_token.op->str);
			break;

		case JBAS_TOKEN_DELIMITER:
			fprintf(f, ";\n");
			break;

		case JBAS_TOKEN_LPAREN:
		case JBAS_TOKEN_RPAREN:
			fprintf(f, JBAS_COLOR_CYAN "%c" JBAS_COLOR_RESET, token->type == JBAS_TOKEN_LPAREN ? '(' : ')');
			break;

		case JBAS_TOKEN_SYMBOL:
			fprintf(f, JBAS_COLOR_RESET "%s" JBAS_COLOR_RESET, token->u.symbol_token.sym->name->str);
			break;

		default:
			fprintf(f, "(%d?)", token->type );
			break;
	}
}

void jbas_debug_dump_resource(FILE *f, jbas_resource *res)
{
}

void jbas_debug_dump_symbol(FILE *f, jbas_symbol *sym)
{
	fprintf(f, "`%s` ", sym->name->str);
	jbas_debug_dump_resource(f, sym->resource);
}
