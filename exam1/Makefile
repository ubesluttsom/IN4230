.PHONY = all submission clean indent
CC     = gcc
CFLAGS = -I include -Wall -Wextra -Wpedantic -ggdb

candidate = 15605

src = $(wildcard src/*.c)
hdr = $(wildcard include/*.h)
#obj = $(patsubst src/%.c, build/%.o, $(src))

all: ping_client ping_server mip_daemon

mip_daemon: build/mip_daemon.o build/mip_arp.o build/ping.o build/common.o
	$(CC) $(CFLAGS) $^ -o mip_daemon

ping_client: build/ping_client.o build/ping.o build/common.o
	$(CC) $(CFLAGS) $^ -o ping_client

ping_server: build/ping_server.o build/ping.o build/common.o
	$(CC) $(CFLAGS) $^ -o ping_server

build/%.o: src/%.c $(hdr)
	$(CC) $(CFLAGS) -c $< -o $@

install:
	mkdir -p bin && mv -u mip_daemon ping_client ping_server sender bin/

indent: $(src)
	indent --linux-style --no-tabs --format-all-comments $^

submission: README.md $(src) $(hdr)
	mkdir -p $(candidate)
	pandoc README.md -o $(candidate)/README.pdf
	cp -u -t $(candidate) src include Makefile
	tar -zcv --owner=nobody --group=nobody -f $(candidate).tgz $(candidate)

clean:
	rm -fr build/*.o bin
