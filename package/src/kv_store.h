#ifndef KV_STORE_H
#define KV_STORE_H

#include <stddef.h>

typedef struct kv_store_pair
{
    const char* key;
    const char* value;
} kv_store_pair_t;

// Opaque struct for thread-safe tagged key-value store.
typedef struct kv_store kv_store_t;

// Create a new thread-safe tagged key-value store.
kv_store_t* kv_store_create(void);

// Destroy the store and free resources.
void kv_store_destroy(kv_store_t* store);

// Set a value for a key.
// The pointer must remain valid and unchanged for the lifetime of the entry.
void kv_store_set(kv_store_t* store, const char* key, const void* value);

// Get a key-value pair by key. Returns a pointer to a static kv_store_pair_t if found, or NULL.
// The returned pointer is only valid until the store is destroyed.
const kv_store_pair_t* kv_store_get(kv_store_t* store, const char* key);

// Iterator function type: called with the pair and user_data. Return 0 to continue, nonzero to stop.
typedef int (*kv_store_iter_func)(const kv_store_pair_t* pair, void* user_data);

// Iterate over all key-value pairs (thread-safe, blocks writers for the duration).
void kv_store_iterate(kv_store_t* store, kv_store_iter_func iter, void* user_data);

// Create and initialize a store with an array of kv_store_pair_t.
kv_store_t* kv_store_init(const kv_store_pair_t* pairs, size_t count);

#endif // KV_STORE_H
