CC = gcc
CFLAGS = -Wall -Wextra -I include
LDFLAGS =
AR = ar

all: build

build: clean
	$(CC) $(CFLAGS) $(LDFLAGS) -c src/ssl-tunnel/lib/*.c && \
	$(AR) rcs bin/libssltunnel.a *.o
	@find . -maxdepth 1 -name '*.o' -exec rm {} \;

	$(CC) $(CFLAGS) $(LDFLAGS) -o bin/ssl-tunnel-server \
	  src/ssl-tunnel/server/*.c \
	  bin/libssltunnel.a

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
