#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "HashTable.h"

Node **hash_table = NULL;
unsigned long table_capacity = INITIAL_CAPACITY;
unsigned long item_count = 0;

void free_value(ValueType type, void *value) {
    if (type == VAL_STRING) {
        free(value);
    } else if (type == VAL_LIST) {
        List *list = (List *)value;
        ListNode *current = list->head;
        while (current != NULL) {
            ListNode *next = current->next;
            free(current->value);
            free(current);
            current = next;
        }
        free(list);
    }
}

void ht_init() {
    hash_table = calloc(table_capacity, sizeof(Node*));
}

unsigned long hash_function(const char *str, unsigned long capacity) {
    unsigned long hash = 5381;
    int c;
    while ((c = *str++)) {
        hash = ((hash << 5) + hash) + c;
    }
    return hash % capacity;
}

void ht_resize() {
    unsigned long new_capacity = table_capacity * 2;
    Node **new_table = calloc(new_capacity, sizeof(Node*));

    for (unsigned long i = 0; i < table_capacity; i++) {
        Node *current = hash_table[i];
        while (current != NULL) {
            Node *next_node = current->next;
            unsigned long new_index = hash_function(current->key, new_capacity);
            
            current->next = new_table[new_index];
            new_table[new_index] = current;
            
            current = next_node;
        }
    }
    free(hash_table); 
    hash_table = new_table;
    table_capacity = new_capacity;
}

void ht_set(const char *key, const char *value) {
    if ((float)item_count / table_capacity >= 0.75) {
        ht_resize();
    }

    unsigned long index = hash_function(key, table_capacity);
    Node *current = hash_table[index];

    while (current != NULL) {
        if (strcmp(current->key, key) == 0) {
            free_value(current->type, current->value); 
            current->type = VAL_STRING;
            current->value = strdup(value);
            current->timestamp = 0;
            return;
        }
        current = current->next;
    }

    Node *new_node = malloc(sizeof(Node));
    new_node->key = strdup(key);
    new_node->type = VAL_STRING;
    new_node->value = strdup(value);
    new_node->timestamp = 0;
    new_node->next = hash_table[index];
    hash_table[index] = new_node;
    
    item_count++; 
}


char *ht_get(const char *key) {
    unsigned long index = hash_function(key, table_capacity);
    Node *current = hash_table[index];

    while (current != NULL) {
        if (strcmp(current->key, key) == 0) {
            if (current->timestamp > 0 && time(NULL) > current->timestamp) {
                ht_delete(key);
                return NULL;
            }
            if (current->type != VAL_STRING) return NULL;
            return (char*)current->value;
        }
        current = current->next;
    }
    return NULL;
}

int ht_delete(const char *key) {
    unsigned long index = hash_function(key, table_capacity);
    Node *current = hash_table[index];
    Node *prev = NULL;

    while (current != NULL) {
        if (strcmp(current->key, key) == 0) {
            if (prev == NULL) hash_table[index] = current->next;
            else prev->next = current->next;
            
            free(current->key);
            free_value(current->type, current->value);
            free(current);
            item_count--; 
            return 1;
        }
        prev = current;
        current = current->next;
    }
    return 0;
}

int ht_increment(const char *key, long delta, long *result) {
    unsigned long index = hash_function(key, table_capacity);
    Node *current = hash_table[index];

    while (current != NULL) {
        if (strcmp(current->key, key) == 0) {
            if (current->timestamp > 0 && time(NULL) > current->timestamp) {
                ht_delete(key);
                break;
            }
            if (current->type != VAL_STRING) return 0; 

            char *endptr;
            long val = strtol((char*)current->value, &endptr, 10);
            if (*endptr != '\0') return 0;

            val += delta;
            *result = val;

            char new_str[32];
            snprintf(new_str, sizeof(new_str), "%ld", val);
            free(current->value);
            current->value = strdup(new_str);
            return 1;
        }
        current = current->next;
    }

    *result = delta;
    char new_str[32];
    snprintf(new_str, sizeof(new_str), "%ld", delta);
    ht_set(key, new_str);
    return 1;
}

int ht_expire(const char *key, int seconds) {
    unsigned long index = hash_function(key, table_capacity);
    Node *current = hash_table[index];

    while (current != NULL) {
        if (strcmp(current->key, key) == 0) {
            current->timestamp = time(NULL) + seconds;
            return 1;
        }
        current = current->next;
    }
    return 0;
}

void ht_sweep_expired() {
    if (item_count == 0) return;

    time_t now = time(NULL);
    int max_cycles = 10; 

    while (max_cycles > 0) {
        int expired_found = 0;
        int keys_sampled = 0;

        for (int i = 0; i < 20; i++) {
            unsigned long random_index = rand() % table_capacity;
            Node *current = hash_table[random_index];
            
            while (current != NULL) {
                Node *next_node = current->next; 
                
                if (current->timestamp > 0) {
                    keys_sampled++;
                    if (now > current->timestamp) {
                        ht_delete(current->key); 
                        expired_found++;
                    }
                }
                current = next_node;
            }
        }

        if (keys_sampled == 0 || (float)expired_found / keys_sampled < 0.25) {
            break; 
        }
        max_cycles--;
    }
}

int ht_lpush(const char *key, const char *val_str) {
    if ((float)item_count / table_capacity >= 0.75) ht_resize();

    unsigned long index = hash_function(key, table_capacity);
    Node *current = hash_table[index];

    while (current != NULL) {
        if (strcmp(current->key, key) == 0) {
            if (current->type != VAL_LIST) return -1; 

            List *list = (List *)current->value;
            ListNode *lnode = malloc(sizeof(ListNode));
            lnode->value = strdup(val_str);
            lnode->prev = NULL;
            lnode->next = list->head;
            
            if (list->head) list->head->prev = lnode;
            list->head = lnode;
            if (!list->tail) list->tail = lnode; 
            
            list->length++;
            return list->length; 
        }
        current = current->next;
    }

    List *list = malloc(sizeof(List));
    ListNode *lnode = malloc(sizeof(ListNode));
    lnode->value = strdup(val_str);
    lnode->next = lnode->prev = NULL;
    list->head = list->tail = lnode;
    list->length = 1;

    Node *new_node = malloc(sizeof(Node));
    new_node->key = strdup(key);
    new_node->type = VAL_LIST;
    new_node->value = list; 
    new_node->timestamp = 0;
    new_node->next = hash_table[index];
    hash_table[index] = new_node;
    
    item_count++;
    return 1;
}

int ht_save(const char *filename) {
    FILE *file = fopen(filename, "wb");
    if (!file) return 0;

    for (unsigned long i = 0; i < table_capacity; i++) {
        Node *current = hash_table[i];
        while (current != NULL) {
            if (current->timestamp == 0 || current->timestamp > time(NULL)) {
                size_t k_len = strlen(current->key);
                
                
                fwrite(&k_len, sizeof(size_t), 1, file);
                fwrite(current->key, 1, k_len, file);
                
                
                fwrite(&current->type, sizeof(ValueType), 1, file);
                fwrite(&current->timestamp, sizeof(time_t), 1, file);

                if (current->type == VAL_STRING) {
                    size_t v_len = strlen((char*)current->value);
                    fwrite(&v_len, sizeof(size_t), 1, file);
                    fwrite(current->value, 1, v_len, file);
                } else if (current->type == VAL_LIST) {
                    List *list = (List*)current->value;
                    fwrite(&list->length, sizeof(unsigned long), 1, file); 
                    
                    ListNode *lnode = list->head;
                    while (lnode != NULL) {
                        size_t v_len = strlen(lnode->value);
                        fwrite(&v_len, sizeof(size_t), 1, file);
                        fwrite(lnode->value, 1, v_len, file);
                        lnode = lnode->next;
                    }
                }
            }
            current = current->next;
        }
    }
    fclose(file);
    return 1;
}

void ht_load(const char *filename) {
    FILE *file = fopen(filename, "rb");
    if (!file) return;

    size_t k_len;
    while (fread(&k_len, sizeof(size_t), 1, file) == 1) {
        char *key = malloc(k_len + 1);
        fread(key, 1, k_len, file);
        key[k_len] = '\0';

        ValueType type;
        time_t timestamp;
        fread(&type, sizeof(ValueType), 1, file);
        fread(&timestamp, sizeof(time_t), 1, file);

        if (type == VAL_STRING) {
            size_t v_len;
            fread(&v_len, sizeof(size_t), 1, file);
            char *value = malloc(v_len + 1);
            fread(value, 1, v_len, file);
            value[v_len] = '\0';
            
            ht_set(key, value);
            free(value);
        } else if (type == VAL_LIST) {
            unsigned long list_length;
            fread(&list_length, sizeof(unsigned long), 1, file);
            
            
            char **temp_arr = malloc(list_length * sizeof(char*));
            for (unsigned long i = 0; i < list_length; i++) {
                size_t v_len;
                fread(&v_len, sizeof(size_t), 1, file);
                temp_arr[i] = malloc(v_len + 1);
                fread(temp_arr[i], 1, v_len, file);
                temp_arr[i][v_len] = '\0';
            }
            
            for (long i = list_length - 1; i >= 0; i--) {
                ht_lpush(key, temp_arr[i]);
                free(temp_arr[i]);
            }
            free(temp_arr);
        }

        
        unsigned long index = hash_function(key, table_capacity);
        Node *current = hash_table[index];
        while (current != NULL) {
            if (strcmp(current->key, key) == 0) {
                current->timestamp = timestamp;
                break;
            }
            current = current->next;
        }
        free(key);
    }
    fclose(file);
}
char *ht_lrange(const char *key, int start, int end) {
    unsigned long index = hash_function(key, table_capacity);
    Node *current = hash_table[index];

    while (current != NULL) {
        if (strcmp(current->key, key) == 0) {
            if (current->type != VAL_LIST) return strdup("-WRONGTYPE Operation against a key holding the wrong kind of value\r\n");

            List *list = (List *)current->value;
            if (list->length == 0) return strdup("*0\r\n");

            int list_len = list->length;
            
            if (start < 0) start = list_len + start;
            if (end < 0) end = list_len + end;
            if (start < 0) start = 0;
            if (end >= list_len) end = list_len - 1;
            if (start > end || start >= list_len) return strdup("*0\r\n");

            int count = end - start + 1;

            char *result = malloc(4096); 
            result[0] = '\0';
            char temp[64];
            snprintf(temp, sizeof(temp), "*%d\r\n", count);
            strcat(result, temp);

            ListNode *lnode = list->head;
            int i = 0;
            while (lnode != NULL && i <= end) {
                if (i >= start) {
                    snprintf(temp, sizeof(temp), "$%zu\r\n", strlen(lnode->value));
                    strcat(result, temp);
                    strcat(result, lnode->value);
                    strcat(result, "\r\n");
                }
                lnode = lnode->next;
                i++;
            }
            return result; 
        }
        current = current->next;
    }
    return strdup("*0\r\n"); 
}

char *ht_rpop(const char *key) {
    unsigned long index = hash_function(key, table_capacity);
    Node *current = hash_table[index];

    while (current != NULL) {
        if (strcmp(current->key, key) == 0) {
            if (current->type != VAL_LIST) return strdup("-WRONGTYPE Operation against a key holding the wrong kind of value\r\n");

            List *list = (List *)current->value;
            if (list->length == 0) return strdup("$-1\r\n"); 

            ListNode *tail = list->tail;
            char *val = strdup(tail->value);

        
            if (list->head == tail) {
                list->head = list->tail = NULL;
            } else {
                list->tail = tail->prev;
                list->tail->next = NULL;
            }
            free(tail->value);
            free(tail);
            list->length--;

            
            if (list->length == 0) ht_delete(key);

            
            char *result = malloc(strlen(val) + 32);
            snprintf(result, strlen(val) + 32, "$%zu\r\n%s\r\n", strlen(val), val);
            free(val);
            return result;
        }
        current = current->next;
    }
    return strdup("$-1\r\n"); 
}