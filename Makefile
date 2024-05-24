CC = gcc
CFLAGS = -Wall -I include
LDFLAGS =

all: build

build: clean
	@find src/ssl-tunnel -name '*.c' | xargs -t $(CC) $(CFLAGS) $(LDFLAGS) -o bin/ssl-tunnel

test: clean
	$(CC) $(CFLAGS) -o bin/unit_test_arrays \
		tests/unit/arrays.c \
		src/ssl-tunnel/arrays.c \
		src/ssl-tunnel/alloc.c \
		src/ssl-tunnel/errors.c
	$(CC) $(CFLAGS) -o bin/unit_test_memory \
		tests/unit/memory.c \
		src/ssl-tunnel/memory.c \
		src/ssl-tunnel/arrays.c \
		src/ssl-tunnel/alloc.c \
		src/ssl-tunnel/errors.c
	$(CC) $(CFLAGS) -o bin/e2e_client \
		tests/e2e/client.c

clean:
	rm -r bin
	mkdir bin
