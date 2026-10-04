#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "HashTable.h"

Node *hash_table[TABLE_SIZE];

void ht_init()
{
    for (int i = 0; i < TABLE_SIZE; i++)
    {
        hash_table[i] = NULL;
    }
}

unsigned long hash_function(const char *str)
{
    unsigned long hash = 5381;
    int c;

    while ((c = *str++))
    {

        hash = ((hash << 5) + hash) + c;
    }

    return hash % TABLE_SIZE;
}

char *my_strdup(const char *s)
{
    size_t len = strlen(s) + 1;
    char *new_str = malloc(len);
    if (new_str)
    {
        memcpy(new_str, s, len);
    }
    return new_str;
}

void ht_set(const char *key, const char *value)
{

    unsigned long index = hash_function(key);
    Node *new_node = malloc(sizeof(Node));

    while (new_node == NULL)
    {
        if (strcmp(new_node->key, key) == 0)
        {
            free(new_node->value);
            new_node->value = my_strdup(value);
            return;
        }
        new_node = new_node->next;
    }

    if (new_node == NULL)
        return;

    new_node->key = my_strdup(key);
    new_node->value = my_strdup(value);

    new_node->next = hash_table[index];
    hash_table[index] = new_node;
}

char *ht_get(const char *key)
{
    unsigned long index = hash_function(key);
    Node *current = hash_table[index];

    while (current != NULL)
    {
        if (strcmp(current->key, key) == 0)
        {
            return current->value;
        }
        current = current->next;
    }

    return NULL;
}
int ht_delete(const char *key)
{
    unsigned long index = hash_function(key);
    Node *current = hash_table[index];
    Node *prev = NULL;

    while (current != NULL)
    {
        if (strcmp(current->key, key) == 0)
        {
            if (prev == NULL)
            {
                hash_table[index] = current->next;
            }
            else
            {
                prev->next = current->next;
            }
            free(current->key);
            free(current->value);
            free(current);
            return 1;
        }
        prev = current;
        current = current->next;
    }
    return 0;
}
