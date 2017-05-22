CC = g++

CFLAGS  = -g -Wall

default: impliedbook

impliedbook:  main.o ib_builder.o channels.o
	$(CC) $(CFLAGS) -o impliedbook main.o ib_builder.o channels.o

main.o:  main.cpp
	$(CC) $(CFLAGS) -c main.cpp

ib_builder.o:  ib_builder.cpp ib_builder.hpp
	$(CC) $(CFLAGS) -c ib_builder.cpp

channels.o:  channels.cpp channels.hpp
	$(CC) $(CFLAGS) -c channels.cpp

clean: 
	rm -rf impliedbook *.o *~
