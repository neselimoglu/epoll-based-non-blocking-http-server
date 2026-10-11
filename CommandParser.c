#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/epoll.h>
#include "CommandParser.h"
#include "HashTable.h"
#include "ClientsState.h"

void handle_client_command(char *buffer, int client_fd, int epoll_fd) {
    char args[4][1024] = {0}; 
    char response[4096] = {0};
    int parsed_args = 0;

    
    if (buffer[0] == '*') {
        int num_args = 0;
        char *ptr = buffer;

        if (sscanf(ptr, "*%d\r\n", &num_args) != 1) return;
        ptr = strstr(ptr, "\r\n"); if (!ptr) return; ptr += 2;

        for (int i = 0; i < num_args && i < 4; i++) {
            int len;
            if (sscanf(ptr, "$%d\r\n", &len) != 1) return;
            ptr = strstr(ptr, "\r\n"); if (!ptr) return; ptr += 2;
            
            strncpy(args[i], ptr, len);
            args[i][len] = '\0';
            
            ptr = strstr(ptr, "\r\n"); if (!ptr) return; ptr += 2;
            parsed_args++;
        }
    } else {
        return; 
    }

    char *command = args[0];
    char *key = args[1];
    char *value = args[2];

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
    else if (strcmp(command, "LPUSH") == 0 && parsed_args >= 3) {
        int len = ht_lpush(key, value);
        if (len == -1) snprintf(response, sizeof(response), "-WRONGTYPE Operation against a key holding the wrong kind of value\r\n");
        else snprintf(response, sizeof(response), ":%d\r\n", len);
    }
    else if (strcmp(command, "LRANGE") == 0 && parsed_args >= 4) {
        int start = atoi(args[2]);
        int end = atoi(args[3]);
        char *res = ht_lrange(key, start, end);
        snprintf(response, sizeof(response), "%s", res);
        free(res); 
    }
    else if (strcmp(command, "RPOP") == 0 && parsed_args >= 2) {
        char *res = ht_rpop(key);
        snprintf(response, sizeof(response), "%s", res);
        free(res);
    }
    else if (strcmp(command, "INCR") == 0 && parsed_args >= 2) {
        long new_val;
        if (ht_increment(key, 1, &new_val)) snprintf(response, sizeof(response), ":%ld\r\n", new_val);
        else snprintf(response, sizeof(response), "-ERR value is not an integer\r\n");
    }
    else if (strcmp(command, "EXPIRE") == 0 && parsed_args >= 3) {
        int seconds = atoi(value);
        if (seconds <= 0) snprintf(response, sizeof(response), "-ERR Invalid time\r\n");
        else {
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