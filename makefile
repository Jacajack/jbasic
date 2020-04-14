SRC = test.c src/jbasic.c src/resource.c src/op.c src/token.c src/symbol.c src/text.c src/debug.c src/paren.c src/cast.c src/kw.c

CFLAGS = -Iinclude -DJBAS_ERROR_REASONS -Wall -lm 

ifneq ($(DEBUG),)
CFLAGS += -O0 -DJBAS_DEBUG -fno-omit-frame-pointer -fsanitize=address -fsanitize=undefined -g -ferror-limit=2
else
CFLAGS += -O3 -flto -ffast-math
endif

all:
	clang -o jbasic $(CFLAGS) $(SRC) 