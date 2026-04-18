CC = gcc
CFLAGS = -g -Wall

all: echo client

echo: echo.c ../csapp.c
	$(CC) $(CFLAGS) -o echo echo.c ../csapp.c -lpthread

client: client.c ../csapp.c
	$(CC) $(CFLAGS) -o client client.c ../csapp.c -lpthread

clean:
	rm -f echo client *.o
