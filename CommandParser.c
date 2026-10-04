#include <stdio.h>
#include <string.h>
#include <sys/epoll.h>
#include <stdlib.h>

#include "CommandParser.h"
#include "HashTable.h"
#include "ClientsState.h"

void handle_client_command(char *buffer, int client_fd, int epoll_fd)
{
    char command[16] = {0};
    char key[64] = {0};
    char value[1024] = {0};
    char response[2048] = {0};

    int parsed = sscanf(buffer, "%s %s %[^\r\n]", command, key, value);

    if (strcmp(command, "SET") == 0 && parsed >= 3)
    {
        ht_set(key, value);
        snprintf(response, sizeof(response), "+OK\r\n");
    }
    else if (strcmp(command, "GET") == 0 && parsed >= 2)
    {
        char *res = ht_get(key);
        if (res != NULL)
        {
            snprintf(response, sizeof(response), "%s\r\n", res);
        }
        else
        {
            snprintf(response, sizeof(response), "(nil)\r\n");
        }
    }
    else if (strcmp(command, "DEL") == 0 && parsed >= 2)
    {
        int deleted = ht_delete(key);
        if (deleted)
        {
            snprintf(response, sizeof(response), ":1\r\n");
        }
        else
        {
            snprintf(response, sizeof(response), ":0\r\n");
        }
    }
    else if (strcmp(command, "EXPIRE") == 0 && parsed >= 3)
    {
        int seconds = atoi(value);
        if (seconds <= 0)
        {
            snprintf(response, sizeof(response), "-ERR Invalid time\r\n");
        }
        else
        {
            int success = ht_expire(key, seconds);
            if (success)
            {
                snprintf(response, sizeof(response), ":1\r\n");
            }
            else
            {
                snprintf(response, sizeof(response), ":0\r\n");
            }
        }
    }
    else if (strcmp(command, "SAVE") == 0)
    {
        if (ht_save("dump.rdb"))
        {
            snprintf(response, sizeof(response), "+OK Saved to disk\r\n");
        }
        else
        {
            snprintf(response, sizeof(response), "-ERR Save failed\r\n");
        }
    }
    else if (strcmp(command, "LOAD") == 0)
    {
        if (ht_load("dump.rdb"))
        {
            snprintf(response, sizeof(response), "+OK Loaded from disk\r\n");
        }
        else
        {
            snprintf(response, sizeof(response), "-ERR Load failed\r\n");
        }
    }
    else
    {
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