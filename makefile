all:
	clang -o test test.c -Wall -lm -O0 -fno-omit-frame-pointer -fsanitize=address -fsanitize=undefined -g