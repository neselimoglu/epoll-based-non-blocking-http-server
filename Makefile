CC = gcc
CFLAGS = -Wall -Wextra -g -O2

SRCS = main.c HTTP.c ClientsState.c
OBJS = $(SRCS:.c=.o)
TARGET = epoll_server

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^

%.o: %.c HTTP.h ClientsState.h
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS) $(TARGET)