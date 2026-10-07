/* -*- mode: C; c-basic-offset: 4; indent-tabs-mode: nil; -*- */
/* vim:set et sts=4: */
/* bus - The Input Bus
 * Copyright (C) 2018-2019 Takao Fujiwara <takao.fujiwara1@gmail.com>
 * Copyright (C) 2018-2019 Red Hat, Inc.
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, see <https://www.gnu.org/licenses/>.
 */

#if !defined (__IBUS_H_INSIDE__) && !defined (IBUS_COMPILATION)
#error "Only <ibus.h> can be included directly"
#endif

#ifndef __IBUS_UNICODE_H_
#define __IBUS_UNICODE_H_

/**
 * SECTION: ibusunicode
 * @short_description: unicode utility.
 * @stability: Unstable
 *
 * miscellaneous unicode APIs.
 */

#include <gio/gio.h>
#include "ibusserializable.h"

/*
 * Type macros.
 */
/* define GOBJECT macros */
#define IBUS_TYPE_UNICODE_DATA       (ibus_unicode_data_get_type ())
#define IBUS_UNICODE_DATA(obj)       (G_TYPE_CHECK_INSTANCE_CAST ((obj), \
                                      IBUS_TYPE_UNICODE_DATA, IBusUnicodeData))
#define IBUS_UNICODE_DATA_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST ((klass), \
                                      IBUS_TYPE_UNICODE_DATA, \
                                      IBusUnicodeDataClass))
#define IBUS_IS_UNICODE_DATA(obj)    (G_TYPE_CHECK_INSTANCE_TYPE ((obj), \
                                      IBUS_TYPE_UNICODE_DATA))
#define IBUS_TYPE_UNICODE_BLOCK      (ibus_unicode_block_get_type ())
#define IBUS_UNICODE_BLOCK(obj)      (G_TYPE_CHECK_INSTANCE_CAST ((obj), \
                                      IBUS_TYPE_UNICODE_BLOCK, \
                                      IBusUnicodeBlock))
#define IBUS_UNICODE_BLOCK_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST ((klass), \
                                      IBUS_TYPE_UNICODE_BLOCK, \
                                      IBusUnicodeBlockClass))
#define IBUS_IS_UNICODE_BLOCK(obj)   (G_TYPE_CHECK_INSTANCE_TYPE ((obj), \
                                      IBUS_TYPE_UNICODE_BLOCK))


G_BEGIN_DECLS

typedef struct _IBusUnicodeData IBusUnicodeData;
typedef struct _IBusUnicodeDataPrivate IBusUnicodeDataPrivate;
typedef struct _IBusUnicodeDataClass IBusUnicodeDataClass;
typedef struct _IBusUnicodeBlock IBusUnicodeBlock;
typedef struct _IBusUnicodeBlockPrivate IBusUnicodeBlockPrivate;
typedef struct _IBusUnicodeBlockClass IBusUnicodeBlockClass;

/**
 * IBusUnicodeDataLoadAsyncFinish:
 * @data_list: (transfer full) (element-type IBusUnicodeData):
 *
 * This callback can receive the list of #IBusUnicodeData.
 */
typedef void (*IBusUnicodeDataLoadAsyncFinish) (GSList  *data_list,
                                                gpointer user_data);

/**
 * IBusUnicodeData:
 *
 * Unicode data likes code, name, alias, block-name.
 * You can get extended values with g_object_get_properties.
 */
struct _IBusUnicodeData {
    IBusSerializable parent;
    /* instance members */

    /*< public >*/
    /*< private >*/
    IBusUnicodeDataPrivate *priv;
};

struct _IBusUnicodeDataClass {
    IBusSerializableClass parent;
    /* class members */
};

struct _IBusUnicodeBlock {
    IBusSerializable parent;
    /* instance members */

    /*< public >*/
    /*< private >*/
    IBusUnicodeBlockPrivate *priv;
};

struct _IBusUnicodeBlockClass {
    IBusSerializableClass parent;
    /* class members */
};

GType           ibus_unicode_data_get_type    (void);
GType           ibus_unicode_block_get_type   (void);

/**
 * ibus_unicode_data_new:
 * @first_property_name: Name of the first property.
 * @...: the NULL-terminated arguments of the properties and values.
 *
 * Creates a new #IBusUnicodeData.
 * code property is required. e.g.
 * ibus_unicode_data_new ("code", 0x3042, NULL)
 *
 * Returns: A newly allocated #IBusUnicodeData.
 */
IBusUnicodeData * ibus_unicode_data_new       (const gchar *first_property_name,
                                               ...);

/**
 * ibus_unicode_data_get_code:
 * @unicode: An #IBusUnicodeData
 *
 * Gets the code point in #IBusUnicodeData.
 *
 * Returns: code property in #IBusUnicodeData
 */
gunichar          ibus_unicode_data_get_code  (IBusUnicodeData    *unicode);

/**
 * ibus_unicode_data_get_name:
 * @unicode: An #IBusUnicodeData
 *
 * Gets the name in #IBusUnicodeData. It should not be freed.
 *
 * Returns: name property in #IBusUnicodeData
 */
const gchar *     ibus_unicode_data_get_name  (IBusUnicodeData    *unicode);

/**
 * ibus_unicode_data_get_alias:
 * @unicode: An #IBusUnicodeData
 *
 * Gets the alias in #IBusUnicodeData. It should not be freed.
 *
 * Returns: alias property in #IBusUnicodeData
 */
const gchar *     ibus_unicode_data_get_alias (IBusUnicodeData    *unicode);

/**
 * ibus_unicode_data_get_block_name:
 * @unicode: An #IBusUnicodeData
 *
 * Gets the block name in #IBusUnicodeData. It should not be freed.
 *
 * Returns: block-name property in #IBusUnicodeData
 */
const gchar *     ibus_unicode_data_get_block_name
                                              (IBusUnicodeData    *unicode);

/**
 * ibus_unicode_data_set_block_name:
 * @unicode: An #IBusUnicodeData
 * @block_name: A block name
 *
 * Sets the block name in #IBusUnicodeData.
 */
void              ibus_unicode_data_set_block_name
                                              (IBusUnicodeData    *unicode,
                                               const gchar        *block_name);

/**
 * ibus_unicode_data_save:
 * @path: A path of the saved Unicode data.
 * @list: (element-type IBusUnicodeData) (transfer none): A list of unicode
 *  data.
 *
 * Save the list of #IBusUnicodeData to the cache file.
 */
void              ibus_unicode_data_save      (const gchar        *path,
                                               GSList             *list);

/**
 * ibus_unicode_data_load:
 * @path: A path of the saved dictionary file.
 * @object: (nullable): If the #GObject has "unicode-deserialize-progress"
 *    signal, this function will emit (the number of desrialized
 *    #IBusUnicodeData, * the total number of #IBusUnicodeData) of uint values
 *    with that signal by 100 times. Otherwise %NULL.
 *
 * Returns: (element-type IBusUnicodeData) (transfer full):
 * An #IBusUnicodeData list loaded from the saved cache file.
 */
GSList *          ibus_unicode_data_load      (const gchar        *path,
                                               GObject            *object);

/**
 * ibus_unicode_data_load_async:
 * @path: A path of the saved dictionary file.
 * @object: (nullable): If the #GObject has "unicode-deserialize-progress"
 *    signal, this function will emit (the number of desrialized
 *    #IBusUnicodeData, * the total number of #IBusUnicodeData) of uint values
 *    with that signal by 100 times. Otherwise %NULL.
 * @cancellable: cancellable.
 * @callback: (scope notified): IBusUnicodeDataLoadAsyncFinish.
 * @user_data: User data.
 *
 * IBusUnicodeDataLoadAsyncFinish can receive the list of #IBusUnicodeData.
 */
void              ibus_unicode_data_load_async
                                              (const gchar        *path,
                                               GObject            *object,
                                               GCancellable       *cancellable,
                                               IBusUnicodeDataLoadAsyncFinish
                                                                   callback,
                                               gpointer            user_data);

/**
 * ibus_unicode_block_new:
 * @first_property_name: Name of the first property.
 * @...: the NULL-terminated arguments of the properties and values.
 *
 * Creates a new #IBusUnicodeBlock.
 * block property is required. e.g.
 * ibus_unicode_block_new ("start", 0x0000, "end", "0x007f", "name", "basic",
 * NULL)
 *
 * Returns: A newly allocated #IBusUnicodeBlock.
 */
IBusUnicodeBlock *ibus_unicode_block_new      (const gchar *first_property_name,
                                               ...);

/**
 * ibus_unicode_block_get_start:
 * @block: An #IBusUnicodeData
 *
 * Gets the start code point in #IBusUnicodeBlock.
 *
 * Returns: start property in #IBusUnicodeBlock
 */
gunichar          ibus_unicode_block_get_start
                                              (IBusUnicodeBlock   *block);

/**
 * ibus_unicode_block_get_end:
 * @block: An #IBusUnicodeData
 *
 * Gets the end code point in #IBusUnicodeBlock.
 *
 * Returns: end property in #IBusUnicodeBlock
 */
gunichar          ibus_unicode_block_get_end
                                              (IBusUnicodeBlock   *block);

/**
 * ibus_unicode_block_get_name:
 * @block: An #IBusUnicodeBlock
 *
 * Gets the name in #IBusUnicodeBlock. It should not be freed.
 *
 * Returns: name property in #IBusUnicodeBlock
 */
const gchar *     ibus_unicode_block_get_name (IBusUnicodeBlock   *block);

/**
 * ibus_unicode_block_save:
 * @path: A path of the saved Unicode block.
 * @list: (element-type IBusUnicodeBlock) (transfer none): A list of unicode
 *  block.
 *
 * Save the list of #IBusUnicodeBlock to the cache file.
 */
void              ibus_unicode_block_save     (const gchar        *path,
                                               GSList             *list);

/**
 * ibus_unicode_block_load:
 * @path: A path of the saved dictionary file.
 *
 * Returns: (element-type IBusUnicodeBlock) (transfer full):
 * An #IBusUnicodeBlock list loaded from the saved cache file.
 */
GSList *          ibus_unicode_block_load     (const gchar        *path);

G_END_DECLS
#endif
