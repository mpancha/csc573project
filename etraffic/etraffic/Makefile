CC=gcc
LIBS=-lpthread

all: server client

client: etraffic.o

server: emergency_server.o

etraffic.o: 
	$(CC) -o etraffic.o etraffic.c
 
emergency_server.o:
	$(CC) -o emergency_server.o emergency_server.c

clean: 
	rm -rf *.o
