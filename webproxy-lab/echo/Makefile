CC = gcc
CFLAGS = -g -Wall

all: echo client

echo: echo.c
	$(CC) $(CFLAGS) -o echo echo.c

client: client.c
	$(CC) $(CFLAGS) -o client client.c

clean:
	rm -f echo client *.o
