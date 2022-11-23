.PHONY = all submission clean indent killall A B C D E
CC     = gcc
CFLAGS = -I include -Wall -Wextra -Wpedantic -ggdb -O0

candidate = 15605

src = $(wildcard src/*.c)
hdr = $(wildcard include/*.h)
#obj = $(patsubst src/%.c, build/%.o, $(src))

all: ping_client ping_server mip_daemon mip_router

mip_router: build/mip_router.o build/mip_arp.o build/mip_unix.o build/common.o build/queue.o
	$(CC) $(CFLAGS) $^ -o mip_router

mip_daemon: build/mip_daemon.o build/mip_arp.o build/mip_unix.o build/common.o build/queue.o
	$(CC) $(CFLAGS) $^ -o mip_daemon

ping_client: build/ping_client.o build/mip_unix.o build/common.o
	$(CC) $(CFLAGS) $^ -o ping_client

ping_server: build/ping_server.o build/mip_unix.o build/common.o
	$(CC) $(CFLAGS) $^ -o ping_server

build/%.o: src/%.c $(hdr)
	$(CC) $(CFLAGS) -c $< -o $@

install:
	mkdir -p bin && mv -u mip_daemon ping_client ping_server sender bin/

indent: $(src) $(hdr)
	indent --linux-style --no-tabs --format-all-comments $^

submission: README.md $(src) $(hdr)
	mkdir -p $(candidate)
	pandoc README.md -o $(candidate)/README.pdf
	cp -u -t $(candidate) src include Makefile
	tar -zcv --owner=nobody --group=nobody -f $(candidate).tgz $(candidate)

clean:
	rm -fr build/*.o bin mip_daemon ping_server ping_client *.socket


# Recipies for testing:

killall:
	killall mip_daemon; killall ping_server; killall mip_router

A:
	./mip_daemon -d serverA.socket 1 &
	./mip_router -d serverA.socket &
	./ping_server serverA.socket &

B:
	./mip_daemon -d serverB.socket 2 &
	./mip_router -d serverB.socket &
	./ping_server serverB.socket &

C:
	./mip_daemon -d serverC.socket 3 &
	./mip_router -d serverC.socket &
	./ping_server serverC.socket &

D:
	./mip_daemon -d serverD.socket 4 &
	./mip_router -d serverD.socket &
	./ping_server serverD.socket &

E:
	./mip_daemon -d serverE.socket 5 &
	./mip_router -d serverE.socket &
	./ping_server serverE.socket &
