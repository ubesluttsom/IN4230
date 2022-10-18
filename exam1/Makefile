.PHONY = all submission clean indent
CC     = gcc
CFLAGS = -I include -Wall -Wextra -Wpedantic -ggdb

src = $(wildcard src/*.c)
#obj = $(patsubst src/%.c, build/%.o, $(src))
hdr = $(wildcard include/*.h)

mip_daemon: build/mip_daemon.o build/mip_arp.o build/common.o
	$(CC) $(CFLAGS) $^ -o mip_daemon

ping_client: build/ping_client.o build/common.o
	$(CC) $(CFLAGS) $^ -o ping_client

ping_server: build/ping_server.o build/common.o
	$(CC) $(CFLAGS) $^ -o ping_server

build/%.o: src/%.c ${hdr}
	$(CC) $(CFLAGS) -c $< -o $@

all: ping_client ping_server mip_daemon

install:
	mkdir -p bin && mv -u mip_daemon ping_client ping_server sender bin/

indent: $(src)
	indent --linux-style --no-tabs --format-all-comments $^

submission: martimn.md
	mkdir -p martimn
	cp martimn.md martimn/martimn.md
	tar -zcvf martimn.tgz martimn

clean:
	rm -fr build/*.o bin
