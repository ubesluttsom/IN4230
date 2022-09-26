.PHONY = all submission clean
CC     = gcc
CFLAGS = -Wall -Wextra -ggdb
SRC    = main.c chat.c

all: $(SRC)
	$(CC) $(CFLAGS) $^ -o main

run: main
	./main

submission: martimn.md
	mkdir -p martimn
	cp martimn.md martimn/martimn.md
	tar -zcvf martimn.tgz martimn

clean:
	rm -fr *.o main
