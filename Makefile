CC = gcc
CFLAGS = -O3

SRCS =  main.c ClientsState.c HashTable.c CommandParser.c
TARGET = redis-clone

all:
	$(CC) $(CFLAGS) $(SRCS) -o $(TARGET)

clean:
	rm -f $(TARGET)