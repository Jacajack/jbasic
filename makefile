SRC = test.c src/jbasic.c src/resource.c src/op.c src/token.c src/symbol.c src/text.c src/debug.c

all:
	clang -o test -Iinclude $(SRC) -Wall -lm -O0 -fno-omit-frame-pointer -fsanitize=address -fsanitize=undefined -g -ferror-limit=2