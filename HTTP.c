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

#include "HTTP.h"




void set_nonblocking(int sockfd){
    int flags = fcntl(sockfd, F_GETFL, 0);
    if(flags == -1){
       exit(EXIT_FAILURE);
    }
    if(fcntl(sockfd, F_SETFL, flags | O_NONBLOCK) == -1){
        exit(EXIT_FAILURE);
    }
}

void handle_http_request(char *buffer, int client_fd, int epoll_fd) {
    char method[16] = {0}, path[256] = {0}, protocol[16] = {0};
    sscanf(buffer, "%s %s %s", method, path, protocol);

    int keep_alive = 1;
    if (strstr(buffer, "Connection: close") != NULL) {
        keep_alive = 0;
    }

    const char *conn_header = keep_alive ? "Connection: keep-alive" : "Connection: close";
    char response[2048]; 

    if (strcmp(method, "GET") == 0 && strcmp(path, "/") == 0) {
        const char *body = "<h1>System - Main Page</h1>";
        snprintf(response, sizeof(response),
            "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\nContent-Length: %zu\r\n%s\r\n\r\n%s", 
            strlen(body), conn_header, body);
    }
    else if (strcmp(method, "GET") == 0 && strcmp(path, "/api/users") == 0) {
        const char *body = "{\"users\": [\"Nail\", \"Eren\"]}";
        snprintf(response, sizeof(response),
            "HTTP/1.1 200 OK\r\nContent-Type: application/json\r\nContent-Length: %zu\r\n%s\r\n\r\n%s", 
            strlen(body), conn_header, body);
    }
    else {
        const char *body = "404 Page Not Found";
        snprintf(response, sizeof(response),
            "HTTP/1.1 404 Not Found\r\nContent-Type: text/plain\r\nContent-Length: %zu\r\n%s\r\n\r\n%s", 
            strlen(body), conn_header, body);
    }

    
    size_t res_len = strlen(response);
    strncpy(clients[client_fd].write_buffer, response, res_len);
    clients[client_fd].write_len = res_len;
    clients[client_fd].write_pos = 0;
    clients[client_fd].keep_alive = keep_alive; 

    
    struct epoll_event ev;
    ev.events = EPOLLIN | EPOLLOUT | EPOLLET | EPOLLRDHUP;
    ev.data.fd = client_fd;
    epoll_ctl(epoll_fd, EPOLL_CTL_MOD, client_fd, &ev);
}

int get_header_value(const char *request, const char *header_name, char *output, size_t max_len) {
    char search_str[64];
    
    snprintf(search_str, sizeof(search_str), "\r\n%s:", header_name);
    
    
    const char *start = strcasestr(request, search_str);
    if (!start) return 0; 
    
    start += strlen(search_str);
    while (*start == ' ') start++; 
    
    const char *end = strstr(start, "\r\n"); 
    if (!end) return 0;
    
    size_t length = end - start;
    if (length >= max_len) length = max_len - 1;
    
    strncpy(output, start, length);
    output[length] = '\0';
    return 1; 
}