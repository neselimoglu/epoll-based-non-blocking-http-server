#include "stddef.h"
#include <time.h>
#include <unistd.h>


#include "ClientsState.h"
#include <string.h>

ClientsState clients[MAX_CLIENTS];

void init_clients_state(int fd) {
    if(fd < 0 || fd >= MAX_CLIENTS) {
        return;
    }

    clients[fd].fd = fd;
    clients[fd].read_pos = 0;
    clients[fd].keep_alive = 0;
    memset(clients[fd].read_buffer, 0, BUFFER_SIZE); 

    clients[fd].write_pos = 0;
    clients[fd].write_len = 0;
    memset(clients[fd].write_buffer, 0, BUFFER_SIZE);

    clients[fd].last_active = time(NULL);
}


void init_all_clients() {
    for(int i = 0; i < MAX_CLIENTS; i++) {
        clients[i].fd = -1;
    }
}


void close_client(int fd) {
    if(fd >= 0 && fd < MAX_CLIENTS && clients[fd].fd != -1) {
        close(fd);
        clients[fd].fd = -1;
    }
}