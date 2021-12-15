#include <stdlib.h>
#include <stdint.h>

typedef void* cache_handle_t;

cache_handle_t cache_create(size_t cache_size);

void cache_clear(cache_handle_t handle, void(*free_data)(void* data,void* ctx), void* ctx);

void cache_free(cache_handle_t handle, void(*free_data)(void* data,void* ctx),void* ctx);

/*
    Usage: Search the cache first. If cache hits, the data will be the return value.
    If cache misses, NULL will be returned. 
    And after the calculation, the new key-value pair needs to be added by cache_add.
*/
void* cache_get(cache_handle_t handle, void* key, size_t key_len);

/*
    Key is copied into the cache.
    Data is NOT copied.
    DO NOT add before calling get. DO NOT add if cache hits.
    Cache can not be updated. If the value could change for a key, you should NOT use cache.
    If the cache overflows, the tail data will be freed using free_data.
*/
int cache_add(cache_handle_t handle, void* key, size_t key_len, void* data, void(*free_data)(void* data,void* ctx),void* ctx);
