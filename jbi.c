#include <jbasic/jbasic.h>
#include <jbasic/debug.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


// This is POSIX only <3
#include <dlfcn.h>
void *dl_load(jbas_env *env, int debug)
{
	// Get library name
	const char *libname = getenv("JBASLIB");

	// Load symbols
	void *handle = NULL;
	if (libname)
	{
		handle = dlopen(libname, RTLD_NOW);
		if (!handle)
		{
			perror("could not load dynamic library");
			jbas_env_destroy(env);
			exit(EXIT_FAILURE);
		}

		// Load symbol list
		jbas_cres *cres = dlsym(handle, "jbas_symbols");
		if (!cres)
		{
			fprintf(stderr,"no symbols to load :P\n");
			return handle;
		}

		// Load symbol count
		int *crescnt = dlsym(handle, "jbas_symbol_count");
		if (!crescnt)
		{
			fprintf(stderr,"symbol count not specified...\n");
			return handle;
		}

		// Import all resources
		for (int i = 0; i < *crescnt; i++)
		{
			if (debug) fprintf(stderr, "importing %s...\n", cres[i].name);

			// Create/get symbol
			jbas_symbol *sym = NULL;
			jbas_error err = jbas_symbol_create(env, &sym, cres[i].name, NULL);
			if (err == JBAS_OK || err == JBAS_SYMBOL_COLLISION)
			{
				// Create jbas resource
				jbas_error err = jbas_resource_create(&env->resource_manager, &sym->res);
				if (err)
				{
					fprintf(stderr, "failed to create resource during symbol import...\n");
					exit(EXIT_FAILURE);
				}

				sym->res->type = JBAS_RESOURCE_CFUN;
				sym->res->cfun = cres[i].cfun;

			}
			else
			{
				fprintf(stderr, "failed to create symbol during symbol import...\n");
				exit(EXIT_FAILURE);
			}

		}

	}

	return handle;
}


int main(int argc, char *argv[])
{
	// Look for debug switch
	int debug = argc > 2 && !strcmp(argv[2], "-debug"); 

	// Help message
	if (argc < 2)
	{
		fprintf(stderr, "Usage: %s FILENAME [-debug]\n", argv[0]);
		exit(EXIT_FAILURE);
	}

	jbas_env env;
	jbas_env_init(&env, 10000, 1000, 1000, 1000);

	// Import C resources
	void *handle = dl_load(&env, debug);

	// Open file
	FILE *f = fopen(argv[1], "rt");
	if (!f)
	{
		perror("could not open input file!");
		exit(EXIT_FAILURE);
	}

	// Read
	size_t len = 10000;
	char *line = malloc(len);
	while (getline(&line, &len, f) > 0)
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

	// Close file
	fclose(f);

	// Debug dump
	if (debug)
	{
		jbas_debug_dump_token_list(stderr, env.tokens);
		printf("\n\n\n");
	}

	// Run
	jbas_error err = jbas_run(&env);

	// Symbol table dump
	if (debug)
	{
		printf("\n\n\n");
		jbas_debug_dump_symbol_table(stderr, &env);
	}

	// Run error?
	if (err)
	{
		fprintf(stderr, "run error %d: %s\n", err, env.error_reason);
		jbas_env_destroy(&env);
		exit(EXIT_FAILURE);
	}

	if (handle) dlclose(handle);
	jbas_env_destroy(&env);
	return EXIT_SUCCESS;
}
