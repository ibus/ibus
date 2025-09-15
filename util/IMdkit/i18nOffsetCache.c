/*
 * Copyright (C) 2014 Peng Huang <shawn.p.huang@gmail.com>
 * Copyright (C) 2014-2025 Red Hat, Inc.
 *
 * Permission to use, copy, modify, distribute, and sell this
 * software and its documentation for any purpose is hereby granted
 * without fee, provided that the above copyright notice appear in
 * all copies and that both that copyright notice and this permission
 * notice appear in supporting documentation, and that the name of
 * the copyright holders not be used in advertising or publicity
 * pertaining to distribution of the software without specific,
 * written prior permission.  The copyright holders make no
 * representations about the suitability of this software for any
 * purpose.  It is provided "as is" without express or implied
 * warranty.
 *
 * THE COPYRIGHT HOLDERS DISCLAIM ALL WARRANTIES WITH REGARD TO THIS
 * SOFTWARE, INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND
 * FITNESS, IN NO EVENT SHALL THE COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN
 * AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION,
 * ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF
 * THIS SOFTWARE.
 *
 * Author: Klemens Baum <klemensbaum@gmail.com>
 */

#include <X11/Xlib.h>
#include <assert.h>
#include <stddef.h>
#include "IMdkit.h"
#include "Xi18n.h"
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

void _Xi18nInitOffsetCache (Xi18nOffsetCache *offset_cache)
{
    offset_cache->size = 0;
    offset_cache->capacity = INITIAL_OFFSET_CACHE_CAPACITY;
    offset_cache->data = (Xi18nAtomOffsetPair *) malloc (
        INITIAL_OFFSET_CACHE_CAPACITY * sizeof (Xi18nAtomOffsetPair));
}

unsigned long _Xi18nLookupPropertyOffset (Xi18nOffsetCache *offset_cache,
                                          Atom key)
{
    Xi18nAtomOffsetPair *data;
    size_t i;

    assert (offset_cache);
    data = offset_cache->data;
    for (i = 0; i < offset_cache->size; ++i) {
        if (data[i].key == key) {
            return data[i].offset;
        }
    }

    return 0;
}

void _Xi18nSetPropertyOffset (Xi18nOffsetCache *offset_cache, Atom key,
                              unsigned long offset)
{
    Xi18nAtomOffsetPair *data = offset_cache->data;
    size_t i;

    assert (data != NULL);
    for (i = 0; i < offset_cache->size; ++i) {
        if (data[i].key == key) {
            data[i].offset = offset;
            return;
        }
    }

    if (++offset_cache->size > offset_cache->capacity) {
        Xi18nAtomOffsetPair *pair;
        offset_cache->capacity *= OFFSET_CACHE_GROWTH_FACTOR;
        pair = (Xi18nAtomOffsetPair *) realloc (data,
                offset_cache->capacity * sizeof (Xi18nAtomOffsetPair));
        if (pair) {
            offset_cache->data = pair;
        } else {
            offset_cache->data = data;
            --offset_cache->size;
        }
        data = offset_cache->data;
    }

    assert (data != NULL);
    if (offset_cache->size > 0 && i < offset_cache->capacity) {
        data[i].key = key;
        data[i].offset = offset;
    }
}

