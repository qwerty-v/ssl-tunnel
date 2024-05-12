CC = gcc
CFLAGS = -Wall -I include
LDFLAGS =

all: build

build: clean
	@find src/ssl-tunnel -name '*.c' | xargs -t $(CC) $(CFLAGS) $(LDFLAGS) -o bin/ssl-tunnel

test: clean
	$(CC) $(CFLAGS) -o bin/unit_test_arrays tests/unit/arrays.c src/ssl-tunnel/arrays.c src/ssl-tunnel/errors.c
	./bin/unit_test_arrays
	$(CC) $(CFLAGS) -o bin/unit_test_memory tests/unit/memory.c src/ssl-tunnel/memory.c src/ssl-tunnel/arrays.c src/ssl-tunnel/errors.c
	./bin/unit_test_memory

clean:
	rm -r bin
	mkdir bin
