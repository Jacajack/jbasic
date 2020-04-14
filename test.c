#include <jbasic/jbasic.h>
#include <jbasic/debug.h>
#include <stdio.h>
#include <stdlib.h>

int main(void)
{
	jbas_env env;
	jbas_env_init(&env, 10000, 1000, 1000, 1000);

	// Read
	char *line = NULL;
	size_t len = 0;
	while (getline(&line, &len, stdin) > 0)
	{
		jbas_error err = jbas_tokenize_string(&env, line);
		if (err)
		{
			fprintf(stderr, "tokenize error %d: %s\n", err, env.error_reason);
			jbas_env_destroy(&env);
			exit(EXIT_FAILURE);
		}
	}
	free(line);

	// Debug dump
	jbas_debug_dump_token_list(stderr, env.tokens);
	printf("\n\n\n");

	// Run
	jbas_error err = jbas_run(&env);

	// Symbol table dump
	printf("\n\n\n");
	jbas_debug_dump_symbol_table(stderr, &env);

	// Run error?
	if (err)
	{
		fprintf(stderr, "run error %d: %s\n", err, env.error_reason);
		jbas_env_destroy(&env);
		exit(EXIT_FAILURE);
	}

	jbas_env_destroy(&env);
	return EXIT_SUCCESS;
}
