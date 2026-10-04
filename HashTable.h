#ifndef HASH_TABLE_H
#define HASH_TABLE_H

#define TABLE_SIZE 10000 

typedef struct Node {
    char *key;
    char *value;
    struct Node *next;
} Node; 
 
unsigned long hash_function(const char *str);
void ht_init();
void ht_set(const char *key, const char *value);
char* ht_get(const char *key);
int ht_delete(const char *key);

#endif