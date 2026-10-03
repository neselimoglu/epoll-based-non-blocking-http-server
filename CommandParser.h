#ifndef COMMAND_PARSER_H
#define COMMAND_PARSER_H

void handle_client_command(char *buffer, int client_fd, int epoll_fd);

#endif