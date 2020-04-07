all:
	clang -o test test.c -Wall -O0 -fsanitize=address -g