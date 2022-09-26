test: main.c
	gcc main.c -o main -Wall -Wextra -ggdb

run: main
	./main

submission: martimn.md
	mkdir -p martimn
	cp martimn.md martimn/martimn.md
	tar -zcvf martimn.tgz martimn

clean:
	rm -fr martimn martimn.tgz main
