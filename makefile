all:
	clang -o test test.c -Wall -fsanitize=address