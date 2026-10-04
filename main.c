#define _GNU_SOURCE 
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/epoll.h>
#include <errno.h>
#include <signal.h> 

#include "ClientsState.h"
#include "CommandParser.h"
#include "HashTable.h"

#define MAX_EVENTS 64


void set_nonblocking(int sockfd){
    int flags = fcntl(sockfd, F_GETFL, 0);
    if(flags != -1) {
        fcntl(sockfd, F_SETFL, flags | O_NONBLOCK);
    }
}

int main(){
    signal(SIGPIPE, SIG_IGN);

    init_all_clients();
    ht_init(); 


    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if(server_fd == -1) exit(EXIT_FAILURE);

    int opt = 1;
    if(setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) == -1) exit(EXIT_FAILURE);

    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(8080);

    if(bind(server_fd, (struct sockaddr*)&server_addr, sizeof(server_addr)) == -1) exit(EXIT_FAILURE);
    if(listen(server_fd, SOMAXCONN) == -1) exit(EXIT_FAILURE);

    set_nonblocking(server_fd);

    int epoll_fd = epoll_create1(0);
    if(epoll_fd == -1) exit(EXIT_FAILURE);

    struct epoll_event event;
    event.events = EPOLLIN | EPOLLET; 
    event.data.fd = server_fd; 
    epoll_ctl(epoll_fd, EPOLL_CTL_ADD, server_fd, &event);

    struct epoll_event events[MAX_EVENTS];

    printf("Database Started (Port 8080). Connect with Telnet\n");

    while(1){
        int n = epoll_wait(epoll_fd, events, MAX_EVENTS, 1000);
        if(n == -1) break;

        time_t now = time(NULL);
        for(int i = 0; i < MAX_CLIENTS; i++) {
            if(clients[i].fd != -1) {
                if(now - clients[i].last_active > 50) { 
                    close_client(clients[i].fd);
                }
            }
        }
        ht_sweep_expired(); 
        for(int i = 0; i < n; i++){
            uint32_t current_events = events[i].events;
            int active_fd = events[i].data.fd;

            if (current_events & (EPOLLERR | EPOLLHUP | EPOLLRDHUP)) {
                close_client(active_fd);
                continue;
            }

            if(active_fd == server_fd){
                while(1) {
                    struct sockaddr_in client_addr;
                    socklen_t client_len = sizeof(client_addr);
                    int client_fd = accept(server_fd, (struct sockaddr*)&client_addr, &client_len);
                    
                    if(client_fd == -1) break;

                    set_nonblocking(client_fd);
                    init_clients_state(client_fd);

                    struct epoll_event client_event;
                    client_event.events = EPOLLIN | EPOLLET | EPOLLRDHUP;
                    client_event.data.fd = client_fd;
                    epoll_ctl(epoll_fd, EPOLL_CTL_ADD, client_fd, &client_event);
                }
            } 
    
            else if (current_events & EPOLLOUT) {
                int client_fd = active_fd;
                while (1) {
                    size_t remain = clients[client_fd].write_len - clients[client_fd].write_pos;
                    if (remain == 0) break;

                    ssize_t count = write(client_fd, clients[client_fd].write_buffer + clients[client_fd].write_pos, remain);
                    
                    if (count > 0) {
                        clients[client_fd].write_pos += count;
                    } else if (count == -1) {
                        if (errno == EAGAIN || errno == EWOULDBLOCK) break;
                        close_client(client_fd);
                        break;
                    }
                }
                
                if (clients[client_fd].write_pos >= clients[client_fd].write_len && clients[client_fd].write_len > 0) {
                    if (!clients[client_fd].keep_alive) {
                        close_client(client_fd);
                    } else {
                        clients[client_fd].read_pos = 0;
                        memset(clients[client_fd].read_buffer, 0, BUFFER_SIZE);
                        clients[client_fd].write_len = 0;
                        clients[client_fd].write_pos = 0;

                        struct epoll_event ev;
                        ev.events = EPOLLIN | EPOLLET | EPOLLRDHUP;
                        ev.data.fd = client_fd;
                        epoll_ctl(epoll_fd, EPOLL_CTL_MOD, client_fd, &ev);
                    }
                }
            }
            
            else if (current_events & EPOLLIN) {
                int client_fd = active_fd;
                while(1) {
                    size_t remain = BUFFER_SIZE - clients[client_fd].read_pos - 1;
                    if (remain <= 0) {
                        close_client(client_fd);
                        break;
                    }

                    ssize_t count = read(client_fd, clients[client_fd].read_buffer + clients[client_fd].read_pos, remain);
                    
                    if(count == -1){
                        if (errno == EAGAIN || errno == EWOULDBLOCK) {
                            
                            
                            if (strchr(clients[client_fd].read_buffer, '\n') != NULL) {
                        
                                handle_client_command(clients[client_fd].read_buffer, client_fd, epoll_fd);
                            }
                            
                            break;
                        } else {
                            close_client(client_fd);
                            break;
                        }
                    } else if(count == 0){
                        close_client(client_fd);
                        break;
                    }
                    
                    clients[client_fd].read_pos += count;
                    clients[client_fd].last_active = time(NULL);
                    clients[client_fd].read_buffer[clients[client_fd].read_pos] = '\0';
                }
            }
        }
    }
    return 0;
}