// #define JBASIC_DEBUG
#define JBASIC_IMPLEMENTATION
#include <jbasic/jbasic.h>
#include <jbasic/debug.h>

int main(void)
{
	jbas_env env;
	jbas_env_init(&env, 10000, 1000, 1000, 1000);

	const char *lines = 
	// "A = 7 + 5 + 10\n"
	// "A = 3 *  7 + 2 * -( 3.0 + (1 / - 1) )\n"
	// "M = 3 * 7 = 1 - 2\n"
	//"TEST = 1 OR ( 7 * 1 + 1 )\n"
	// "DUPA = 77\n"
	//"14 + foo(7 + 1, 2 * 3) \n"
	"A = 7\n"
	"ORNONTOWICE = 0\n"
	"B = -------(A*2)\n"
	// "B = 0 OR (A = 1)\n"
	// "B = 1 OR (C = 1)\n"
	// "B = 1 - - 7 * ( A =  - - - - 2 + - 3 )\n"
	// "( 1 + 4 , 7, 3 ) , 33 * 6 , ( 2 , ( 4 , 5 / 1.1 ) ) \n"
	// "C = 1 && 0.14\n"
	//"Y = 2 + 3 * ( 7.0 / 5 + 4 ) + 3 * ( 21.7 * 55.23 - 3.127 / 12 )\n"
	;
	/*
	"TEST_STR = 'this is a test string'; PRINT 'hehe'\n"
	"A = 34\n"
	"IF a > (b + 17) THEN\n"
	"PRINT 'rawr'\n"
	"ENDIF\n"
	"PRINT `fus ro dah!`\n";
	*/

	jbas_error err;
	err = jbas_tokenize_string(&env, lines);
	fprintf(stderr, "tokenize error %d: %s\n", err, env.error_reason);

	// printf("---------\n");

	printf("\n\n\n");

	jbas_debug_dump_token_list(stderr, env.tokens);

	printf("\n\n\n");

	// Symbol table dump
	//for (int i = 0; i < env.symbol_manager.max_count; i++)
	//	if (env.symbol_manager.is_used[i])
	//		printf("\t- %s\n", env.symbol_manager.symbol_storage[i].name->str);


	// Run
	err = jbas_run_tokens(&env);

	printf("\n\n\n");
	fprintf(stderr, "run error %d: %s\n", err, env.error_reason);



	jbas_debug_dump_symbol_table(stderr, &env);

	jbas_env_destroy(&env);
}
