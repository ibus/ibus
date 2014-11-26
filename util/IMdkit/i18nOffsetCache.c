#include <X11/Xlib.h>
#include <stddef.h>
#include "IMdkit.h"
#include "Xi18n.h"

/*
* The XIM specification does not limit the number of window properties
* that can be used to transfer data, but Xlib uses the atom strings
* _client0 through _client20.
*
* So use that as a sensible initial size for the offset cache.
*/
#define INITIAL_OFFSET_CACHE_CAPACITY 21
#define OFFSET_CACHE_GROWTH_FACTOR 2

void _Xi18nInitOffsetCache(Xi18nOffsetCache *offset_cache)
{
    offset_cache->size = 0;
    offset_cache->capacity = INITIAL_OFFSET_CACHE_CAPACITY;
    offset_cache->data = (Xi18nAtomOffsetPair *)malloc(
        INITIAL_OFFSET_CACHE_CAPACITY * sizeof(Xi18nAtomOffsetPair));
}

unsigned long _Xi18nLookupPropertyOffset(Xi18nOffsetCache *offset_cache,
                                         Atom key)
{
    Xi18nAtomOffsetPair *data = offset_cache->data;
    size_t i;

    for (i = 0; i < offset_cache->size; ++i)
    {
        if (data[i].key == key)
        {
            return data[i].offset;
        }
    }

    return 0;
}

void _Xi18nSetPropertyOffset(Xi18nOffsetCache *offset_cache, Atom key,
                             unsigned long offset)
{
    Xi18nAtomOffsetPair *data = offset_cache->data;
    size_t i;

    for (i = 0; i < offset_cache->size; ++i)
    {
        if (data[i].key == key)
        {
            data[i].offset = offset;
            return;
        }
    }

    if (++offset_cache->size > offset_cache->capacity)
    {
        offset_cache->capacity *= OFFSET_CACHE_GROWTH_FACTOR;
        offset_cache->data = data = (Xi18nAtomOffsetPair *)realloc(data,
                offset_cache->capacity * sizeof(Xi18nAtomOffsetPair));
    }

    data[i].key = key;
    data[i].offset = offset;
}

