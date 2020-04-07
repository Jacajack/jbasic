#define JBASIC_DEBUG
#define JBASIC_IMPLEMENTATION
#include "jbasic.h"

int main(void)
{
	jbas_env env;
	jbas_env_init(&env, 10000);

	const char *lines = 
	"'aa' TEST_STR = 'this is a test string'\n"
	"PRINT 'hehe'\n"
	"PRINT `This is nice!`\n";

	jbas_tokenize_string(&env, lines);

	printf("---------\n");

	for (jbas_token *t = jbas_token_list_begin(env.tokens); t; t = t->r)
		jbas_debug_dump_token(t);

	jbas_env_destroy(&env);
}
