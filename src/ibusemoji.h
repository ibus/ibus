/* -*- mode: C; c-basic-offset: 4; indent-tabs-mode: nil; -*- */
/* vim:set et sts=4: */
/* bus - The Input Bus
 * Copyright (C) 2017-2019 Takao Fujiwara <takao.fujiwara1@gmail.com>
 * Copyright (C) 2017-2019 Red Hat, Inc.
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

#ifndef __IBUS_EMOJI_H_
#define __IBUS_EMOJI_H_

/**
 * SECTION: ibusemoji
 * @short_description: emoji utility.
 * @stability: Unstable
 *
 * miscellaneous emoji APIs.
 */

#include "ibusserializable.h"

/*
 * Type macros.
 */
/* define GOBJECT macros */
#define IBUS_TYPE_EMOJI_DATA         (ibus_emoji_data_get_type ())
#define IBUS_EMOJI_DATA(obj)         (G_TYPE_CHECK_INSTANCE_CAST ((obj), \
                                      IBUS_TYPE_EMOJI_DATA, IBusEmojiData))
#define IBUS_EMOJI_DATA_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST ((klass), \
                                      IBUS_TYPE_EMOJI_DATA, IBusEmojiDataClass))
#define IBUS_IS_EMOJI_DATA(obj)      (G_TYPE_CHECK_INSTANCE_TYPE ((obj), \
                                      IBUS_TYPE_EMOJI_DATA))


G_BEGIN_DECLS

typedef struct _IBusEmojiData IBusEmojiData;
typedef struct _IBusEmojiDataPrivate IBusEmojiDataPrivate;
typedef struct _IBusEmojiDataClass IBusEmojiDataClass;

/**
 * IBusEmojiData:
 *
 * Emoji data likes emoji unicode, annotations, description, category.
 * You can get extended values with g_object_get_properties.
 */
struct _IBusEmojiData {
    IBusSerializable parent;
    /* instance members */

    /*< public >*/
    /*< private >*/
    IBusEmojiDataPrivate *priv;
};

struct _IBusEmojiDataClass {
    IBusSerializableClass parent;
    /* class members */
};

GType           ibus_emoji_data_get_type     (void);

/**
 * ibus_emoji_data_new:
 * @first_property_name: Name of the first property.
 * @...: the NULL-terminated arguments of the properties and values.
 *
 * Creates a new #IBusEmojiData.
 * emoji property is required. e.g.
 * ibus_emoji_data_new ("emoji", "😁", NULL)
 *
 * Returns: A newly allocated #IBusEmojiData.
 */
IBusEmojiData * ibus_emoji_data_new          (const gchar *first_property_name,
                                               ...);

/**
 * ibus_emoji_data_get_emoji:
 * @emoji : An #IBusEmojiData
 *
 * Gets the emoji character in #IBusEmojiData. It should not be freed.
 *
 * Returns: emoji property in #IBusEmojiData
 */
const gchar *   ibus_emoji_data_get_emoji       (IBusEmojiData *emoji);

/**
 * ibus_emoji_data_get_annotations:
 * @emoji : An #IBusEmojiData
 *
 * Gets the annotation list in #IBusEmojiData. It should not be freed.
 *
 * Returns: (transfer none) (element-type utf8):
 *          annotation list property in #IBusEmojiData
 */
GSList *        ibus_emoji_data_get_annotations (IBusEmojiData *emoji);

/**
 * ibus_emoji_data_set_annotations:
 * @emoji : An #IBusEmojiData
 * @annotations: (transfer full) (element-type utf8): List of emoji annotations
 *
 * Sets the annotation list in #IBusEmojiData.
 */
void            ibus_emoji_data_set_annotations (IBusEmojiData *emoji,
                                                 GSList        *annotations);

/**
 * ibus_emoji_data_get_description:
 * @emoji : An #IBusEmojiData
 *
 * Gets the emoji description in #IBusEmojiData. It should not be freed.
 *
 * Returns: description property in #IBusEmojiData
 */
const gchar *   ibus_emoji_data_get_description (IBusEmojiData *emoji);

/**
 * ibus_emoji_data_set_description:
 * @emoji : An #IBusEmojiData
 * @description: An emoji description
 *
 * Sets the description in #IBusEmojiData.
 */
void            ibus_emoji_data_set_description (IBusEmojiData *emoji,
                                                 const gchar   *description);


/**
 * ibus_emoji_data_get_category:
 * @emoji : An #IBusEmojiData
 *
 * Gets the emoji category in #IBusEmojiData. It should not be freed.
 *
 * Returns: category property in #IBusEmojiData
 */
const gchar *   ibus_emoji_data_get_category    (IBusEmojiData *emoji);


/**
 * ibus_emoji_dict_save:
 * @path: A path of the saved dictionary file.
 * @dict: (element-type utf8 gpointer) (transfer none): An Emoji dictionary
 *
 * Saves the Emoji dictionary to the cache file.
 * Recommend to use ibus_emoji_data_save() instead becase GSList in
 * GHashTable does not work with Gir and Vala.
 * Calls ibus_emoji_data_save() internally. The format of the hash table
 * changed and now is { emoji character, #IBusEmojiData object }.
 */
void            ibus_emoji_dict_save            (const gchar    *path,
                                                 GHashTable     *dict);
/**
 * ibus_emoji_dict_load:
 * @path: A path of the saved dictionary file.
 *
 * Returns: (element-type utf8 gpointer) (transfer none): An Emoji dictionary
 * file loaded from the saved cache file.
 *
 * A hash table of { emoji character, #IBusEmojiData object } is loaded
 * from the saved cache file.
 * Recommend to use ibus_emoji_data_load() instead becase GSList in
 * GHashTable does not work with Gir and Vala.
 * Calls ibus_emoji_data_load() internally.
 */
GHashTable *    ibus_emoji_dict_load            (const gchar    *path);

/**
 * ibus_emoji_dict_lookup:
 * @dict: (element-type utf8 IBusEmojiData) (transfer full): An Emoji dictionary
 * @emoji: an emoji character
 *
 * Returns: (transfer none): An #IBusEmojiData of @emoji.
 * This API was prepared for the old dict foramat with Gir and Vala
 * but no longer needed.
 * Use ibus_emoji_data_load() instead.
 */
IBusEmojiData * ibus_emoji_dict_lookup          (GHashTable     *dict,
                                                 const gchar    *emoji);
/**
 * ibus_emoji_data_save:
 * @path: A path of the saved emoji data.
 * @list: (element-type IBusEmojiData) (transfer none): A list of emoji data.
 *
 * Save the list of #IBusEmojiData to the cache file.
 */
void            ibus_emoji_data_save            (const gchar    *path,
                                                 GSList         *list);

/**
 * ibus_emoji_data_load:
 * @path: A path of the saved dictionary file.
 *
 * Returns: (element-type IBusEmojiData) (transfer full):
 * An #IBusEmojiData list loaded from the saved cache file.
 */
GSList *        ibus_emoji_data_load            (const gchar    *path);

G_END_DECLS
#endif
