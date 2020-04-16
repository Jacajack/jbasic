#include <jbasic/defs.h>
#include <jbasic/jbasic.h>
#include <jbasic/cast.h>
#include <stdio.h>

jbas_error stdjbas_hw(jbas_env *env, jbas_token *args, jbas_token *res)
{
	puts("Hello world! (from stdjbas)");
	res->type = JBAS_TOKEN_NUMBER;
	res->number_token.type = JBAS_NUM_INT;
	res->number_token.i = 0;
	return JBAS_OK;
}

jbas_error stdjbas_getchar(jbas_env *env, jbas_token *args, jbas_token *res)
{
	res->type = JBAS_TOKEN_NUMBER;
	res->number_token.type = JBAS_NUM_INT;
	res->number_token.i = getchar();
	return JBAS_OK;
}

jbas_error stdjbas_putchar(jbas_env *env, jbas_token *args, jbas_token *res)
{
	jbas_error err = jbas_token_to_number_type(env, args, JBAS_NUM_INT);
	if (err)
	{
		JBAS_ERROR_REASON(env, "PUTCHAR bad argument!");
		return err;
	}

	int c = args->number_token.i;
	putchar(c);

	res->type = JBAS_TOKEN_NUMBER;
	res->number_token.type = JBAS_NUM_INT;
	res->number_token.i = c;
	return JBAS_OK;
}


jbas_cres jbas_symbols[] = {
	{.name = "HWTEST", .cfun = stdjbas_hw},
	{.name = "GETCHAR", .cfun = stdjbas_getchar},
	{.name = "PUTCHAR", .cfun = stdjbas_putchar},
};
int jbas_symbol_count = (sizeof(jbas_symbols) / sizeof(jbas_symbols[0]));