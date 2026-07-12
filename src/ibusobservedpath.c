/* -*- mode: C; c-basic-offset: 4; indent-tabs-mode: nil; -*- */
/* vim:set et sts=4: */
/* ibus - The Input IBus
 * Copyright (C) 2008-2015 Peng Huang <shawn.p.huang@gmail.com>
 * Copyright (C) 2020 Takao Fujiwara <takao.fujiwara1@gmail.com>
 * Copyright (C) 2008-2020 Red Hat, Inc.
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
#include <glib/gstdio.h>
#include <stdlib.h>
#include "ibusinternal.h"
#include "ibusobservedpath.h"


enum {
    LAST_SIGNAL,
};


/* IBusObservedPathPrivate */
struct _IBusObservedPathPrivate {
    guint   *file_hash_list;
};
typedef struct _IBusObservedPathPrivate IBusObservedPathPrivate;

#define IBUS_OBSERVED_PATH_GET_PRIVATE(o)  \
   ((IBusObservedPathPrivate *)ibus_observed_path_get_instance_private (o))

// static guint            _signals[LAST_SIGNAL] = { 0 };

/* functions prototype */
static void      ibus_observed_path_destroy         (IBusObservedPath       *path);
static gboolean  ibus_observed_path_serialize       (IBusObservedPath       *path,
                                                     GVariantBuilder        *builder);
static gint      ibus_observed_path_deserialize     (IBusObservedPath       *path,
                                                     GVariant               *variant);
static gboolean  ibus_observed_path_copy            (IBusObservedPath       *dest,
                                                     const IBusObservedPath *src);
static gboolean  ibus_observed_path_parse_xml_node  (IBusObservedPath       *path,
                                                     XMLNode                *node);

G_DEFINE_TYPE_WITH_PRIVATE (IBusObservedPath,
                            ibus_observed_path,
                            IBUS_TYPE_SERIALIZABLE)

static void
ibus_observed_path_class_init (IBusObservedPathClass *class)
{
    IBusObjectClass *object_class = IBUS_OBJECT_CLASS (class);
    IBusSerializableClass *serializable_class = IBUS_SERIALIZABLE_CLASS (class);

    object_class->destroy = (IBusObjectDestroyFunc) ibus_observed_path_destroy;

    serializable_class->serialize   = (IBusSerializableSerializeFunc) ibus_observed_path_serialize;
    serializable_class->deserialize = (IBusSerializableDeserializeFunc) ibus_observed_path_deserialize;
    serializable_class->copy        = (IBusSerializableCopyFunc) ibus_observed_path_copy;
}


static void
ibus_observed_path_init (IBusObservedPath *path)
{
}

static void
ibus_observed_path_destroy (IBusObservedPath *path)
{
    IBusObservedPathPrivate *priv;
    g_free (path->path);
    if (IBUS_IS_OBSERVED_PATH (path)) {
        priv = IBUS_OBSERVED_PATH_GET_PRIVATE (path);
        g_clear_pointer (&priv->file_hash_list, g_free);
    }
    IBUS_OBJECT_CLASS (ibus_observed_path_parent_class)->destroy (IBUS_OBJECT (path));
}

static gboolean
ibus_observed_path_serialize (IBusObservedPath *path,
                              GVariantBuilder  *builder)
{
    IBusObservedPathPrivate *priv = IBUS_OBSERVED_PATH_GET_PRIVATE (path);
    gboolean retval;
    guint i;

    retval = IBUS_SERIALIZABLE_CLASS (ibus_observed_path_parent_class)->
            serialize ((IBusSerializable *)path, builder);
    g_return_val_if_fail (retval, FALSE);

    g_variant_builder_add (builder, "s", path->path);
    g_variant_builder_add (builder, "x", path->mtime);

    if (!priv->file_hash_list) {
        g_variant_builder_add (builder, "u", 0);
        return TRUE;
    }
    for (i = 0; priv->file_hash_list[i]; i++);
    g_variant_builder_add (builder, "u", i);
    for (i = 0; priv->file_hash_list[i]; i++)
        g_variant_builder_add (builder, "u", priv->file_hash_list[i]);

    return TRUE;
}

static gint
ibus_observed_path_deserialize (IBusObservedPath *path,
                                GVariant         *variant)
{
    IBusObservedPathPrivate *priv = IBUS_OBSERVED_PATH_GET_PRIVATE (path);
    gint retval;
    guint i, length = 0;

    retval = IBUS_SERIALIZABLE_CLASS (ibus_observed_path_parent_class)->
            deserialize ((IBusSerializable *)path, variant);
    g_return_val_if_fail (retval, 0);

    ibus_g_variant_get_child_string (variant, retval++, &path->path);
    g_variant_get_child (variant, retval++, "x", &path->mtime);

    if (g_variant_n_children (variant) < retval + 2)
        return retval;
    g_variant_get_child (variant, retval++, "u", &length);
    if (!length)
        return retval;
    priv->file_hash_list = g_new0 (guint, length + 1);
    for (i = 0; i < length; i++)
        g_variant_get_child (variant, retval++, "u", &priv->file_hash_list[i]);

    return retval;
}

static gboolean
ibus_observed_path_copy (IBusObservedPath       *dest,
                         const IBusObservedPath *src)
{
    IBusObservedPathPrivate *dest_priv = IBUS_OBSERVED_PATH_GET_PRIVATE (dest);
    IBusObservedPathPrivate *src_priv =
            IBUS_OBSERVED_PATH_GET_PRIVATE ((IBusObservedPath *)src);
    gboolean retval;
    guint i;

    retval = IBUS_SERIALIZABLE_CLASS (ibus_observed_path_parent_class)->
            copy ((IBusSerializable *)dest, (IBusSerializable *)src);
    g_return_val_if_fail (retval, FALSE);

    dest->path = g_strdup (src->path);
    dest->mtime = src->mtime;

    g_clear_pointer (&dest_priv->file_hash_list, g_free);
    if (!src_priv->file_hash_list)
        return TRUE;
    for (i = 0; src_priv->file_hash_list[i]; i++);
    dest_priv->file_hash_list = g_new0 (guint, i + 1);
    for (i = 0; src_priv->file_hash_list[i]; i++)
        dest_priv->file_hash_list[i] = src_priv->file_hash_list[i];

    return TRUE;
}

#define g_string_append_indent(string, indent)  \
    {                                           \
        gint i;                                 \
        for (i = 0; i < (indent); i++) {        \
            g_string_append (string, "    ");   \
        }                                       \
    }

void
ibus_observed_path_output (IBusObservedPath *path,
                           GString          *output,
                           gint              indent)
{
    IBusObservedPathPrivate *priv = IBUS_OBSERVED_PATH_GET_PRIVATE (path);
    guint i;

    g_assert (IBUS_IS_OBSERVED_PATH (path));
    g_assert (output);

    if (!priv->file_hash_list) {
        g_string_append_indent (output, indent);
        g_string_append_printf (output, "<path mtime=\"%ld\" >%s</path>\n",
                                path->mtime,
                                path->path);
    } else {
        g_string_append_indent (output, indent);
        g_string_append_printf (
                output,
                "<path mtime=\"%ld\" type=\"dir\" path=\"%s\">\n",
                path->mtime,
                path->path);
        for (i = 0; priv->file_hash_list[i]; i++) {
            g_string_append_indent (output, indent + 1);
            g_string_append_printf (output, "<file hash=\"%u\" />\n",
                                    priv->file_hash_list[i]);
        }
        g_string_append_indent (output, indent);
        g_string_append_printf (output, "</path>\n");
    }
}

gboolean
ibus_observed_path_check_modification (IBusObservedPath *path)
{
    IBusObservedPathPrivate *priv = IBUS_OBSERVED_PATH_GET_PRIVATE (path);
    gchar *real_path = NULL;
    struct stat buf;
    gboolean retval = FALSE;
    GDir *dir = NULL;
    const gchar *name;
    guint i = 0;
    guint file_num = 0;

    g_assert (IBUS_IS_OBSERVED_PATH (path));

    if (path->path[0] == '~') {
        const gchar *homedir = g_get_home_dir ();
        real_path = g_build_filename (homedir, path->path + 2, NULL);
    }
    else {
        real_path = g_strdup (path->path);
    }

    if (g_stat (real_path, &buf) != 0) {
        buf.st_mtime = 0;
    }


    if (path->mtime != buf.st_mtime) {
        retval = TRUE;
        goto end_check_modification;
    }

    /* If an ibus engine is installed, normal file system updates
     * the directory mtime of "/usr/share/ibus/component" and
     * path->mtime of the cache file and buf.st_mtime of the current directory
     * could have the different values.
     *
     * But under a special file system, the buf.st_mtime is not updated
     * even if an ibus engine is installed, likes Fedora Silverblue
     * and ibus_observed_path_check_modification() could not detect
     * the installed ibus engines.
     * Now path->priv->file_hash_list reserves the hash list of the files
     * in the observed directory and if a new ibus engine is installed,
     * the hash of the compose file does not exists in the cache's
     * file_hash_list and ibus-daemon regenerate the cache successfully.
     */
    if (!priv->file_hash_list) {
        /* If the cache version is old, ibus_registry_load_cache() returns
         * FALSE and ibus_registry_check_modification() and this are not
         * called.
         * If the cache version is the latest, the cache file includes the
         * filled file_hash_list for directories with ibus_observed_path_new()
         * when the cache was generated.
         * Then if file_hash_list is null, it's a simple file in ibus
         * components and return here simply.
         */
        goto end_check_modification;
    }
    dir = g_dir_open (real_path, 0, NULL);
    g_return_val_if_fail (dir, FALSE);

    while ((name = g_dir_read_name (dir)) != NULL) {
        guint current_hash;
        gboolean has_file = FALSE;

        if (!g_str_has_suffix (name, ".xml"))
            continue;
        current_hash = g_str_hash (name);
        for (i = 0; priv->file_hash_list[i]; i++) {
            if (current_hash == priv->file_hash_list[i]) {
                has_file = TRUE;
                break;
            }
        }
        if (!has_file) {
            retval = TRUE;
            goto end_check_modification;
        }
        file_num++;
    }
    if (!retval) {
        for (i = 0; priv->file_hash_list[i]; i++);
        if (file_num != i)
            retval = TRUE;
    }

end_check_modification:
    if (dir)
        g_dir_close (dir);
    g_free (real_path);
    return retval;
}

static void
ibus_observed_path_fill_stat (IBusObservedPath *path)
{
    g_assert (IBUS_IS_OBSERVED_PATH (path));

    gchar *real_path = NULL;
    struct stat buf;

    if (path->path[0] == '~') {
        const gchar *homedir = g_get_home_dir ();
        real_path = g_build_filename (homedir, path->path + 2, NULL);
    }
    else {
        real_path = g_strdup (path->path);
    }

    if (g_stat (real_path, &buf) == 0) {
        path->is_exist = 1;
        if (S_ISDIR (buf.st_mode)) {
            path->is_dir = 1;
        }
        path->mtime = buf.st_mtime;
    }
    else {
        path->is_dir = 0;
        path->is_exist = 0;
        path->mtime = 0;
    }
}

GList *
ibus_observed_path_traverse (IBusObservedPath *path,
                             gboolean          dir_only)
{
    g_assert (IBUS_IS_OBSERVED_PATH (path));

    GDir *dir;
    const gchar *name;
    GList *paths = NULL;

    dir = g_dir_open (path->path, 0, NULL);

    if (dir == NULL)
        return NULL;

    while ((name = g_dir_read_name (dir)) != NULL) {
        IBusObservedPath *sub;

        sub = g_object_new (IBUS_TYPE_OBSERVED_PATH, NULL);
        g_object_ref_sink (sub);
        sub->path = g_build_filename (path->path, name, NULL);

        ibus_observed_path_fill_stat (sub);
        if (sub->is_exist && sub->is_dir) {
            paths = g_list_append (paths, sub);
            paths = g_list_concat (paths,
                                   ibus_observed_path_traverse (sub, dir_only));
        } else if (sub->is_exist && !dir_only) {
            paths = g_list_append (paths, sub);
        } else {
            g_object_unref (sub);
        }
    }
    g_dir_close (dir);

    return paths;
}


static gboolean
ibus_observed_path_parse_file (IBusObservedPath *path,
                               XMLNode          *node,
                               int              *nth)
{
    IBusObservedPathPrivate *priv = IBUS_OBSERVED_PATH_GET_PRIVATE (path);
    gchar **attr;

    for (attr = node->attributes; attr[0]; attr += 2) {
        guint hash = 0;

        if (g_strcmp0 (*attr, "hash") == 0)
            hash = atol (attr[1]);
        else if (g_strcmp0 (*attr, "name") == 0)
            hash = g_str_hash (attr[1]);
        if (hash) {
            if (!priv->file_hash_list) {
                *nth = 0;
                priv->file_hash_list = g_new0 (guint, *nth + 2);
            } else {
                priv->file_hash_list = g_renew (guint, priv->file_hash_list,
                                                *nth + 2);
            }
            priv->file_hash_list[*nth] = hash;
            priv->file_hash_list[*nth + 1] = 0;
            *nth += 1;
            continue;
        }
        g_warning ("Unkonwn attribute %s", attr[0]);
    }

    return TRUE;
}


static gboolean
ibus_observed_path_parse_xml_node (IBusObservedPath *path,
                                   XMLNode          *node)
{
    gchar **attr;
    const gchar *full_path = node->text;
    GList *p;
    int i = 0;

    g_assert (IBUS_IS_OBSERVED_PATH (path));
    g_assert (node);

    if (G_UNLIKELY (g_strcmp0 (node->name, "path") != 0))
        return FALSE;

    for (attr = node->attributes; attr[0]; attr += 2) {
        if (g_strcmp0 (*attr, "mtime") == 0) {
            path->mtime = atol (attr[1]);
            continue;
        }
        if (g_strcmp0 (*attr, "path") == 0) {
            full_path = attr[1];
            continue;
        }
        if (g_strcmp0 (*attr, "type") == 0) {
            if (!g_strcmp0 (attr[1], "dir"))
                path->is_dir = TRUE;
            else if (!g_strcmp0 (attr[1], "file"))
                path->is_dir = FALSE;
            else
                g_warning ("The type attribute can be \"dir\" or \"file\".");
            continue;
        }
        g_warning ("Unkonwn attribute %s", attr[0]);
    }

    if (full_path[0] == '~' && full_path[1] != G_DIR_SEPARATOR) {
        g_warning ("Invalid path \"%s\"", full_path);
        return FALSE;
    }

    path->path = g_strdup (full_path);

    if (!path->is_dir)
        return TRUE;

    for (i = 0, p = node->sub_nodes; p != NULL; p = p->next) {
        XMLNode *sub_node = (XMLNode *)p->data;

        if (G_UNLIKELY (g_strcmp0 (sub_node->name, "file") != 0)) {
            g_warning ("Unkonwn tag %s", sub_node->name);
            continue;
        }
        ibus_observed_path_parse_file (path, sub_node, &i);
    }

    return TRUE;
}


IBusObservedPath *
ibus_observed_path_new_from_xml_node (XMLNode *node,
                                     gboolean fill_stat)
{
    g_assert (node);

    IBusObservedPath *path;

    path = (IBusObservedPath *) g_object_new (IBUS_TYPE_OBSERVED_PATH, NULL);

    if (!ibus_observed_path_parse_xml_node (path, node)) {
        g_object_unref (path);
        path = NULL;
    }
    else if (fill_stat) {
        ibus_observed_path_fill_stat (path);
    }

    return path;
}

IBusObservedPath *
ibus_observed_path_new (const gchar *path,
                        gboolean     fill_stat)
{
    IBusObservedPath *op;
    IBusObservedPathPrivate *priv;
    GList *file_list, *l;
    guint i = 0;

    g_assert (path);
    op = (IBusObservedPath *) g_object_new (IBUS_TYPE_OBSERVED_PATH, NULL);
    op->path = g_strdup (path);

    priv = IBUS_OBSERVED_PATH_GET_PRIVATE (op);
    l = file_list = ibus_observed_path_traverse (op, FALSE);
    for (; l; l = l->next) {
        IBusObservedPath *sub = l->data;
        const gchar *file = NULL;

        g_return_val_if_fail (sub && sub->path, op);

        file = sub->path;
        if (!g_str_has_suffix (file, ".xml"))
            continue;
        if (g_str_has_prefix (file, path)) {
            file += strlen (path);
            if (*file == '/')
                file++;
            /* Ignore sub directories */
            if (strchr (file, '/'))
                continue;
        }
        if (!i)
            priv->file_hash_list = g_new0 (guint, i + 2);
        else
            priv->file_hash_list = g_renew (guint, priv->file_hash_list, i + 2);
        priv->file_hash_list[i] = g_str_hash (file);
        priv->file_hash_list[i + 1] = 0;
        ++i;
    }
    g_list_free_full (file_list, (GDestroyNotify)g_object_unref);

    if (fill_stat)
        ibus_observed_path_fill_stat (op);

    return op;
}

