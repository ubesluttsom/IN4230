.PHONY = all submission clean
CC     = gcc
CFLAGS = -Wall -Wextra -ggdb

all: ping_client ping_server

ping_client: ping_client.c ping.h
	$(CC) $(CFLAGS) $^ -o ping_client

ping_server: ping_server.c ping.h
	$(CC) $(CFLAGS) $^ -o ping_server

submission: martimn.md
	mkdir -p martimn
	cp martimn.md martimn/martimn.md
	tar -zcvf martimn.tgz martimn

clean:
	rm -fr *.o main
