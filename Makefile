CC = gcc
CFLAGS = -O2 -Wall
LFLAGS =

all:	snake

snake:	snake.o
	$(CC) $(LFLAGS) -o snake snake.o

hello_world.o:	snake.c
	$(CC) $(CFLAGS) -c snake.c
clean:	
	rm -f *~ *.o snake

