#ifndef CLIENTS_STATE_H
#define CLIENTS_STATE_H

#include <time.h>

#define MAX_CLIENTS 1000
#define BUFFER_SIZE 4096

typedef struct {
    int fd;
    char read_buffer[BUFFER_SIZE];
    size_t read_pos;
    int keep_alive;

    char write_buffer[BUFFER_SIZE];
    size_t write_pos;
    size_t write_len;

    time_t last_active;
} ClientsState;



extern ClientsState clients[MAX_CLIENTS];

void init_clients_state(int fd);
void init_all_clients();
void close_client(int fd);



#endif
