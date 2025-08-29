CC = gcc
CFLAGS = -Wall -O2 -g -o build -D_GNU_SOURCE

build: client server

client: file.o socket.o
server: file.o socket.o

clean:
	rm -f build *.o