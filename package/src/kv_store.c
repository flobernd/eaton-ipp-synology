#include "kv_store.h"

#include <stdlib.h>
#include <string.h>
#include <pthread.h>

#define INITIAL_BUCKETS 64

typedef struct kv_entry
{
    kv_store_pair_t pair;
    struct kv_entry* next;
} kv_entry_t;

struct kv_store
{
    pthread_rwlock_t lock;
    kv_entry_t** buckets;
    size_t bucket_count;
};

static size_t hash_key(const char* key, size_t buckets)
{
    size_t hash = 5381;
    while (*key)
    {
        hash = ((hash << 5) + hash) + (unsigned char)(*key++);
    }

    return hash % buckets;
}

kv_store_t* kv_store_create(void)
{
    kv_store_t* store = malloc(sizeof(kv_store_t));
    if (!store)
    {
        return NULL;
    }

    store->bucket_count = INITIAL_BUCKETS;
    store->buckets = calloc(store->bucket_count, sizeof(kv_entry_t*));
    pthread_rwlock_init(&store->lock, NULL);

    return store;
}

void kv_store_destroy(kv_store_t* store)
{
    if (!store)
    {
        return;
    }

    for (size_t i = 0; i < store->bucket_count; ++i)
    {
        kv_entry_t* entry = store->buckets[i];
        while (entry)
        {
            kv_entry_t* tmp = entry;
            entry = entry->next;
            free((void*)tmp->pair.key);
            free(tmp);
        }
    }

    free(store->buckets);
    pthread_rwlock_destroy(&store->lock);
    free(store);

    return;
}

void kv_store_set(kv_store_t* store, const char* key, const void* value)
{
    size_t idx = hash_key(key, store->bucket_count);
    pthread_rwlock_wrlock(&store->lock);

    kv_entry_t* entry = store->buckets[idx];
    while (entry)
    {
        if (strcmp(entry->pair.key, key) == 0)
        {
            entry->pair.value = value;
            pthread_rwlock_unlock(&store->lock);
            return;
        }

        entry = entry->next;
    }

    entry = malloc(sizeof(kv_entry_t));
    entry->pair.key = strdup(key);
    entry->pair.value = value;
    entry->next = store->buckets[idx];
    store->buckets[idx] = entry;
    pthread_rwlock_unlock(&store->lock);

    return;
}

const kv_store_pair_t* kv_store_get(kv_store_t* store, const char* key)
{
    size_t idx = hash_key(key, store->bucket_count);
    pthread_rwlock_rdlock(&store->lock);

    kv_entry_t* entry = store->buckets[idx];
    while (entry)
    {
        if (strcmp(entry->pair.key, key) == 0)
        {
            const kv_store_pair_t* result = &entry->pair;
            pthread_rwlock_unlock(&store->lock);
            return result;
        }

        entry = entry->next;
    }

    pthread_rwlock_unlock(&store->lock);

    return NULL;
}

void kv_store_iterate(kv_store_t* store, kv_store_iter_func iter, void* user_data)
{
    pthread_rwlock_rdlock(&store->lock);

    for (size_t i = 0; i < store->bucket_count; ++i)
    {
        kv_entry_t* entry = store->buckets[i];
        while (entry)
        {
            if (iter(&entry->pair, user_data))
            {
                pthread_rwlock_unlock(&store->lock);
                return;
            }

            entry = entry->next;
        }
    }

    pthread_rwlock_unlock(&store->lock);

    return;
}

kv_store_t* kv_store_init(const kv_store_pair_t* pairs, size_t count)
{
    kv_store_t* store = kv_store_create();
    if (!store)
    {
        return NULL;
    }

    for (size_t i = 0; i < count; ++i)
    {
        kv_store_set(store, pairs[i].key, pairs[i].value);
    }

    return store;
}
