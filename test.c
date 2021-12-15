#include <stdio.h>
#include <string.h>
#include "cache.h"

struct 
{
    char* key;
    char* value;
} test_kv[] = {
    {"key1","value1"},
    {"key2","value2"},
    {"key3","value3"},
    {"key1","value1"},
    {"key4","value4"},
    {"key1","value1"},
    {"key1","value1"},
    {"key3","value3"},
    {"key2","value2"}
};

int main(int argc, char const *argv[])
{
    cache_handle_t cache = cache_create(3);
    char* value = NULL;
    
    for(int i=0;i<(sizeof(test_kv)/sizeof(test_kv[0]));i++)
    {
        value = cache_get(cache,test_kv[i].key,strlen(test_kv[i].key));
        if(value == NULL)
        {
            cache_add(cache,test_kv[i].key,strlen(test_kv[i].key),test_kv[i].value,NULL,NULL);
            printf("Add %s:%s\n",test_kv[i].key,test_kv[i].value);
        }
        else
        {
            printf("Hit %s:%s\n",test_kv[i].key,value);
        }
    }
    
    cache_free(cache,NULL,NULL);
    return 0;
}