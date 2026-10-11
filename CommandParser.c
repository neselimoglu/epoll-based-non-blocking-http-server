#include <stdio.h>
#include <string.h>
#include <strings.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/epoll.h>
#include <time.h>
#include "CommandParser.h"
#include "HashTable.h"
#include "ClientsState.h"

void handle_client_command(char *buffer, int client_fd, int epoll_fd) {
    char args[4][1024] = {0}; 
    char response[4096] = {0};
    int parsed_args = 0;

    // Dinamik RESP Ayrıştırıcı ve Telnet Failsafe
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
        // Hata toleransı (Inline format destekleyici)
        int tokens = sscanf(buffer, "%s %s %[^\r\n]", args[0], args[1], args[2]);
        if (tokens >= 1) parsed_args = tokens;
        else return;
    }

    char *command = args[0];
    char *key = args[1];
    char *value = args[2];

    // strcasecmp: Büyük/Küçük harf duyarlılığını kaldırır (SUBSCRIBE = subscribe)
    if (strcasecmp(command, "PING") == 0) {
        snprintf(response, sizeof(response), "+PONG\r\n");
    }
    else if (strcasecmp(command, "SET") == 0 && parsed_args >= 3) {
        ht_set(key, value);
        snprintf(response, sizeof(response), "+OK\r\n");
    } 
    else if (strcasecmp(command, "GET") == 0 && parsed_args >= 2) {
        char *res = ht_get(key);
        if (res != NULL) snprintf(response, sizeof(response), "$%zu\r\n%s\r\n", strlen(res), res);
        else snprintf(response, sizeof(response), "$-1\r\n"); 
    }
    else if (strcasecmp(command, "DEL") == 0 && parsed_args >= 2) {
        int deleted = ht_delete(key);
        snprintf(response, sizeof(response), ":%d\r\n", deleted); 
    }
    else if (strcasecmp(command, "LPUSH") == 0 && parsed_args >= 3) {
        int len = ht_lpush(key, value);
        if (len == -1) snprintf(response, sizeof(response), "-WRONGTYPE Operation against a key holding the wrong kind of value\r\n");
        else snprintf(response, sizeof(response), ":%d\r\n", len);
    }
    else if (strcasecmp(command, "LRANGE") == 0 && parsed_args >= 4) {
        int start = atoi(args[2]);
        int end = atoi(args[3]);
        char *res = ht_lrange(key, start, end);
        snprintf(response, sizeof(response), "%s", res);
        free(res);
    }
    else if (strcasecmp(command, "RPOP") == 0 && parsed_args >= 2) {
        char *res = ht_rpop(key);
        snprintf(response, sizeof(response), "%s", res);
        free(res);
    }
    else if (strcasecmp(command, "INCR") == 0 && parsed_args >= 2) {
        long new_val;
        if (ht_increment(key, 1, &new_val)) snprintf(response, sizeof(response), ":%ld\r\n", new_val);
        else snprintf(response, sizeof(response), "-ERR value is not an integer\r\n");
    }
    else if (strcasecmp(command, "EXPIRE") == 0 && parsed_args >= 3) {
        int seconds = atoi(value);
        if (seconds <= 0) snprintf(response, sizeof(response), "-ERR Invalid time\r\n");
        else {
            if (ht_expire(key, seconds)) snprintf(response, sizeof(response), ":1\r\n");
            else snprintf(response, sizeof(response), ":0\r\n");
        }
    }
    else if (strcasecmp(command, "SAVE") == 0) {
        if (ht_save("dump.rdb")) snprintf(response, sizeof(response), "+OK\r\n");
        else snprintf(response, sizeof(response), "-ERR Save failed\r\n");
    }
    else if (strcasecmp(command, "BGSAVE") == 0) {
        pid_t pid = fork();
        if (pid == 0) {
            ht_save("dump.rdb");
            exit(0); 
        } else if (pid > 0) {
            snprintf(response, sizeof(response), "+Background saving started\r\n");
        } else {
            snprintf(response, sizeof(response), "-ERR fork failed\r\n");
        }
    }
    else if (strcasecmp(command, "SUBSCRIBE") == 0 && parsed_args >= 2) {
        strncpy(clients[client_fd].subscribed_channel, key, 127);
        clients[client_fd].subscribed_channel[127] = '\0';
        
        // KRİTİK DÜZELTME: Tam sayı taşmasını engellemek için 10 yıl yerine 24 saat ekliyoruz.
        clients[client_fd].last_active = time(NULL) + 86400; 
        
        snprintf(response, sizeof(response), "*3\r\n$9\r\nsubscribe\r\n$%zu\r\n%s\r\n:1\r\n", strlen(key), key);
    }
    else if (strcasecmp(command, "PUBLISH") == 0 && parsed_args >= 3) {
        int receivers = 0;
        char pub_msg[2048];
        
        snprintf(pub_msg, sizeof(pub_msg), "*3\r\n$7\r\nmessage\r\n$%zu\r\n%s\r\n$%zu\r\n%s\r\n", strlen(key), key, strlen(value), value);
        size_t pub_len = strlen(pub_msg);

        for (int i = 0; i < MAX_CLIENTS; i++) {
            if (clients[i].fd != -1 && strcmp(clients[i].subscribed_channel, key) == 0) {
                
                if (clients[i].write_pos == clients[i].write_len) {
                    clients[i].write_pos = 0;
                    clients[i].write_len = 0;
                }
                
                memcpy(clients[i].write_buffer + clients[i].write_len, pub_msg, pub_len);
                clients[i].write_len += pub_len;
                
                struct epoll_event ev;
                ev.events = EPOLLIN | EPOLLOUT | EPOLLET | EPOLLRDHUP;
                ev.data.fd = clients[i].fd;
                epoll_ctl(epoll_fd, EPOLL_CTL_MOD, clients[i].fd, &ev);
                
                receivers++;
            }
        }
        snprintf(response, sizeof(response), ":%d\r\n", receivers);
    }
    else {
        snprintf(response, sizeof(response), "-ERR Unknown Command\r\n");
    }

    // Ortak Müşteri Yanıt Gönderim Motoru
    size_t res_len = strlen(response);
    if (res_len > 0) {
        if (clients[client_fd].write_pos == clients[client_fd].write_len) {
            clients[client_fd].write_pos = 0;
            clients[client_fd].write_len = 0;
        }
        memcpy(clients[client_fd].write_buffer + clients[client_fd].write_len, response, res_len);
        clients[client_fd].write_len += res_len;
        clients[client_fd].keep_alive = 1;

        struct epoll_event ev;
        ev.events = EPOLLIN | EPOLLOUT | EPOLLET | EPOLLRDHUP;
        ev.data.fd = client_fd;
        epoll_ctl(epoll_fd, EPOLL_CTL_MOD, client_fd, &ev);
    }
}