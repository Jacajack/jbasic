#define JBASIC_DEBUG
#define JBASIC_IMPLEMENTATION
#include "jbasic.h"

int main(void)
{
	jbas_env jb =
	{
		.token_stack_size = 12
	};

	const char *lines = 
	"TEST_STR = 'this is a test string'\n"
	"PRINT 'hehe'\n"
	"PRINT `This is nice!`\n";

	jbas_run_lines(&jb, lines);
}
