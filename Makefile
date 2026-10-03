CC = gcc
CFLAGS = -Wall -Wextra -g -O2

<<<<<<< HEAD

=======
>>>>>>> 9507df1cf66e1133c643a76f39006c0be0726896
SRCS = main.c HTTP.c ClientsState.c
OBJS = $(SRCS:.c=.o)
TARGET = epoll_server

<<<<<<< HEAD

all: $(TARGET)


=======
all: $(TARGET)

>>>>>>> 9507df1cf66e1133c643a76f39006c0be0726896
$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^

%.o: %.c HTTP.h ClientsState.h
	$(CC) $(CFLAGS) -c $< -o $@

<<<<<<< HEAD

=======
>>>>>>> 9507df1cf66e1133c643a76f39006c0be0726896
clean:
	rm -f $(OBJS) $(TARGET)