CFLAGS= -Wall -g -I./
LIBS=-lpthread

tiny: tiny.o csapp.o
	cc -o tiny tiny.o csapp.o $(LIBS)


clean:
	rm *.o tiny

re: clean tiny
