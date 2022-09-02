test: test.c
	gcc test.c -o test -Wall -Wextra -ggdb

run: test
	./test

submission: martimn.md
	mkdir -p martimn
	cp martimn.md martimn/martimn.md
	tar -zcvf martimn.tgz martimn

clean:
	rm -fr martimn martimn.tgz test
