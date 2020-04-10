all:
	clang -o test test.c -Wall -O0 -fno-omit-frame-pointer -fsanitize=address -fsanitize=undefined -g