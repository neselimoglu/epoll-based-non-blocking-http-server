#ifndef HASH_TABLE_H
#define HASH_TABLE_H

#define INITIAL_CAPACITY 10000 

#include <time.h>


typedef enum { VAL_STRING, VAL_LIST } ValueType;

typedef struct ListNode {
    char *value;
    struct ListNode *next;
    struct ListNode *prev;
} ListNode;

typedef struct List {
    ListNode *head;
    ListNode *tail;
    unsigned long length;
} List;

typedef struct Node {
    char *key;
    ValueType type;   
    void *value;      
    time_t timestamp;
    struct Node *next;
} Node;


int ht_lpush(const char *key, const char *value);
 
unsigned long hash_function(const char *str, unsigned long capacity);
void ht_init();

int ht_expire(const char *key, int seconds);
void ht_sweep_expired();

void ht_set(const char *key, const char *value);
char* ht_get(const char *key);
int ht_delete(const char *key);
int ht_save(const char *filename);
void ht_load(const char *filename);
int ht_increment(const char *key, long delta, long *result);

char *ht_lrange(const char *key, int start, int end);
char *ht_rpop(const char *key);

#endif