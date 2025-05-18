CC = gcc
CFLAGS = -Wall -Wextra -I"$(shell realpath include)"
LDFLAGS =
AR = ar

all: build

build: clean
	cp src/ssl-tunnel/lib/*.c bin/libssltunnel
	cp src/ssl-tunnel/backend/*.c bin/libssltunnel

	$(CC) -shared -fPIC $(CFLAGS) $(LDFLAGS) -o bin/libssltunnel.so bin/libssltunnel/*.c

	@for f in bin/libssltunnel/*.c; do \
		$(CC) $(CFLAGS) $(LDFLAGS) -c $$f -o bin/libssltunnel/$$(basename $$f .c).o; \
	done
	$(AR) rcs bin/libssltunnel.a bin/libssltunnel/*.o

	$(CC) $(CFLAGS) $(LDFLAGS) -o bin/ssl-tunnel \
	  deamon/linux/*.c \
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
	mkdir -p bin/libssltunnel
