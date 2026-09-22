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

#define MAX_EVENTS 64

void set_nonblocking(int sockfd){
    int flags = fcntl(sockfd, F_GETFL, 0);
    if(flags == -1){
       exit(EXIT_FAILURE);
    }
    if(fcntl(sockfd, F_SETFL, flags | O_NONBLOCK) == -1){
        exit(EXIT_FAILURE);
    }
}

int handle_http_request(char *buffer, int client_fd) {
    char method[16] = {0};
    char path[256] = {0};
    char protocol[16] = {0};

    sscanf(buffer, "%s %s %s", method, path, protocol);

    int keep_alive = 1;
    if (strstr(buffer, "Connection: close") != NULL) {
        keep_alive = 0;
    }

    const char *conn_header = keep_alive ? "Connection: keep-alive" : "Connection: close";
    char response[2048]; 

    if (strcmp(method, "GET") == 0 && strcmp(path, "/") == 0) {
        const char *body = "<h1>Home Page</h1>";
        snprintf(response, sizeof(response),
            "HTTP/1.1 200 OK\r\n"
            "Content-Type: text/html\r\n"
            "Content-Length: %zu\r\n"
            "%s\r\n\r\n"
            "%s", strlen(body), conn_header, body);
    }
    else if (strcmp(method, "GET") == 0 && strcmp(path, "/api/users") == 0) {
        const char *body = "{\"users\": [\"Nail\", \"Eren\"]}";
        snprintf(response, sizeof(response),
            "HTTP/1.1 200 OK\r\n"
            "Content-Type: application/json\r\n"
            "Content-Length: %zu\r\n"
            "%s\r\n\r\n"
            "%s", strlen(body), conn_header, body);
    }
    else {
        const char *body = "404 Page Not Found";
        snprintf(response, sizeof(response),
            "HTTP/1.1 404 Not Found\r\n"
            "Content-Type: text/plain\r\n"
            "Content-Length: %zu\r\n"
            "%s\r\n\r\n"
            "%s", strlen(body), conn_header, body);
    }

    ssize_t bytes_written = write(client_fd, response, strlen(response));
    if (bytes_written == -1) {
       // perror("write");
    }
    return keep_alive ? 0 : 1; 
}

int main(){
    signal(SIGPIPE, SIG_IGN);

    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if(server_fd == -1){
        exit(EXIT_FAILURE);
    }

    int opt = 1;
    if(setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) == -1){
        exit(EXIT_FAILURE);
    }

    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(8080);

    if(bind(server_fd, (struct sockaddr*)&server_addr, sizeof(server_addr)) == -1){
        exit(EXIT_FAILURE);
    }

    if(listen(server_fd, SOMAXCONN) == -1){ 
        exit(EXIT_FAILURE);
    }

    set_nonblocking(server_fd);

    int epoll_fd = epoll_create1(0);
    if(epoll_fd == -1){
        exit(EXIT_FAILURE);
    }

    struct epoll_event event;
    event.events = EPOLLIN | EPOLLET; 
    event.data.fd = server_fd; 
    epoll_ctl(epoll_fd, EPOLL_CTL_ADD, server_fd, &event);

    struct epoll_event events[MAX_EVENTS];

    while(1){
        int n = epoll_wait(epoll_fd, events, MAX_EVENTS, -1);
        if(n == -1){
            break;
        }

        for(int i = 0; i < n; i++){
            if(events[i].data.fd == server_fd){
                while(1) {
                    struct sockaddr_in client_addr;
                    socklen_t client_len = sizeof(client_addr);
                    int client_fd = accept(server_fd, (struct sockaddr*)&client_addr, &client_len);
                    
                    if(client_fd == -1){
                        if (errno == EAGAIN || errno == EWOULDBLOCK) {
                            break; 
                        } else {
                            break;
                        }
                    }

                    set_nonblocking(client_fd);

                    struct epoll_event client_event;
                    client_event.events = EPOLLIN | EPOLLET;
                    client_event.data.fd = client_fd;
                    epoll_ctl(epoll_fd, EPOLL_CTL_ADD, client_fd, &client_event);
                }
            } else {
                int client_fd = events[i].data.fd;
                char buffer[2048];
                buffer[0] = '\0';
                int has_data = 0;
                while(1) {
                    ssize_t count = read(client_fd, buffer, sizeof(buffer) - 1);
                    if(count == -1){
                        if (errno == EAGAIN || errno == EWOULDBLOCK) {
                            if(has_data) {
                                int should_close = handle_http_request(buffer, client_fd);
                                if (should_close) {
                                    close(client_fd);
                                }
                            }
                            break;
                        } else {
                            close(client_fd);
                            break;
                        }
                    } else if(count == 0){
                        close(client_fd);
                        break;
                    }
                    
                    buffer[count] = '\0';
                    has_data = 1;
                }
            }
        }
    }
    return 0;
}