.PHONY: clean

out: hello.c
	gcc hello.c -o hello

run: hello
	./hello

clean:
	rm -f hello
