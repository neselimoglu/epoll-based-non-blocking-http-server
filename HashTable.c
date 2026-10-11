#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "HashTable.h"

// YENİ DİNAMİK YAPILAR
Node **hash_table = NULL;
unsigned long table_capacity = INITIAL_CAPACITY;
unsigned long item_count = 0;

// Sunucu başlarken tabloyu tahsis et
void ht_init() {
    hash_table = calloc(table_capacity, sizeof(Node*));
}

// Kapasiteye göre Hash hesaplama
unsigned long hash_function(const char *str, unsigned long capacity) {
    unsigned long hash = 5381;
    int c;
    while ((c = *str++)) {
        hash = ((hash << 5) + hash) + c;
    }
    return hash % capacity;
}

// YENİ: Tabloyu %75 dolduğunda 2 katına çıkarma motoru
void ht_resize() {
    unsigned long new_capacity = table_capacity * 2;
    Node **new_table = calloc(new_capacity, sizeof(Node*));

    for (unsigned long i = 0; i < table_capacity; i++) {
        Node *current = hash_table[i];
        while (current != NULL) {
            Node *next_node = current->next;
            
            // Elemanın yeni tablodaki indeksini hesapla
            unsigned long new_index = hash_function(current->key, new_capacity);
            
            // Yeni tabloya yerleştir
            current->next = new_table[new_index];
            new_table[new_index] = current;
            
            current = next_node;
        }
    }
    free(hash_table); // Eski küçük tabloyu RAM'den sil
    hash_table = new_table;
    table_capacity = new_capacity;
}

// Veri Ekleme / Güncelleme
void ht_set(const char *key, const char *value) {
    // Doluluk oranı (Load Factor) %75'i geçtiyse tabloyu büyüt
    if ((float)item_count / table_capacity >= 0.75) {
        ht_resize();
    }

    unsigned long index = hash_function(key, table_capacity);
    Node *current = hash_table[index];

    while (current != NULL) {
        if (strcmp(current->key, key) == 0) {
            free(current->value);
            current->value = strdup(value);
            current->timestamp = 0;
            return;
        }
        current = current->next;
    }

    Node *new_node = malloc(sizeof(Node));
    new_node->key = strdup(key);
    new_node->value = strdup(value);
    new_node->timestamp = 0;
    new_node->next = hash_table[index];
    hash_table[index] = new_node;
    
    item_count++; // Yeni eleman eklendiğinde sayacı artır
}

// Veri Okuma
char *ht_get(const char *key) {
    unsigned long index = hash_function(key, table_capacity);
    Node *current = hash_table[index];

    while (current != NULL) {
        if (strcmp(current->key, key) == 0) {
            if (current->timestamp > 0 && time(NULL) > current->timestamp) {
                ht_delete(key);
                return NULL;
            }
            return current->value;
        }
        current = current->next;
    }
    return NULL;
}

// Veri Silme
int ht_delete(const char *key) {
    unsigned long index = hash_function(key, table_capacity);
    Node *current = hash_table[index];
    Node *prev = NULL;

    while (current != NULL) {
        if (strcmp(current->key, key) == 0) {
            if (prev == NULL) hash_table[index] = current->next;
            else prev->next = current->next;
            
            free(current->key);
            free(current->value);
            free(current);
            item_count--; // Silinince sayacı azalt
            return 1;
        }
        prev = current;
        current = current->next;
    }
    return 0;
}

// Matematiksel Operasyon (INCR/DECR)
int ht_increment(const char *key, long delta, long *result) {
    unsigned long index = hash_function(key, table_capacity);
    Node *current = hash_table[index];

    while (current != NULL) {
        if (strcmp(current->key, key) == 0) {
            if (current->timestamp > 0 && time(NULL) > current->timestamp) {
                ht_delete(key);
                break;
            }

            char *endptr;
            long val = strtol(current->value, &endptr, 10);
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

// Süre Atama (EXPIRE)
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

// Çöp Toplayıcı (Sweep)
void ht_sweep_expired() {
    time_t now = time(NULL);
    for (unsigned long i = 0; i < table_capacity; i++) {
        Node *current = hash_table[i];
        while (current != NULL) {
            Node *next_node = current->next;
            if (current->timestamp > 0 && now > current->timestamp) {
                ht_delete(current->key);
            }
            current = next_node;
        }
    }
}

// Diske Kaydet
int ht_save(const char *filename) {
    FILE *file = fopen(filename, "wb");
    if (!file) return 0;

    for (unsigned long i = 0; i < table_capacity; i++) {
        Node *current = hash_table[i];
        while (current != NULL) {
            if (current->timestamp == 0 || current->timestamp > time(NULL)) {
                size_t k_len = strlen(current->key);
                size_t v_len = strlen(current->value);

                fwrite(&k_len, sizeof(size_t), 1, file);
                fwrite(current->key, 1, k_len, file);
                fwrite(&v_len, sizeof(size_t), 1, file);
                fwrite(current->value, 1, v_len, file);
                fwrite(&current->timestamp, sizeof(time_t), 1, file);
            }
            current = current->next;
        }
    }
    fclose(file);
    return 1;
}

// Diskten Yükle
void ht_load(const char *filename) {
    FILE *file = fopen(filename, "rb");
    if (!file) return;

    size_t k_len, v_len;
    time_t timestamp;

    while (fread(&k_len, sizeof(size_t), 1, file) == 1) {
        char *key = malloc(k_len + 1);
        fread(key, 1, k_len, file);
        key[k_len] = '\0';

        fread(&v_len, sizeof(size_t), 1, file);
        char *value = malloc(v_len + 1);
        fread(value, 1, v_len, file);
        value[v_len] = '\0';

        fread(&timestamp, sizeof(time_t), 1, file);

        ht_set(key, value);

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
        free(value);
    }
    fclose(file);
}