CFLAGS= -Wall -g -I./include
LIBS=-lpthread

tiny: tiny.o csapp.o
	cc -o tiny tiny.o csapp.o $(LIBS)
# 	gcc -o tiny tiny.c csapp.c -lpthread

clean:
	rm *.o tiny