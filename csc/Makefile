.PHONY: clean

all:
	$(MAKE) -C code
	gcc ./code/main.o ./code/fibo.o -o fibo

clean:
	$(MAKE) clean -C code
