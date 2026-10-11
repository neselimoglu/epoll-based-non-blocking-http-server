#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/epoll.h>
#include "CommandParser.h"
#include "HashTable.h"
#include "ClientsState.h"

void handle_client_command(char *buffer, int client_fd, int epoll_fd) {
    char command[32] = {0};
    char key[128] = {0};
    char value[1024] = {0};
    char response[2048] = {0};
    int parsed_args = 0;

    if (buffer[0] == '*') {
        int num_args = 0;
        char *ptr = buffer;

        if (sscanf(ptr, "*%d\r\n", &num_args) != 1);
        ptr = strstr(ptr, "\r\n");
        if (!ptr);
        ptr += 2;

        int len;
        if (sscanf(ptr, "$%d\r\n", &len) != 1);
        ptr = strstr(ptr, "\r\n");
        if (!ptr);
        ptr += 2;
        strncpy(command, ptr, len);
        command[len] = '\0';
        ptr = strstr(ptr, "\r\n");
        if (!ptr);
        ptr += 2;
        parsed_args = 1;

        if (num_args >= 2) {
            if (sscanf(ptr, "$%d\r\n", &len) != 1);
            ptr = strstr(ptr, "\r\n");
            if (!ptr);
            ptr += 2;
            strncpy(key, ptr, len);
            key[len] = '\0';
            ptr = strstr(ptr, "\r\n");
            if (!ptr) ;
            ptr += 2;
            parsed_args = 2;
        }

        if (num_args >= 3) {
            if (sscanf(ptr, "$%d\r\n", &len) != 1);
            ptr = strstr(ptr, "\r\n");
            if (!ptr);
            ptr += 2;
            strncpy(value, ptr, len);
            value[len] = '\0';
            parsed_args = 3;
        }
    }

    if (strcmp(command, "PING") == 0) {
        snprintf(response, sizeof(response), "+PONG\r\n");
    }
    else if (strcmp(command, "SET") == 0 && parsed_args >= 3) {
        ht_set(key, value);
        snprintf(response, sizeof(response), "+OK\r\n");
    } 
    else if (strcmp(command, "GET") == 0 && parsed_args >= 2) {
        char *res = ht_get(key);
        if (res != NULL) {
            snprintf(response, sizeof(response), "$%zu\r\n%s\r\n", strlen(res), res);
        } else {
            snprintf(response, sizeof(response), "$-1\r\n"); 
        }
    }
    else if (strcmp(command, "DEL") == 0 && parsed_args >= 2) {
        int deleted = ht_delete(key);
        snprintf(response, sizeof(response), ":%d\r\n", deleted); 
    }
    else if (strcmp(command, "INCR") == 0 && parsed_args >= 2) {
        long new_val;
        if (ht_increment(key, 1, &new_val)) {
            snprintf(response, sizeof(response), ":%ld\r\n", new_val);
        } else {
            snprintf(response, sizeof(response), "-ERR value is not an integer\r\n");
        }
    }
    else if (strcmp(command, "DECR") == 0 && parsed_args >= 2) {
        long new_val;
        if (ht_increment(key, -1, &new_val)) {
            snprintf(response, sizeof(response), ":%ld\r\n", new_val);
        } else {
            snprintf(response, sizeof(response), "-ERR value is not an integer\r\n");
        }
    }
    else if (strcmp(command, "EXPIRE") == 0 && parsed_args >= 3) {
        int seconds = atoi(value);
        if (seconds <= 0) {
            snprintf(response, sizeof(response), "-ERR Invalid time\r\n");
        } else {
            if (ht_expire(key, seconds)) snprintf(response, sizeof(response), ":1\r\n");
            else snprintf(response, sizeof(response), ":0\r\n");
        }
    }
    else if (strcmp(command, "SAVE") == 0) {
        if (ht_save("dump.rdb")) snprintf(response, sizeof(response), "+OK\r\n");
        else snprintf(response, sizeof(response), "-ERR Save failed\r\n");
    }
    else {
        snprintf(response, sizeof(response), "-ERR Unknown Command\r\n");
    }

    size_t res_len = strlen(response);
    strncpy(clients[client_fd].write_buffer, response, res_len);
    clients[client_fd].write_len = res_len;
    clients[client_fd].write_pos = 0;
    clients[client_fd].keep_alive = 1;

    struct epoll_event ev;
    ev.events = EPOLLIN | EPOLLOUT | EPOLLET | EPOLLRDHUP;
    ev.data.fd = client_fd;
    epoll_ctl(epoll_fd, EPOLL_CTL_MOD, client_fd, &ev);
}