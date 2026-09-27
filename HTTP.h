#ifndef HTTP_H
#define HTTP_H




void set_nonblocking(int sockfd);
void handle_http_request(char *buffer, int client_fd, int epoll_fd);
int get_header_value(const char *request, const char *header_name, char *output, size_t max_len);

#endif 