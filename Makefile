CC = gcc
CFLAGS = -Wall -I include
LDFLAGS =

all: build

build: clean
	@find src/ssl-tunnel -name '*.c' | xargs -t $(CC) $(CFLAGS) $(LDFLAGS) -o bin/ssl-tunnel

clean:
	rm -r bin
	mkdir bin
