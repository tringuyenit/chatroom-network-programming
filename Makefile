CFLAGS = -c -Wall
CC = gcc

all: server client

server:  server.o server-helper.o utils.o
	${CC} server.o  server-helper.o  utils.o -pthread -o server  

server-helper.o: server-helper.c
	${CC} ${CFLAGS} server-helper.c

client: client.o utils.o
	${CC} client.o utils.o -pthread -o client

client.o: client.c
	${CC} ${CFLAGS} client.c

utils.o: utils.c
	${CC} ${CFLAGS} utils.c

clean:
	rm -f *.o *~