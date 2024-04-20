CC=gcc
CFLAGS=-g -lm

problem2 : problem2.o helpers.o
	$(CC) -o problem2 helpers.o problem2.o -pthread

.PHONY : clean

clean : 
	rm *.o $(objects) problem2