CFLAGS= -Wall -g -I./
LIBS=-lpthread

tiny: tiny.o csapp.o ./cgi-bin/adder.o
	cc -o tiny tiny.o csapp.o $(LIBS)
	gcc -o ./cgi-bin/adder ./cgi-bin/adder.c $(LIBS)
#tiny.o: tiny.c
#       cc -o tiny.o tiny.c $(LIBS)

#csapp.o: csapp.c
#       cc -o csapp.o csapp.c $(LIBS)

adder.o : ./cgi-bin/adder.c
	cc -o ./cgi-bin/adder.o ./cgi-bin/adder.c $(LIBS)

clean:
	rm *.o tiny
	rm ./cgi-bin/*.o ./cgi-bin/adder


re: clean tiny
