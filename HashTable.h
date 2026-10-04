#ifndef HASH_TABLE_H
#define HASH_TABLE_H

#define TABLE_SIZE 10000 

#include <time.h>

typedef struct Node {
    char *key;
    char *value;
    time_t timestamp;
    struct Node *next;
} Node; 
 
unsigned long hash_function(const char *str);
void ht_init();

int ht_expire(const char *key, int seconds);
void ht_sweep_expired();

void ht_set(const char *key, const char *value);
char* ht_get(const char *key);
int ht_delete(const char *key);
int ht_save(const char *filename);
int ht_load(const char *filename);
int ht_increment(const char *key, long delta, long *result);

#endif