.PHONY: clean

all: fibo.o main.o

fibo.o: fibo.c ../include/fibo.h
	gcc -c fibo.c -I  ../include/

main.o: main.c ../include/fibo.h
	gcc -c main.c -I ../include/

clean:
	rm -f *.o
