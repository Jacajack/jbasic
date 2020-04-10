// #define JBASIC_DEBUG
#define JBASIC_IMPLEMENTATION
#include "jbasic.h"

int main(void)
{
	jbas_env env;
	jbas_env_init(&env, 10000, 1000, 1000, 1000);

	const char *lines = 
	"A = 7 + 5 + 10\n"
	"A = 1 + 7 + 2 * ( 3.0 / (1 + 1) )\n";
	/*
	"TEST_STR = 'this is a test string'; PRINT 'hehe'\n"
	"A = 34\n"
	"IF a > (b + 17) THEN\n"
	"PRINT 'rawr'\n"
	"ENDIF\n"
	"PRINT `fus ro dah!`\n";
	*/

	jbas_tokenize_string(&env, lines);

	// printf("---------\n");

	printf("\n\n\n");

	for (jbas_token *t = jbas_token_list_begin(env.tokens); t; t = t->r)
		jbas_debug_dump_token(stderr, t);

	printf("\n\n--------- Symbol dump:\n");

	// Symbol table dump
	for (int i = 0; i < env.symbol_manager.max_count; i++)
		if (env.symbol_manager.is_used[i])
			printf("\t- %s\n", env.symbol_manager.symbol_storage[i].name->str);

	// Run
	jbas_run_tokens(&env);

	jbas_env_destroy(&env);
}
