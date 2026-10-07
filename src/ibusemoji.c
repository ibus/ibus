/* -*- mode: C; c-basic-offset: 4; indent-tabs-mode: nil; -*- */
/* vim:set et sts=4: */
/* bus - The Input Bus
 * Copyright (C) 2017-2021 Takao Fujiwara <takao.fujiwara1@gmail.com>
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
#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <glib.h>
#include <glib/gstdio.h>
#include "ibusemoji.h"
#include "ibusinternal.h"

#define IBUS_EMOJI_DATA_MAGIC "IBusEmojiData"
#define IBUS_EMOJI_DATA_VERSION (5)

enum {
    PROP_0 = 0,
    PROP_EMOJI,
    PROP_ANNOTATIONS,
    PROP_DESCRIPTION,
    PROP_CATEGORY,
};

struct _IBusEmojiDataPrivate {
    gchar      *emoji;
    GSList     *annotations;
    gchar      *description;
    gchar      *category;
};

#define IBUS_EMOJI_DATA_GET_PRIVATE(o)  \
   ((IBusEmojiDataPrivate *)ibus_emoji_data_get_instance_private (o))

/* functions prototype */
static void      ibus_emoji_data_set_property  (IBusEmojiData       *emoji,
                                                guint                prop_id,
                                                const GValue        *value,
                                                GParamSpec          *pspec);
static void      ibus_emoji_data_get_property  (IBusEmojiData       *emoji,
                                                guint                prop_id,
                                                GValue              *value,
                                                GParamSpec          *pspec);
static void      ibus_emoji_data_destroy       (IBusEmojiData       *emoji);
static gboolean  ibus_emoji_data_serialize     (IBusEmojiData       *emoji,
                                                GVariantBuilder     *builder);
static gint      ibus_emoji_data_deserialize   (IBusEmojiData       *emoji,
                                                GVariant            *variant);
static gboolean  ibus_emoji_data_copy          (IBusEmojiData       *emoji,
                                                const IBusEmojiData *src);

G_DEFINE_TYPE_WITH_PRIVATE (IBusEmojiData,
                            ibus_emoji_data,
                            IBUS_TYPE_SERIALIZABLE)

static void
ibus_emoji_data_class_init (IBusEmojiDataClass *class)
{
    IBusObjectClass *object_class = IBUS_OBJECT_CLASS (class);
    GObjectClass *gobject_class = G_OBJECT_CLASS (class);
    IBusSerializableClass *serializable_class = IBUS_SERIALIZABLE_CLASS (class);

    object_class->destroy = (IBusObjectDestroyFunc) ibus_emoji_data_destroy;
    gobject_class->set_property =
            (GObjectSetPropertyFunc) ibus_emoji_data_set_property;
    gobject_class->get_property =
            (GObjectGetPropertyFunc) ibus_emoji_data_get_property;
    serializable_class->serialize   =
            (IBusSerializableSerializeFunc) ibus_emoji_data_serialize;
    serializable_class->deserialize =
            (IBusSerializableDeserializeFunc) ibus_emoji_data_deserialize;
    serializable_class->copy        =
            (IBusSerializableCopyFunc) ibus_emoji_data_copy;

    /* install properties */
    /**
     * IBusEmojiData:emoji:
     *
     * The emoji character
     */
    g_object_class_install_property (gobject_class,
                    PROP_EMOJI,
                    g_param_spec_string ("emoji",
                        "emoji character",
                        "The emoji character UTF-8",
                        NULL,
                        G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));

    /**
     * IBusEmojiData:annotations: (transfer container) (element-type utf8):
     *
     * The emoji annotations
     */
    g_object_class_install_property (gobject_class,
                    PROP_ANNOTATIONS,
                    g_param_spec_pointer ("annotations",
                        "emoji annotations",
                        "The emoji annotation list",
                        G_PARAM_READWRITE | G_PARAM_CONSTRUCT));

    /**
     * IBusEmojiData:description:
     *
     * The emoji description
     */
    g_object_class_install_property (gobject_class,
                    PROP_DESCRIPTION,
                    g_param_spec_string ("description",
                        "emoji description",
                        "The emoji description",
                        "",
                        G_PARAM_READWRITE | G_PARAM_CONSTRUCT));

    /**
     * IBusEmojiData:category:
     *
     * The emoji category
     */
    g_object_class_install_property (gobject_class,
                    PROP_CATEGORY,
                    g_param_spec_string ("category",
                        "emoji category",
                        "The emoji category",
                        "",
                        G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));
}

static void
ibus_emoji_data_init (IBusEmojiData *emoji)
{
    emoji->priv = IBUS_EMOJI_DATA_GET_PRIVATE (emoji);
}

static void
free_dict_words (gpointer list)
{
    g_slist_free_full (list, g_free);
}

static void
ibus_emoji_data_destroy (IBusEmojiData *emoji)
{
    g_clear_pointer (&emoji->priv->emoji, g_free);
    g_clear_pointer (&emoji->priv->annotations, free_dict_words);
    g_clear_pointer (&emoji->priv->description, g_free);
    g_clear_pointer (&emoji->priv->category, g_free);

    IBUS_OBJECT_CLASS (ibus_emoji_data_parent_class)->
            destroy (IBUS_OBJECT (emoji));
}

static void
ibus_emoji_data_set_property (IBusEmojiData *emoji,
                              guint          prop_id,
                              const GValue  *value,
                              GParamSpec    *pspec)
{
    switch (prop_id) {
    case PROP_EMOJI:
        g_assert (emoji->priv->emoji == NULL);
        emoji->priv->emoji = g_value_dup_string (value);
        break;
    case PROP_ANNOTATIONS:
        if (emoji->priv->annotations)
            g_slist_free_full (emoji->priv->annotations, g_free);
        emoji->priv->annotations =
                g_slist_copy_deep (g_value_get_pointer (value),
                                   (GCopyFunc) g_strdup, NULL);
        break;
    case PROP_DESCRIPTION:
        g_free (emoji->priv->description);
        emoji->priv->description = g_value_dup_string (value);
        break;
    case PROP_CATEGORY:
        g_assert (emoji->priv->category == NULL);
        emoji->priv->category = g_value_dup_string (value);
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (emoji, prop_id, pspec);
    }
}

static void
ibus_emoji_data_get_property (IBusEmojiData *emoji,
                              guint          prop_id,
                              GValue        *value,
                              GParamSpec    *pspec)
{
    switch (prop_id) {
    case PROP_EMOJI:
        g_value_set_string (value, ibus_emoji_data_get_emoji (emoji));
        break;
    case PROP_ANNOTATIONS:
        g_value_set_pointer (
                value,
                g_slist_copy_deep (ibus_emoji_data_get_annotations (emoji),
                                   (GCopyFunc) g_strdup, NULL));
        break;
    case PROP_DESCRIPTION:
        g_value_set_string (value, ibus_emoji_data_get_description (emoji));
        break;
    case PROP_CATEGORY:
        g_value_set_string (value, ibus_emoji_data_get_category (emoji));
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (emoji, prop_id, pspec);
    }
}

static gboolean
ibus_emoji_data_serialize (IBusEmojiData   *emoji,
                           GVariantBuilder *builder)
{
    GSList *l;
    gboolean retval = IBUS_SERIALIZABLE_CLASS (ibus_emoji_data_parent_class)->
            serialize ((IBusSerializable *)emoji, builder);
    g_return_val_if_fail (retval, FALSE);

#define NOTNULL(s) ((s) != NULL ? (s) : "")
    /* If you will add a new property, you can append it at the end and
     * you should not change the serialized order of name, longname,
     * description, ... because the order is also used in other applications
     * likes ibus-qt. */
    g_variant_builder_add (builder, "s", NOTNULL (emoji->priv->emoji));
    g_variant_builder_add (builder, "u",
                           g_slist_length (emoji->priv->annotations));
    for (l = emoji->priv->annotations; l != NULL; l = l->next) {
        g_variant_builder_add (builder, "s", NOTNULL (l->data));
    }
    g_variant_builder_add (builder, "s", NOTNULL (emoji->priv->description));
    g_variant_builder_add (builder, "s", NOTNULL (emoji->priv->category));
#undef NOTNULL
    return TRUE;
}

static gint
ibus_emoji_data_deserialize (IBusEmojiData *emoji,
                             GVariant      *variant)
{
    guint length, i;
    GSList *annotations = NULL;
    gint retval = IBUS_SERIALIZABLE_CLASS (ibus_emoji_data_parent_class)->
            deserialize ((IBusSerializable *)emoji, variant);
    g_return_val_if_fail (retval, 0);

    /* If you will add a new property, you can append it at the end and
     * you should not change the serialized order of name, longname,
     * description, ... because the order is also used in other applications
     * likes ibus-qt. */
    ibus_g_variant_get_child_string (variant, retval++,
                                     &emoji->priv->emoji);
    g_variant_get_child (variant, retval++, "u", &length);
    for (i = 0; i < length; i++) {
        gchar *s = NULL;
        g_variant_get_child (variant, retval++, "s", &s);
        annotations = g_slist_append (annotations, s);
    }
    emoji->priv->annotations = annotations;
    ibus_g_variant_get_child_string (variant, retval++,
                                     &emoji->priv->description);
    ibus_g_variant_get_child_string (variant, retval++,
                                     &emoji->priv->category);
    return retval;
}

static gboolean
ibus_emoji_data_copy (IBusEmojiData       *dest,
                      const IBusEmojiData *src)
{
    gboolean retval = IBUS_SERIALIZABLE_CLASS (ibus_emoji_data_parent_class)->
            copy ((IBusSerializable *)dest,
                  (IBusSerializable *)src);
    g_return_val_if_fail (retval, FALSE);

    dest->priv->emoji            = g_strdup (src->priv->emoji);
    dest->priv->annotations      = g_slist_copy_deep (src->priv->annotations,
                                                      (GCopyFunc) g_strdup,
                                                      NULL);
    dest->priv->description      = g_strdup (src->priv->description);
    dest->priv->category         = g_strdup (src->priv->category);
    return TRUE;
}

IBusEmojiData *
ibus_emoji_data_new (const gchar *first_property_name, ...)
{
    va_list var_args;
    IBusEmojiData *emoji;

    g_assert (first_property_name != NULL);
    va_start (var_args, first_property_name);
    emoji = (IBusEmojiData *) g_object_new_valist (IBUS_TYPE_EMOJI_DATA,
                                                   first_property_name,
                                                   var_args);
    va_end (var_args);
    /* emoji is required. Other properties are set in class_init by default. */
    g_assert (emoji->priv->emoji != NULL);
    g_assert (emoji->priv->description != NULL);
    g_assert (emoji->priv->category != NULL);
    return emoji;
}

const gchar *
ibus_emoji_data_get_emoji (IBusEmojiData *emoji)
{
    g_return_val_if_fail (IBUS_IS_EMOJI_DATA (emoji), NULL);

    return emoji->priv->emoji;
}

GSList *
ibus_emoji_data_get_annotations (IBusEmojiData *emoji)
{
    g_return_val_if_fail (IBUS_IS_EMOJI_DATA (emoji), NULL);

    return emoji->priv->annotations;
}

void
ibus_emoji_data_set_annotations (IBusEmojiData *emoji,
                                 GSList        *annotations)
{
    g_return_if_fail (IBUS_IS_EMOJI_DATA (emoji));

    if (emoji->priv->annotations)
        g_slist_free_full (emoji->priv->annotations, g_free);
    emoji->priv->annotations = annotations;
}

const gchar *
ibus_emoji_data_get_description (IBusEmojiData *emoji)
{
    g_return_val_if_fail (IBUS_IS_EMOJI_DATA (emoji), NULL);

    return emoji->priv->description;
}

void
ibus_emoji_data_set_description (IBusEmojiData *emoji,
                                 const gchar   *description)
{
    g_return_if_fail (IBUS_IS_EMOJI_DATA (emoji));

    g_free (emoji->priv->description);
    emoji->priv->description = g_strdup (description);
}

const gchar *
ibus_emoji_data_get_category (IBusEmojiData *emoji)
{
    g_return_val_if_fail (IBUS_IS_EMOJI_DATA (emoji), NULL);

    return emoji->priv->category;
}

static void
variant_foreach_add_emoji (IBusEmojiData   *emoji,
                           GVariantBuilder *builder)
{
    g_variant_builder_open (builder, G_VARIANT_TYPE_VARIANT);
    g_variant_builder_add_value (
            builder,
            ibus_serializable_serialize (IBUS_SERIALIZABLE (emoji)));
    g_variant_builder_close (builder);
}

static GVariant *
ibus_emoji_data_list_serialize (GSList *list)
{
    GVariantBuilder builder;

    g_variant_builder_init (&builder, G_VARIANT_TYPE ("av"));
    g_slist_foreach (list,  (GFunc) variant_foreach_add_emoji, &builder);
    return g_variant_builder_end (&builder);
}

static GSList *
ibus_emoji_data_list_deserialize (GVariant           *variant)
{
    GSList *list = NULL;
    GVariantIter iter;
    GVariant *emoji_variant = NULL;

    g_variant_iter_init (&iter, variant);
    while (g_variant_iter_loop (&iter, "v", &emoji_variant)) {
        IBusEmojiData *data =
                IBUS_EMOJI_DATA (ibus_serializable_deserialize (emoji_variant));
        list = g_slist_append (list, data);
        g_clear_pointer (&emoji_variant, g_variant_unref);
    }

    return list;
}

void
ibus_emoji_dict_save (const gchar *path,
                      GHashTable  *dict)
{
    GList *values, *v;
    GSList *list_for_save = NULL;

    g_return_if_fail (path != NULL);
    g_return_if_fail (dict != NULL);

    values = g_hash_table_get_values (dict);
    for (v = values; v; v = v->next) {
        IBusEmojiData *data = v->data;
        if (!IBUS_IS_EMOJI_DATA (data)) {
            g_warning ("Your dict format of { annotation char, emoji GSList "
                       "} is no longer supported.\n"
                       "{ emoji char, IBusEmojiData GSList } is expected.");
            return;
        }
        list_for_save = g_slist_append (list_for_save, data);
    }

    ibus_emoji_data_save (path, list_for_save);
}

GHashTable *
ibus_emoji_dict_load (const gchar *path)
{
    GSList *list = ibus_emoji_data_load (path);
    GSList *l;
    GHashTable *dict = g_hash_table_new_full (g_str_hash,
                                              g_str_equal,
                                              g_free,
                                              g_object_unref);

    for (l = list; l; l = l->next) {
        IBusEmojiData *data = l->data;
        if (!IBUS_IS_EMOJI_DATA (data)) {
            g_warning ("Your dict format is no longer supported.\n"
                       "Need to create the dictionaries again.");
            return NULL;
        }
        g_hash_table_insert (dict,
                             g_strdup (ibus_emoji_data_get_emoji (data)),
                             g_object_ref_sink (data));
    }

    g_slist_free (list);

    return dict;
}


IBusEmojiData *
ibus_emoji_dict_lookup (GHashTable  *dict,
                        const gchar *emoji)
{
    return (IBusEmojiData *) g_hash_table_lookup (dict, emoji);
}

void
ibus_emoji_data_save (const gchar *path,
                      GSList      *list)
{
    GVariant *variant;
    const gchar *header = IBUS_EMOJI_DATA_MAGIC;
    const guint16 version = IBUS_EMOJI_DATA_VERSION;
    const gchar *contents;
    gsize length;
    gchar *dir;
    GStatBuf buf = { 0, };
    GError *error = NULL;

    g_return_if_fail (path != NULL);
    g_return_if_fail (list != NULL);
    if (list->data == NULL) {
        g_warning ("Failed to save IBus emoji data: Need a list data.");
        return;
    }

    variant = g_variant_new ("(sqv)",
                             header,
                             version,
                             ibus_emoji_data_list_serialize (list));

    contents =  g_variant_get_data (variant);
    length =  g_variant_get_size (variant);

    dir = g_path_get_dirname (path);
    if (g_strcmp0 (dir, ".") != 0 && g_stat (dir, &buf) != 0) {
        errno = 0;
        if (g_mkdir_with_parents (dir, 0777)) {
            g_warning ("Failed mkdir %s: %s", dir, g_strerror (errno));
            return;
        }
    }
    g_free (dir);
    if (!g_file_set_contents (path, contents, length, &error)) {
        g_warning ("Failed to save emoji dict %s: %s", path, error->message);
        g_error_free (error);
    }

    g_variant_unref (variant);
}

GSList *
ibus_emoji_data_load (const gchar *path)
{
    gchar *contents = NULL;
    gsize length = 0;
    GError *error = NULL;
    GVariant *variant_table = NULL;
    GVariant *variant = NULL;
    const gchar *header = NULL;
    guint16 version = 0;
    GSList *retval = NULL;

    if (!g_file_test (path, G_FILE_TEST_EXISTS)) {
        g_warning ("Emoji dict does not exist: %s", path);
        goto out_load_cache;
    }

    if (!g_file_get_contents (path, &contents, &length, &error)) {
        g_warning ("Failed to get dict content %s: %s", path, error->message);
        g_error_free (error);
        goto out_load_cache;
    }

    variant_table = g_variant_new_from_data (G_VARIANT_TYPE ("(sq)"),
                                             contents,
                                             length,
                                             FALSE,
                                             NULL,
                                             NULL);

    if (variant_table == NULL) {
        g_warning ("cache table is broken.");
        goto out_load_cache;
    }

    g_variant_get (variant_table, "(&sq)", &header, &version);

    if (g_strcmp0 (header, IBUS_EMOJI_DATA_MAGIC) != 0) {
        g_warning ("cache is not IBusEmojiData.");
        goto out_load_cache;
    }

    if (version > IBUS_EMOJI_DATA_VERSION) {
        g_warning ("cache version is different: %u != %u",
                   version, IBUS_EMOJI_DATA_VERSION);
        goto out_load_cache;
    }

    version = 0;
    header = NULL;
    g_variant_unref (variant_table);

    variant_table = g_variant_new_from_data (G_VARIANT_TYPE ("(sqv)"),
                                             contents,
                                             length,
                                             FALSE,
                                             NULL,
                                             NULL);

    if (variant_table == NULL) {
        g_warning ("cache table is broken.");
        goto out_load_cache;
    }

    g_variant_get (variant_table, "(&sqv)",
                   NULL,
                   NULL,
                   &variant);

    if (variant == NULL) {
        g_warning ("cache dict is broken.");
        goto out_load_cache;
    }

    retval = ibus_emoji_data_list_deserialize (variant);

out_load_cache:
    if (variant)
        g_variant_unref (variant);
    if (variant_table)
        g_variant_unref (variant_table);
    g_free (contents);

    return retval;
}
