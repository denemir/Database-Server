CC=gcc
CFLAGS= -Wall -Werror -std=gnu99 -pthread

all: clean dbserver dbclient
dbserver: dbserver.c msg.h
	$(CC) $(CFLAGS) -o dbserver dbserver.c
dbclient: dbclient.c msg.h
	$(CC) $(CFLAGS) -o dbclient dbclient.c
clean:
	rm -f dbserver dbclient
