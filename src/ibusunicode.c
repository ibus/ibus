/* -*- mode: C; c-basic-offset: 4; indent-tabs-mode: nil; -*- */
/* vim:set et sts=4: */
/* bus - The Input Bus
 * Copyright (C) 2018-2021 Takao Fujiwara <takao.fujiwara1@gmail.com>
 * Copyright (C) 2018-2021 Red Hat, Inc.
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
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301
 * USA
 */
#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <glib.h>
#include <glib/gstdio.h>
#include "ibusinternal.h"
#include "ibuserror.h"
#include "ibusunicode.h"

#define IBUS_UNICODE_DATA_MAGIC "IBusUnicodeData"
#define IBUS_UNICODE_BLOCK_MAGIC "IBusUnicodeBlock"
#define IBUS_UNICODE_DATA_VERSION (1)
#define IBUS_UNICODE_DESERIALIZE_SIGNALL_STR \
        "deserialize-unicode"

enum {
    PROP_0 = 0,
    PROP_CODE,
    PROP_NAME,
    PROP_ALIAS,
    PROP_BLOCK_NAME,
    PROP_START,
    PROP_END
};

struct _IBusUnicodeDataPrivate {
    gunichar    code;
    gchar      *name;
    gchar      *alias;
    gchar      *block_name;
};

struct _IBusUnicodeBlockPrivate {
    gunichar    start;
    gunichar    end;
    gchar      *name;
};

typedef struct {
    IBusUnicodeDataLoadAsyncFinish callback;
    gpointer                       user_data;
} IBusUnicodeDataLoadData;

#define IBUS_UNICODE_DATA_GET_PRIVATE(o)  \
   ((IBusUnicodeDataPrivate *)ibus_unicode_data_get_instance_private (o))
#define IBUS_UNICODE_BLOCK_GET_PRIVATE(o)  \
   ((IBusUnicodeBlockPrivate *)ibus_unicode_block_get_instance_private (o))

/* functions prototype */
static void      ibus_unicode_data_set_property (IBusUnicodeData      *unicode,
                                                 guint                 prop_id,
                                                 const GValue         *value,
                                                 GParamSpec           *pspec);
static void      ibus_unicode_data_get_property (IBusUnicodeData      *unicode,
                                                 guint                 prop_id,
                                                 GValue               *value,
                                                 GParamSpec           *pspec);
static void      ibus_unicode_data_destroy      (IBusUnicodeData      *unicode);
static gboolean  ibus_unicode_data_serialize    (IBusUnicodeData      *unicode,
                                                 GVariantBuilder      *builder);
static gint      ibus_unicode_data_deserialize  (IBusUnicodeData      *unicode,
                                                 GVariant             *variant);
static gboolean  ibus_unicode_data_copy         (IBusUnicodeData      *dest,
                                                const IBusUnicodeData *src);
static void      ibus_unicode_block_set_property
                                                (IBusUnicodeBlock     *block,
                                                 guint                 prop_id,
                                                 const GValue         *value,
                                                 GParamSpec           *pspec);
static void      ibus_unicode_block_get_property
                                                (IBusUnicodeBlock     *block,
                                                 guint                 prop_id,
                                                 GValue               *value,
                                                 GParamSpec           *pspec);
static void      ibus_unicode_block_destroy     (IBusUnicodeBlock     *block);
static gboolean  ibus_unicode_block_serialize   (IBusUnicodeBlock     *block,
                                                 GVariantBuilder      *builder);
static gint      ibus_unicode_block_deserialize (IBusUnicodeBlock     *block,
                                                 GVariant             *variant);
static gboolean  ibus_unicode_block_copy        (IBusUnicodeBlock     *dest,
                                                const IBusUnicodeBlock *src);

G_DEFINE_TYPE_WITH_PRIVATE (IBusUnicodeData,
                            ibus_unicode_data,
                            IBUS_TYPE_SERIALIZABLE)
G_DEFINE_TYPE_WITH_PRIVATE (IBusUnicodeBlock,
                            ibus_unicode_block,
                            IBUS_TYPE_SERIALIZABLE)

static void
ibus_unicode_data_class_init (IBusUnicodeDataClass *class)
{
    IBusObjectClass *object_class = IBUS_OBJECT_CLASS (class);
    GObjectClass *gobject_class = G_OBJECT_CLASS (class);
    IBusSerializableClass *serializable_class = IBUS_SERIALIZABLE_CLASS (class);

    object_class->destroy = (IBusObjectDestroyFunc) ibus_unicode_data_destroy;
    gobject_class->set_property =
            (GObjectSetPropertyFunc) ibus_unicode_data_set_property;
    gobject_class->get_property =
            (GObjectGetPropertyFunc) ibus_unicode_data_get_property;
    serializable_class->serialize   =
            (IBusSerializableSerializeFunc) ibus_unicode_data_serialize;
    serializable_class->deserialize =
            (IBusSerializableDeserializeFunc) ibus_unicode_data_deserialize;
    serializable_class->copy        =
            (IBusSerializableCopyFunc) ibus_unicode_data_copy;

    /* install properties */
    /**
     * IBusUnicodeData:code:
     *
     * The Uniode code point
     */
    g_object_class_install_property (gobject_class,
                    PROP_CODE,
                    g_param_spec_unichar ("code",
                        "code point",
                        "The Unicode code point",
                        0,
                        G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));


    /**
     * IBusUnicodeData:name:
     *
     * The Uniode name
     */
    g_object_class_install_property (gobject_class,
                    PROP_NAME,
                    g_param_spec_string ("name",
                        "name",
                        "The Unicode name",
                        "",
                        G_PARAM_READWRITE | G_PARAM_CONSTRUCT));

    /**
     * IBusUnicodeData:alias:
     *
     * The Uniode alias name
     */
    g_object_class_install_property (gobject_class,
                    PROP_ALIAS,
                    g_param_spec_string ("alias",
                        "alias name",
                        "The Unicode alias name",
                        "",
                        G_PARAM_READWRITE | G_PARAM_CONSTRUCT));

    /**
     * IBusUnicodeData:block-name:
     *
     * The Uniode block name
     */
    g_object_class_install_property (gobject_class,
                    PROP_BLOCK_NAME,
                    g_param_spec_string ("block-name",
                        "block name",
                        "The Unicode block name",
                        "",
                        G_PARAM_READWRITE | G_PARAM_CONSTRUCT));
}

static void
ibus_unicode_data_init (IBusUnicodeData *unicode)
{
    unicode->priv = IBUS_UNICODE_DATA_GET_PRIVATE (unicode);
}

static void
ibus_unicode_data_destroy (IBusUnicodeData *unicode)
{
    g_clear_pointer (&unicode->priv->name, g_free);
    g_clear_pointer (&unicode->priv->alias, g_free);
    g_clear_pointer (&unicode->priv->block_name, g_free);

    IBUS_OBJECT_CLASS (ibus_unicode_data_parent_class)->
            destroy (IBUS_OBJECT (unicode));
}

static void
ibus_unicode_data_set_property (IBusUnicodeData *unicode,
                                guint            prop_id,
                                const GValue    *value,
                                GParamSpec      *pspec)
{
    switch (prop_id) {
    case PROP_CODE:
        g_assert (unicode->priv->code == 0);
        unicode->priv->code = g_value_get_uint (value);
        break;
    case PROP_NAME:
        g_assert (unicode->priv->name == NULL);
        unicode->priv->name = g_value_dup_string (value);
        break;
    case PROP_ALIAS:
        g_assert (unicode->priv->alias == NULL);
        unicode->priv->alias = g_value_dup_string (value);
        break;
    case PROP_BLOCK_NAME:
        g_free (unicode->priv->block_name);
        unicode->priv->block_name = g_value_dup_string (value);
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (unicode, prop_id, pspec);
    }
}

static void
ibus_unicode_data_get_property (IBusUnicodeData *unicode,
                                guint            prop_id,
                                GValue          *value,
                                GParamSpec      *pspec)
{
    switch (prop_id) {
    case PROP_CODE:
        g_value_set_uint (value, ibus_unicode_data_get_code (unicode));
        break;
    case PROP_NAME:
        g_value_set_string (value, ibus_unicode_data_get_name (unicode));
        break;
    case PROP_ALIAS:
        g_value_set_string (value, ibus_unicode_data_get_alias (unicode));
        break;
    case PROP_BLOCK_NAME:
        g_value_set_string (value, ibus_unicode_data_get_block_name (unicode));
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (unicode, prop_id, pspec);
    }
}

static gboolean
ibus_unicode_data_serialize (IBusUnicodeData   *unicode,
                             GVariantBuilder   *builder)
{
    gboolean retval = IBUS_SERIALIZABLE_CLASS (ibus_unicode_data_parent_class)->
            serialize ((IBusSerializable *)unicode, builder);
    g_return_val_if_fail (retval, FALSE);

#define NOTNULL(s) ((s) != NULL ? (s) : "")
    /* If you will add a new property, you can append it at the end and
     * you should not change the serialized order of name, longname,
     * description, ... because the order is also used in other applications
     * likes ibus-qt. */
    g_variant_builder_add (builder, "u", unicode->priv->code);
    g_variant_builder_add (builder, "s", NOTNULL (unicode->priv->name));
    g_variant_builder_add (builder, "s", NOTNULL (unicode->priv->alias));
    /* Use IBusUnicodeBlock for memory usage.
    g_variant_builder_add (builder, "s", NOTNULL (unicode->priv->block_name));
     */
#undef NOTNULL
    return TRUE;
}

static gint
ibus_unicode_data_deserialize (IBusUnicodeData *unicode,
                               GVariant        *variant)
{
    gint retval = IBUS_SERIALIZABLE_CLASS (ibus_unicode_data_parent_class)->
            deserialize ((IBusSerializable *)unicode, variant);
    g_return_val_if_fail (retval, 0);

    /* If you will add a new property, you can append it at the end and
     * you should not change the serialized order of name, longname,
     * description, ... because the order is also used in other applications
     * likes ibus-qt. */
    g_variant_get_child (variant, retval++, "u", &unicode->priv->code);
    ibus_g_variant_get_child_string (variant, retval++,
                                     &unicode->priv->name);
    ibus_g_variant_get_child_string (variant, retval++,
                                     &unicode->priv->alias);
    /* Use IBusUnicodeBlock for memory usage.
    ibus_g_variant_get_child_string (variant, retval++,
                                     &unicode->priv->block_name);
     */
    return retval;
}

static gboolean
ibus_unicode_data_copy (IBusUnicodeData       *dest,
                        const IBusUnicodeData *src)
{
    gboolean retval = IBUS_SERIALIZABLE_CLASS (ibus_unicode_data_parent_class)->
            copy ((IBusSerializable *)dest,
                  (IBusSerializable *)src);
    g_return_val_if_fail (retval, FALSE);

    dest->priv->code             = src->priv->code;
    dest->priv->name             = g_strdup (src->priv->name);
    dest->priv->alias            = g_strdup (src->priv->alias);
    dest->priv->block_name       = g_strdup (src->priv->block_name);
    return TRUE;
}

IBusUnicodeData *
ibus_unicode_data_new (const gchar *first_property_name, ...)
{
    va_list var_args;
    IBusUnicodeData *unicode;

    g_assert (first_property_name != NULL);
    va_start (var_args, first_property_name);
    unicode = (IBusUnicodeData *) g_object_new_valist (IBUS_TYPE_UNICODE_DATA,
                                                       first_property_name,
                                                       var_args);
    va_end (var_args);
    /* code is required. Other properties are set in class_init by default. */
    g_assert (unicode->priv->name != NULL);
    g_assert (unicode->priv->alias != NULL);
    g_assert (unicode->priv->block_name != NULL);
    return unicode;
}

gunichar
ibus_unicode_data_get_code (IBusUnicodeData *unicode)
{
    g_return_val_if_fail (IBUS_IS_UNICODE_DATA (unicode), G_MAXUINT32);

    return unicode->priv->code;
}

const gchar *
ibus_unicode_data_get_name (IBusUnicodeData *unicode)
{
    g_return_val_if_fail (IBUS_IS_UNICODE_DATA (unicode), "");

    return unicode->priv->name;
}

const gchar *
ibus_unicode_data_get_alias (IBusUnicodeData *unicode)
{
    g_return_val_if_fail (IBUS_IS_UNICODE_DATA (unicode), "");

    return unicode->priv->alias;
}

const gchar *
ibus_unicode_data_get_block_name (IBusUnicodeData *unicode)
{
    g_return_val_if_fail (IBUS_IS_UNICODE_DATA (unicode), "");

    return unicode->priv->block_name;
}

void
ibus_unicode_data_set_block_name (IBusUnicodeData *unicode,
                                  const gchar     *block_name)
{
    g_return_if_fail (IBUS_IS_UNICODE_DATA (unicode));

    g_free (unicode->priv->block_name);
    unicode->priv->block_name = g_strdup (block_name);
}

static void
variant_foreach_add_unicode (IBusUnicodeData *unicode,
                             GVariantBuilder *builder)
{
    g_variant_builder_add (
            builder, "v",
            ibus_serializable_serialize (IBUS_SERIALIZABLE (unicode)));
}

static GVariant *
ibus_unicode_data_list_serialize (GSList *list)
{
    GVariantBuilder builder;

    g_variant_builder_init (&builder, G_VARIANT_TYPE ("av"));
    g_slist_foreach (list,  (GFunc) variant_foreach_add_unicode, &builder);
    return g_variant_builder_end (&builder);
}

static GSList *
ibus_unicode_data_list_deserialize (GVariant *variant,
                                    GObject  *source_object)
{
    GSList *list = NULL;
    GVariantIter iter;
    GVariant *unicode_variant = NULL;
    gsize i, size;
    gboolean has_signal = FALSE;

    if (G_IS_OBJECT (source_object)) {
        has_signal = g_signal_lookup (
                IBUS_UNICODE_DESERIALIZE_SIGNALL_STR,
                G_OBJECT_TYPE (source_object));
        if (!has_signal) {
            const gchar *type_name =
                    g_type_name (G_OBJECT_TYPE (source_object));
            g_warning ("GObject %s does not have the signal \"%s\"",
                       type_name ? type_name : "(null)",
                       IBUS_UNICODE_DESERIALIZE_SIGNALL_STR);
        }
    }
    g_variant_iter_init (&iter, variant);
    size = g_variant_iter_n_children (&iter);
    i = 0;
    while (g_variant_iter_loop (&iter, "v", &unicode_variant)) {
        IBusUnicodeData *data =
                IBUS_UNICODE_DATA (ibus_serializable_deserialize (
                        unicode_variant));
        list = g_slist_append (list, data);
        g_clear_pointer (&unicode_variant, g_variant_unref);
        if (has_signal && (i == 0 || ((i + 1) % 100) == 0)) {
            g_signal_emit_by_name (source_object,
                                   IBUS_UNICODE_DESERIALIZE_SIGNALL_STR,
                                   i + 1, size);
        }
        i++;
    }
    if (has_signal && (i != 1 && (i % 100) != 0)) {
        g_signal_emit_by_name (source_object,
                               IBUS_UNICODE_DESERIALIZE_SIGNALL_STR,
                               i, size);
    }

    return list;
}

void
ibus_unicode_data_save (const gchar *path,
                        GSList      *list)
{
    GVariant *variant;
    const gchar *header = IBUS_UNICODE_DATA_MAGIC;
    const guint16 version = IBUS_UNICODE_DATA_VERSION;
    const gchar *contents;
    gsize length;
    gchar *dir;
    GStatBuf buf = { 0, };
    GError *error = NULL;

    g_return_if_fail (path != NULL);
    g_return_if_fail (list != NULL);
    if (list->data == NULL) {
        g_warning ("Failed to save IBus Unicode data: Need a list data.");
        return;
    }

    variant = g_variant_new ("(sqv)",
                             header,
                             version,
                             ibus_unicode_data_list_serialize (list));

    contents =  g_variant_get_data (variant);
    length =  g_variant_get_size (variant);

    dir = g_path_get_dirname (path);
    if (g_strcmp0 (dir, ".") != 0 && g_stat (dir, &buf) != 0) {
        errno = 0;
        if (g_mkdir_with_parents (dir, 0777)) {
            g_warning ("Failed to mkdir %s: %s", dir, g_strerror (errno));
            return;
        }

    }
    g_free (dir);
    if (!g_file_set_contents (path, contents, length, &error)) {
        g_warning ("Failed to save Unicode dict %s: %s", path, error->message);
        g_error_free (error);
    }

    g_variant_unref (variant);
}

static GSList *
ibus_unicode_data_load_with_error (const gchar *path,
                                   GObject     *source_object,
                                   GError     **error)
{
    gchar *contents = NULL;
    gsize length = 0;
    GVariant *variant_table = NULL;
    GVariant *variant = NULL;
    const gchar *header = NULL;
    guint16 version = 0;
    GSList *retval = NULL;

    if (!g_file_test (path, G_FILE_TEST_EXISTS)) {
        g_set_error (error,
                     IBUS_ERROR,
                     IBUS_ERROR_FAILED,
                     "Unicode dict does not exist: %s", path);
        goto out_load_cache;
    }

    if (!g_file_get_contents (path, &contents, &length, error)) {
        goto out_load_cache;
    }

    variant_table = g_variant_new_from_data (G_VARIANT_TYPE ("(sq)"),
                                             contents,
                                             length,
                                             FALSE,
                                             NULL,
                                             NULL);

    if (variant_table == NULL) {
        g_set_error (error,
                     IBUS_ERROR,
                     IBUS_ERROR_FAILED,
                     "cache table is broken.");
        goto out_load_cache;
    }

    g_variant_get (variant_table, "(&sq)", &header, &version);

    if (g_strcmp0 (header, IBUS_UNICODE_DATA_MAGIC) != 0) {
        g_set_error (error,
                     IBUS_ERROR,
                     IBUS_ERROR_FAILED,
                     "cache is not IBusUnicodeData.");
        goto out_load_cache;
    }

    if (version > IBUS_UNICODE_DATA_VERSION) {
        g_set_error (error,
                     IBUS_ERROR,
                     IBUS_ERROR_FAILED,
                     "cache version is different: %u != %u",
                     version, IBUS_UNICODE_DATA_VERSION);
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
        g_set_error (error,
                     IBUS_ERROR,
                     IBUS_ERROR_FAILED,
                     "cache table is broken.");
        goto out_load_cache;
    }

    g_variant_get (variant_table, "(&sqv)",
                   NULL,
                   NULL,
                   &variant);

    if (variant == NULL) {
        g_set_error (error,
                     IBUS_ERROR,
                     IBUS_ERROR_FAILED,
                     "cache dict is broken.");
        goto out_load_cache;
    }

    retval = ibus_unicode_data_list_deserialize (variant, source_object);

out_load_cache:
    if (variant)
        g_variant_unref (variant);
    if (variant_table)
        g_variant_unref (variant_table);
    g_free (contents);

    return retval;
}

GSList *
ibus_unicode_data_load (const gchar *path,
                        GObject     *source_object)
{
    GError *error = NULL;
    GSList *retval = ibus_unicode_data_load_with_error (path,
                                                        source_object,
                                                        &error);

    if (retval == NULL) {
        g_warning ("%s", error->message);
        g_error_free (error);
    }

    return retval;
}

static void
ibus_unicode_data_load_async_thread (GTask        *task,
                                     gpointer      source_object,
                                     gpointer      task_data,
                                     GCancellable *cancellable)
{
    GSList *retval;
    gchar *path = (gchar *)task_data;
    GError *error = NULL;

    g_assert (path != NULL);

    retval = ibus_unicode_data_load_with_error (path, source_object, &error);
    g_free (path);
    if (retval == NULL)
        g_task_return_error (task, error);
    else
        g_task_return_pointer (task, retval, NULL);
    g_object_unref (task);
}

static void
ibus_unicode_data_load_async_done (GObject *source_object,
                                   GAsyncResult *res,
                                   gpointer user_data)
{
    IBusUnicodeDataLoadData *data = (IBusUnicodeDataLoadData*)user_data;
    GSList *list;
    GError *error = NULL;
    g_assert (data != NULL);
    list = g_task_propagate_pointer (G_TASK (res), &error);
    if (error) {
        g_warning ("%s", error->message);
        g_error_free (error);
        data->callback (NULL, data->user_data);
    } else {
        data->callback (list, data->user_data);
    }
    g_slice_free (IBusUnicodeDataLoadData, data);
}

void
ibus_unicode_data_load_async (const gchar        *path,
                              GObject            *source_object,
                              GCancellable       *cancellable,
                              IBusUnicodeDataLoadAsyncFinish
                                                  callback,
                              gpointer            user_data)
{
    GTask *task;
    IBusUnicodeDataLoadData *data;

    g_return_if_fail (path != NULL);

    data = g_slice_new0 (IBusUnicodeDataLoadData);
    data->callback = callback;
    data->user_data = user_data;
    task = g_task_new (source_object,
                       cancellable,
                       ibus_unicode_data_load_async_done,
                       data);
    g_task_set_source_tag (task, ibus_unicode_data_load_async);
    g_task_set_task_data (task, g_strdup (path), NULL);
    g_task_run_in_thread (task, ibus_unicode_data_load_async_thread);
}

static void
ibus_unicode_block_class_init (IBusUnicodeBlockClass *class)
{
    IBusObjectClass *object_class = IBUS_OBJECT_CLASS (class);
    GObjectClass *gobject_class = G_OBJECT_CLASS (class);
    IBusSerializableClass *serializable_class = IBUS_SERIALIZABLE_CLASS (class);

    object_class->destroy = (IBusObjectDestroyFunc) ibus_unicode_block_destroy;
    gobject_class->set_property =
            (GObjectSetPropertyFunc) ibus_unicode_block_set_property;
    gobject_class->get_property =
            (GObjectGetPropertyFunc) ibus_unicode_block_get_property;
    serializable_class->serialize   =
            (IBusSerializableSerializeFunc) ibus_unicode_block_serialize;
    serializable_class->deserialize =
            (IBusSerializableDeserializeFunc) ibus_unicode_block_deserialize;
    serializable_class->copy        =
            (IBusSerializableCopyFunc) ibus_unicode_block_copy;

    /* install properties */
    /**
     * IBusUnicodeBlock:start:
     *
     * The Uniode start code point
     */
    g_object_class_install_property (gobject_class,
                    PROP_START,
                    /* Cannot use g_param_spec_unichar() for the Unicode
                     * boundary values because the function checks
                     * if the value is a valid Unicode besides MAXUINT.
                     */
                    g_param_spec_uint ("start",
                        "start code point",
                        "The Unicode start code point",
                        0,
                        G_MAXUINT,
                        0,
                        G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));


    /**
     * IBusUnicodeBlock:end:
     *
     * The Uniode end code point
     */
    g_object_class_install_property (gobject_class,
                    PROP_END,
                    /* Cannot use g_param_spec_unichar() for the Unicode
                     * boundary values because the function checks
                     * if the value is a valid Unicode besides MAXUINT.
                     */
                    g_param_spec_uint ("end",
                        "end code point",
                        "The Unicode end code point",
                        0,
                        G_MAXUINT,
                        0,
                        G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));


    /**
     * IBusUnicodeBlock:name:
     *
     * The Uniode block name
     */
    g_object_class_install_property (gobject_class,
                    PROP_NAME,
                    g_param_spec_string ("name",
                        "name",
                        "The Unicode name",
                        "",
                        G_PARAM_READWRITE | G_PARAM_CONSTRUCT));
}

static void
ibus_unicode_block_init (IBusUnicodeBlock *block)
{
    block->priv = IBUS_UNICODE_BLOCK_GET_PRIVATE (block);
}

static void
ibus_unicode_block_destroy (IBusUnicodeBlock *block)
{
    g_clear_pointer (&block->priv->name, g_free);

    IBUS_OBJECT_CLASS (ibus_unicode_block_parent_class)->
            destroy (IBUS_OBJECT (block));
}

static void
ibus_unicode_block_set_property (IBusUnicodeBlock *block,
                                 guint             prop_id,
                                 const GValue     *value,
                                 GParamSpec       *pspec)
{
    switch (prop_id) {
    case PROP_START:
        g_assert (block->priv->start == 0);
        block->priv->start = g_value_get_uint (value);
        break;
    case PROP_END:
        g_assert (block->priv->end == 0);
        block->priv->end = g_value_get_uint (value);
        break;
    case PROP_NAME:
        g_assert (block->priv->name == NULL);
        block->priv->name = g_value_dup_string (value);
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (block, prop_id, pspec);
    }
}

static void
ibus_unicode_block_get_property (IBusUnicodeBlock *block,
                                 guint             prop_id,
                                 GValue           *value,
                                 GParamSpec       *pspec)
{
    switch (prop_id) {
    case PROP_START:
        g_value_set_uint (value, ibus_unicode_block_get_start (block));
        break;
    case PROP_END:
        g_value_set_uint (value, ibus_unicode_block_get_end (block));
        break;
    case PROP_NAME:
        g_value_set_string (value, ibus_unicode_block_get_name (block));
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (block, prop_id, pspec);
    }
}

static gboolean
ibus_unicode_block_serialize (IBusUnicodeBlock *block,
                              GVariantBuilder  *builder)
{
    gboolean retval = IBUS_SERIALIZABLE_CLASS (ibus_unicode_block_parent_class)->
            serialize ((IBusSerializable *)block, builder);
    g_return_val_if_fail (retval, FALSE);

#define NOTNULL(s) ((s) != NULL ? (s) : "")
    /* If you will add a new property, you can append it at the end and
     * you should not change the serialized order of name, longname,
     * description, ... because the order is also used in other applications
     * likes ibus-qt. */
    g_variant_builder_add (builder, "u", block->priv->start);
    g_variant_builder_add (builder, "u", block->priv->end);
    g_variant_builder_add (builder, "s", NOTNULL (block->priv->name));
#undef NOTNULL
    return TRUE;
}

static gint
ibus_unicode_block_deserialize (IBusUnicodeBlock *block,
                                GVariant        *variant)
{
    gint retval = IBUS_SERIALIZABLE_CLASS (ibus_unicode_block_parent_class)->
            deserialize ((IBusSerializable *)block, variant);
    g_return_val_if_fail (retval, 0);

    /* If you will add a new property, you can append it at the end and
     * you should not change the serialized order of name, longname,
     * description, ... because the order is also used in other applications
     * likes ibus-qt. */
    g_variant_get_child (variant, retval++, "u", &block->priv->start);
    g_variant_get_child (variant, retval++, "u", &block->priv->end);
    ibus_g_variant_get_child_string (variant, retval++,
                                     &block->priv->name);
    return retval;
}

static gboolean
ibus_unicode_block_copy (IBusUnicodeBlock       *dest,
                         const IBusUnicodeBlock *src)
{
    gboolean retval = IBUS_SERIALIZABLE_CLASS (ibus_unicode_block_parent_class)->
            copy ((IBusSerializable *)dest,
                  (IBusSerializable *)src);
    g_return_val_if_fail (retval, FALSE);

    dest->priv->start            = src->priv->start;
    dest->priv->end              = src->priv->end;
    dest->priv->name             = g_strdup (src->priv->name);
    return TRUE;
}

IBusUnicodeBlock *
ibus_unicode_block_new (const gchar *first_property_name, ...)
{
    va_list var_args;
    IBusUnicodeBlock *block;

    g_assert (first_property_name != NULL);
    va_start (var_args, first_property_name);
    block = (IBusUnicodeBlock *) g_object_new_valist (IBUS_TYPE_UNICODE_BLOCK,
                                                     first_property_name,
                                                     var_args);
    va_end (var_args);
    /* end is required. Other properties are set in class_init by default. */
    g_assert (block->priv->start != block->priv->end);
    g_assert (block->priv->name != NULL);
    return block;
}

gunichar
ibus_unicode_block_get_start (IBusUnicodeBlock *block)
{
    g_return_val_if_fail (IBUS_IS_UNICODE_BLOCK (block), G_MAXUINT32);

    return block->priv->start;
}

gunichar
ibus_unicode_block_get_end (IBusUnicodeBlock *block)
{
    g_return_val_if_fail (IBUS_IS_UNICODE_BLOCK (block), G_MAXUINT32);

    return block->priv->end;
}

const gchar *
ibus_unicode_block_get_name (IBusUnicodeBlock *block)
{
    g_return_val_if_fail (IBUS_IS_UNICODE_BLOCK (block), "");

    return block->priv->name;
}

static void
variant_foreach_add_block (IBusUnicodeBlock *block,
                           GVariantBuilder *builder)
{
    g_variant_builder_add (
            builder, "v",
            ibus_serializable_serialize (IBUS_SERIALIZABLE (block)));
}

static GVariant *
ibus_unicode_block_list_serialize (GSList *list)
{
    GVariantBuilder builder;

    g_variant_builder_init (&builder, G_VARIANT_TYPE ("av"));
    g_slist_foreach (list,  (GFunc) variant_foreach_add_block, &builder);
    return g_variant_builder_end (&builder);
}

static GSList *
ibus_unicode_block_list_deserialize (GVariant *variant)
{
    GSList *list = NULL;
    GVariantIter iter;
    GVariant *unicode_variant = NULL;

    g_variant_iter_init (&iter, variant);
    while (g_variant_iter_loop (&iter, "v", &unicode_variant)) {
        IBusUnicodeBlock *data =
                IBUS_UNICODE_BLOCK (ibus_serializable_deserialize (
                        unicode_variant));
        list = g_slist_append (list, data);
        g_clear_pointer (&unicode_variant, g_variant_unref);
    }

    return list;
}

void
ibus_unicode_block_save (const gchar *path,
                         GSList      *list)
{
    GVariant *variant;
    const gchar *header = IBUS_UNICODE_BLOCK_MAGIC;
    const guint16 version = IBUS_UNICODE_DATA_VERSION;
    const gchar *contents;
    gsize length;
    gchar *dir;
    GStatBuf buf = { 0, };
    GError *error = NULL;

    g_return_if_fail (path != NULL);
    g_return_if_fail (list != NULL);
    if (list->data == NULL) {
        g_warning ("Failed to save IBus Unicode block: Need a list data.");
        return;
    }

    variant = g_variant_new ("(sqv)",
                             header,
                             version,
                             ibus_unicode_block_list_serialize (list));

    contents =  g_variant_get_data (variant);
    length =  g_variant_get_size (variant);

    dir = g_path_get_dirname (path);
    if (g_strcmp0 (dir, ".") != 0 && g_stat (dir, &buf) != 0) {
        errno = 0;
        if (g_mkdir_with_parents (dir, 0777)) {
            g_warning ("Failed to mkdir %s: %s", dir, g_strerror (errno));
            return;
        }
    }
    g_free (dir);
    if (!g_file_set_contents (path, contents, length, &error)) {
        g_warning ("Failed to save Unicode dict %s: %s", path, error->message);
        g_error_free (error);
    }

    g_variant_unref (variant);
}

GSList *
ibus_unicode_block_load (const gchar *path)
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
        g_warning ("Unicode dict does not exist: %s", path);
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

    if (g_strcmp0 (header, IBUS_UNICODE_BLOCK_MAGIC) != 0) {
        g_warning ("cache is not IBusUnicodeBlock.");
        goto out_load_cache;
    }

    if (version > IBUS_UNICODE_DATA_VERSION) {
        g_warning ("cache version is different: %u != %u",
                   version, IBUS_UNICODE_DATA_VERSION);
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

    retval = ibus_unicode_block_list_deserialize (variant);

out_load_cache:
    if (variant)
        g_variant_unref (variant);
    if (variant_table)
        g_variant_unref (variant_table);
    g_free (contents);

    return retval;
}

