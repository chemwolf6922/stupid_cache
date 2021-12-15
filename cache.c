#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include "map-xxhash/map.h"
#include "cache.h"

/* a simple list for keeping track of the cache order */

typedef struct list_node_s
{
    void* data;
    struct list_node_s* prev;
    struct list_node_s* next;
} list_node_t;

typedef struct
{
    list_node_t* head;
    list_node_t* tail;
    size_t length;
} list_t;

list_node_t* list_node_create(void* data)
{
    list_node_t* node = malloc(sizeof(list_node_t));
    if(!node)
    {
        return NULL;    
    }
    memset(node,0,sizeof(list_node_t));
    node->data = data;
    return node;
}

void list_node_free(list_node_t* node,void(*free_data)(void* data, void* ctx),void* ctx)
{
    if(!node)
    {
        return;
    }    
    if(free_data != NULL)
    {
        free_data(node->data,ctx);
    }
    free(node);
}

list_t* list_create(void)
{
    list_t* list = malloc(sizeof(list_t));
    if(!list)
    {
        return NULL;
    }
    memset(list,0,sizeof(list_t));
    return list;
}

void list_clear(list_t* list, void(*free_data)(void* data, void* ctx), void* ctx)
{
    if(!list)
    {
        return;
    }
    list_node_t* node = list->head;
    while(node != NULL)
    {
        list_node_t* next = node->next;
        list_node_free(node,free_data,ctx);
        node = next;
    }
    list->head = NULL;
    list->tail = NULL;
    list->length = 0;
}

void list_free(list_t* list, void(*free_data)(void* data,void* ctx), void* ctx)
{
    if(!list)
    {
        return;
    }
    list_clear(list,free_data,ctx);
    free(list);
}

void list_unlink(list_t* list, list_node_t* node)
{
    if(node == NULL)
    {
        return;
    }
    if(node->prev != NULL)
    {
        node->prev->next = node->next;
    }
    else
    {
        /* head */
        list->head = node->next;
    }
    if(node->next != NULL)
    {
        node->next->prev = node->prev;
    }
    else
    {
        /* tail */
        list->tail = node->prev;
    }
    node->prev = NULL;
    node->next = NULL;
    list->length --;
}

void list_push_head(list_t* list, list_node_t* node)
{
    if(!node)
    {
        return;
    }
    node->next = list->head;
    if(node->next != NULL)
    {
        node->next->prev = node;
    }
    node->prev = NULL;
    list->head = node;
    if(list->tail == NULL)
    {
        list->tail = node;
    }
    list->length ++;
}

list_node_t* list_pop_tail(list_t* list)
{
    list_node_t* node = list->tail;
    list_unlink(list,node);
    return node;
}

/* cache implementation */

typedef struct
{
    /* map stores list nodes. And provide o(1) access to the nodes */
    map_handle_t map;
    /* list stores the k-v pair. */
    list_t* list;
    size_t max_size;
} cache_t;

typedef struct
{
    void* key;
    size_t key_len;
    void* data;
} cache_list_data_t;

/*
    Key is copied. Value is NOT.
*/
cache_list_data_t* cache_list_data_create(void* key, size_t key_len, void* value)
{
    cache_list_data_t* data = malloc(sizeof(cache_list_data_t) + key_len);
    if(!data)
        return NULL;
    memset(data,0,sizeof(cache_list_data_t));
    data->key = (void*)(data + 1);
    data->key_len = key_len;
    memcpy(data->key,key,key_len);
    data->data = value;
    return data;
}

typedef struct
{
    void(*free)(void* data, void* ctx);
    void* ctx;
} free_func_t;

/* ctx should be a free_func_t */
void cache_list_data_free(void* data, void* ctx)
{
    if(!data)
        return;
    cache_list_data_t* list_data = data;

    if(ctx != NULL)
    {
        free_func_t* free_func = ctx;
        if(free_func->free != NULL)
        {
            free_func->free(list_data->data,free_func->ctx);
        }
    }
    free(list_data);
}

void cache_clear(cache_handle_t handle, void(*free_data)(void* data,void* ctx), void* ctx)
{
    if(handle == NULL)
        return;
    cache_t* cache = (cache_t*)handle;

    if(cache->list != NULL)
    {
        free_func_t list_data_data_free = {
            .free = free_data,
            .ctx = ctx
        };
        list_clear(cache->list,cache_list_data_free,&list_data_data_free);
    }
    if(cache->map != NULL)
    {
        map_clear(cache->map,NULL,NULL);
    }
}

void cache_free(cache_handle_t handle, void(*free_data)(void* data,void* ctx),void* ctx)
{
    if(handle == NULL)
        return;
    cache_t* cache = (cache_t*)handle;

    cache_clear(cache,free_data,ctx);
    if(cache->list != NULL)
    {
        list_free(cache->list,NULL,NULL);
    }    
    if(cache->map != NULL)
    {
        map_delete(cache->map,NULL,NULL);
    }
    free(cache);
}

cache_handle_t cache_create(size_t cache_size)
{
    cache_t* cache = malloc(sizeof(cache_t));
    if(!cache)
        goto error;
    memset(cache,0,sizeof(cache_t));
    cache->max_size = cache_size;
    cache->list = list_create();
    if(!cache->list)
        goto error;
    cache->map = map_create();
    if(!cache->map)
        goto error;
    return (cache_handle_t)cache;
error:
    cache_free(cache,NULL,NULL);
    return NULL;
}


/*
    Usage: Search the cache first. If cache hits, the data will be the return value.
    If cache misses, NULL will be returned. 
    And after the calculation, the new key-value pair needs to be added by cache_add.
*/
void* cache_get(cache_handle_t handle, void* key, size_t key_len)
{
    if(handle == NULL)
        return NULL;
    cache_t* cache = (cache_t*)handle;

    list_node_t* list_node = map_get(cache->map,key,key_len);
    if(!list_node)
        return NULL;
    /* udpate cache */
    list_unlink(cache->list,list_node);
    list_push_head(cache->list,list_node);
    cache_list_data_t* node_data = list_node->data;
    return node_data->data;
}

/*
    Key is copied into the cache.
    Data is NOT copied.
    DO NOT add before calling get. DO NOT add if cache hits.
    Cache can not be updated. If the value could change for a key, you should NOT use cache.
    If the cache overflows, the tail data will be freed using free_data.
*/
int cache_add(cache_handle_t handle, void* key, size_t key_len, void* data, void(*free_data)(void* data,void* ctx),void* ctx)
{
    if(handle == NULL)
        return -1;
    cache_t* cache = (cache_t*)handle;

    if(map_has(cache->map,key,key_len))
    {
        /* Do not udpate the existed value, but also do not rise an error. */
        return 0;
    }
    bool list_node_linked = false;
    list_node_t* list_node = NULL;
    cache_list_data_t* list_data = NULL;
    /* add the new entry */
    list_data = cache_list_data_create(key,key_len,data);
    if(!list_data)
        goto error;
    list_node = list_node_create(list_data);
    if(!list_node)
        goto error;
    if(map_add(cache->map,key,key_len,list_node) == NULL)
        goto error;
    list_push_head(cache->list,list_node);
    list_node_linked = true;
    /* delete last entry if cache overflowed */
    if(cache->list->length > cache->max_size)
    {
        list_node_t* tail = list_pop_tail(cache->list);
        if(!tail)
            goto error;
        cache_list_data_t* tail_data = tail->data;
        if(!tail_data)
            goto error;
        map_remove(cache->map,tail_data->key,tail_data->key_len);
        free_func_t tail_data_data_free = {
            .free = free_data,
            .ctx = ctx
        };
        list_node_free(tail,cache_list_data_free,&tail_data_data_free);
    }
    return 0;
error:
    if(list_node_linked)
        list_unlink(cache->list,list_node);
    if(list_node != NULL)
        list_node_free(list_node,NULL,NULL);
    if(list_data != NULL)
        cache_list_data_free(list_data,NULL);
    map_remove(cache->map,key,key_len);
    return -1;
}
