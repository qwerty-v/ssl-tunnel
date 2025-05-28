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
	$(CC) $(CFLAGS) -o bin/unit_alloc \
		tests/unit/alloc.c \
		src/ssl-tunnel/lib/alloc.c
	$(CC) $(CFLAGS) -o bin/unit_slice \
		tests/unit/slice.c \
		src/ssl-tunnel/lib/slice.c \
		src/ssl-tunnel/lib/alloc.c \
		src/ssl-tunnel/lib/err.c
	$(CC) $(CFLAGS) -o bin/unit_memscope \
		tests/unit/memscope.c \
		src/ssl-tunnel/lib/memscope.c \
		src/ssl-tunnel/lib/slice.c \
		src/ssl-tunnel/lib/alloc.c \
		src/ssl-tunnel/lib/err.c
	$(CC) $(CFLAGS) -o bin/e2e_client \
		tests/e2e/client.c

clean:
	rm -r bin
	mkdir -p bin/libssltunnel
