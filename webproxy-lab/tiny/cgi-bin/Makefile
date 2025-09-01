CC = gcc
CFLAGS = -O2 -Wall -I ../

CGIS = adder user

all: $(CGIS)

user: user.o

adder: adder.c
	$(CC) $(CFLAGS) -o adder adder.c

clean:
	rm -f adder *~
