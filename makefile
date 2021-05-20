CC = gcc
DEBUG = -g
CFLAGS = -Wall -lpthread -c $(DEBUG)
LFLAGS = -Wall -lpthread $(DEBUG)

all: client server

client: client.o
	$(CC) $(LFLAGS) client.o  -o client

server: server.o
	$(CC) $(LFLAGS) server.o  -o server


client.o: client.c
	$(CC) $(CFLAGS) client.c

server.o: server.c
	$(CC) $(CFLAGS) server.c


clean:
	rm -rf *.o *~ client server
