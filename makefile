SRC = jbi.c src/jbasic.c src/resource.c src/op.c src/token.c src/symbol.c src/text.c src/debug.c src/paren.c src/cast.c src/kw.c

CFLAGS = -rdynamic -Iinclude -DJBAS_ERROR_REASONS -Wall -lm -ldl
CLIBFLAGS = -Iinclude -Wall -lm -fPIC -shared -DJBAS_ERROR_REASONS

CC = clang

ifneq ($(DEBUG),)
CFLAGS += -O0 -DJBAS_DEBUG -DnoJBAS_RESOURCE_DEBUG -fno-omit-frame-pointer -fsanitize=address -fsanitize=undefined -g -ferror-limit=2
else
CFLAGS += -O3 -flto -ffast-math -march=native -ftree-vectorize
endif

all: libs/stdjbas.so
	$(CC) $(CFLAGS) -o jbi $(SRC) 

libs/stdjbas.so: libs/stdjbas.c
	$(CC) $(CLIBFLAGS)  -o libs/stdjbas.so libs/stdjbas.c 