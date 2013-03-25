/* -*- mode: C; c-basic-offset: 4; indent-tabs-mode: nil; -*- */
/* vim:set et sts=4: */
/* ibus - The Input Bus
 * Copyright (C) 2008-2010 Peng Huang <shawn.p.huang@gmail.com>
 * Copyright (C) 2011 Daiki Ueno <ueno@unixuser.org>
 * Copyright (C) 2008-2011 Red Hat, Inc.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#include <string.h>
#include <ibus.h>
#include "config-private.h"

#define DCONF_PREFIX "/desktop/ibus"
#define DCONF_PRESERVE_NAME_PREFIXES_KEY \
    DCONF_PREFIX"/general/dconf-preserve-name-prefixes"

struct _IBusConfigDConf {
    IBusConfigService parent;
    DConfClient *client;

    /* if a dconf path matches one of preserve_name_prefixes, don't convert
       key names from/to GSettings key names (see _to_gsettings_name
       and _from_gsettings_name) */
    GSList *preserve_name_prefixes;
};

struct _IBusConfigDConfClass {
    IBusConfigServiceClass parent;
};

/* functions prototype */
static void      ibus_config_dconf_class_init  (IBusConfigDConfClass *class);
static void      ibus_config_dconf_init        (IBusConfigDConf      *config);
static void      ibus_config_dconf_destroy     (IBusConfigDConf      *config);
static gboolean  ibus_config_dconf_set_value   (IBusConfigService    *config,
                                                const gchar          *section,
                                                const gchar          *name,
                                                GVariant             *value,
                                                GError              **error);
static GVariant *ibus_config_dconf_get_value   (IBusConfigService    *config,
                                                const gchar          *section,
                                                const gchar          *name,
                                                GError              **error);
static GVariant *ibus_config_dconf_get_values  (IBusConfigService    *config,
                                                const gchar          *section,
                                                GError              **error);
static gboolean  ibus_config_dconf_unset_value (IBusConfigService    *config,
                                                const gchar          *section,
                                                const gchar          *name,
                                                GError              **error);

G_DEFINE_TYPE (IBusConfigDConf, ibus_config_dconf, IBUS_TYPE_CONFIG_SERVICE)

static void
ibus_config_dconf_class_init (IBusConfigDConfClass *class)
{
    IBusObjectClass *object_class = IBUS_OBJECT_CLASS (class);
    IBusConfigServiceClass *config_class = IBUS_CONFIG_SERVICE_CLASS (class);

    object_class->destroy = (IBusObjectDestroyFunc) ibus_config_dconf_destroy;
    config_class->set_value = ibus_config_dconf_set_value;
    config_class->get_value = ibus_config_dconf_get_value;
    config_class->get_values = ibus_config_dconf_get_values;
    config_class->unset_value = ibus_config_dconf_unset_value;
}

static gboolean
_has_prefixes (const gchar *path, GSList *prefixes)
{
    GSList *head = prefixes;
    for (; head; head = head->next) {
        if (g_str_has_prefix (path, head->data)) {
            return TRUE;
        }
    }
    return FALSE;
}

/* Convert key names from/to GSettings names.  While GSettings only
 * accepts lowercase letters / numbers / and dash ('-'), IBus uses
 * underscore ('_') and some engines even use uppercase letters.
 *
 * To minimize the gap, we do the following conversion:
 *
 * - when converting from IBus names to GSettings names, first convert
 *   all letters to lowercase and then replace underscores with dashes.
 * - when converting from GSettings names to IBus names, simply
 *   replace dashes with underscores.
 *
 * Note that though the conversion does not always roundtrip, it does
 * in most cases.
 */
static gchar *
_to_gsettings_name (const gchar *name)
{
    return g_strcanon (g_ascii_strdown (name, -1),
                       "abcdefghijklmnopqrstuvwxyz0123456789-",
                       '-');
}

static gchar *
_from_gsettings_name (const gchar *name)
{
    gchar *retval = g_strdup (name), *p;
    for (p = retval; *p != '\0'; p++)
        if (*p == '-')
            *p = '_';
    return retval;
}

typedef gchar *(* NameConvFunc) (const gchar *);

static gchar *
_conv_path (const gchar *path, NameConvFunc conv_func)
{
    gchar **strv = g_strsplit (path, "/", -1), **p;
    gchar *retval;

    for (p = strv; *p; p++) {
        gchar *canon;
        canon = (*conv_func) (*p);
        g_free (*p);
        *p = canon;
    }

    retval = g_strjoinv ("/", strv);
    g_strfreev (strv);
    return retval;
}

static gchar *
_to_gsettings_path (const gchar *path)
{
    return _conv_path (path, _to_gsettings_name);
}

static gchar *
_from_gsettings_path (const gchar *gpath)
{
    return _conv_path (gpath, _from_gsettings_name);
}

static void
_watch_func (DConfClient         *client,
             const gchar         *gpath,
             const gchar * const *items,
#ifndef DCONF_0_13_4
             gint                 n_items,
#endif
             const gchar         *tag,
             IBusConfigDConf     *config)
{
    gchar **gkeys = NULL;
    gint i;
#ifdef DCONF_0_13_4
    gint n_items;

    n_items = g_strv_length ((gchar **)items);
#endif

    g_return_if_fail (gpath != NULL);
    g_return_if_fail (n_items >= 0);

    if (dconf_is_key (gpath, NULL)) {
        /* If path is a key, the notification should be taken to mean
           that one key may have changed. */
        n_items = 1;
        gkeys = g_malloc0_n (n_items + 1, sizeof (gchar *));
        gkeys[0] = g_strdup (gpath);
    } else {
        if (n_items == 0) {
            /* If path is a dir and items is empty then it is an
               indication that any key under path may have
               changed. */
            gkeys = dconf_client_list (config->client, gpath, &n_items);
        } else {
            gkeys = g_boxed_copy (G_TYPE_STRV, items);
        }
        for (i = 0; i < n_items; i++) {
            gchar *gname = gkeys[i];
            gkeys[i] = g_strdup_printf ("%s/%s", gpath, gname);
            g_free (gname);
        }
    }

    for (i = 0; i < n_items; i++) {
        gchar *gname, *path, *name;
        GVariant *variant = dconf_client_read (config->client, gkeys[i]);

        if (variant == NULL) {
            /* Use a empty tuple for a unset value */
            variant = g_variant_new_tuple (NULL, 0);
        }

        gname = strrchr (gkeys[i], '/');
        g_assert (gname);
        *gname++ = '\0';

        if (_has_prefixes (gkeys[i], config->preserve_name_prefixes)) {
            path = gkeys[i];
            name = gname;
        } else {
            path = _from_gsettings_path (gkeys[i]);
            name = _from_gsettings_name (gname);
        }

        ibus_config_service_value_changed ((IBusConfigService *) config,
                                           path + sizeof (DCONF_PREFIX),
                                           name,
                                           variant);
        if (path != gkeys[i]) {
            g_free (path);
        }
        if (name != gname) {
            g_free (name);
        }
        g_variant_unref (variant);
    }
    g_strfreev (gkeys);
}

static void
ibus_config_dconf_init (IBusConfigDConf *config)
{
    GVariant *variant;
#ifdef DCONF_0_13_4
    config->client = dconf_client_new ();

    g_signal_connect (config->client, "changed",
                      G_CALLBACK (_watch_func), config);

    dconf_client_watch_fast (config->client, DCONF_PREFIX"/");
#else
    GError *error;

    config->client = dconf_client_new ("ibus",
                                       (DConfWatchFunc)_watch_func,
                                       config,
                                       NULL);

    error = NULL;
    if (!dconf_client_watch (config->client, DCONF_PREFIX"/", NULL, &error))
        g_warning ("Can not watch dconf path %s", DCONF_PREFIX"/");
#endif

    config->preserve_name_prefixes = NULL;
    variant = dconf_client_read (config->client,
                                 DCONF_PRESERVE_NAME_PREFIXES_KEY);
    if (variant != NULL) {
        GVariantIter iter;
        GVariant *child;

        g_variant_iter_init (&iter, variant);
        while ((child = g_variant_iter_next_value (&iter))) {
            char *prefix = g_variant_dup_string (child, NULL);
            config->preserve_name_prefixes =
                g_slist_prepend (config->preserve_name_prefixes,
                                 prefix);
            g_variant_unref (child);
        }
        g_variant_unref (variant);
    }
}

static void
ibus_config_dconf_destroy (IBusConfigDConf *config)
{
    if (config->client) {
#ifdef DCONF_0_13_4
        dconf_client_unwatch_fast (config->client, DCONF_PREFIX"/");
#else
        GError *error = NULL;
        if (!dconf_client_unwatch (config->client, DCONF_PREFIX"/", NULL, &error))
            g_warning ("Can not unwatch dconf path %s", DCONF_PREFIX"/");
#endif

        g_object_unref (config->client);
        config->client = NULL;
    }

    g_slist_free_full (config->preserve_name_prefixes, (GDestroyNotify) g_free);
    config->preserve_name_prefixes = NULL;

    IBUS_OBJECT_CLASS (ibus_config_dconf_parent_class)->
        destroy ((IBusObject *)config);
}

static gboolean
ibus_config_dconf_set_value (IBusConfigService *config,
                             const gchar       *section,
                             const gchar       *name,
                             GVariant          *value,
                             GError           **error)
{
    IBusConfigDConf *dconf = IBUS_CONFIG_DCONF (config);
    DConfClient *client = dconf->client;
    gchar *path, *gpath, *gname, *gkey;
    gboolean retval;

    path = g_strdup_printf (DCONF_PREFIX"/%s", section);
    gpath = _to_gsettings_path (path);
    g_free (path);

    if (_has_prefixes (gpath, dconf->preserve_name_prefixes)) {
        gname = (char *) name;
    } else {
        gname = _to_gsettings_name (name);
    }
    gkey = g_strconcat (gpath, "/", gname, NULL);
    g_free (gpath);
    if (gname != name) {
        g_free (gname);
    }

#ifdef DCONF_0_13_4
    retval = dconf_client_write_fast (client, gkey, value, error);
#else
    retval = dconf_client_write (client,
                                 gkey,
                                 value,
                                 NULL,   /* tag */
                                 NULL,   /* cancellable */
                                 error);
#endif
    g_free (gkey);

    /* notify the caller that the value has changed, as dconf does not
       call watch_func when the caller is the process itself */
    if (retval) {
        if (value == NULL) {
            /* Use a empty tuple for a unset value */
            value = g_variant_new_tuple (NULL, 0);
        }
        ibus_config_service_value_changed (config, section, name, value);
    }
    return retval;
}

static GVariant *
ibus_config_dconf_get_value (IBusConfigService *config,
                             const gchar       *section,
                             const gchar       *name,
                             GError           **error)
{
    IBusConfigDConf *dconf = IBUS_CONFIG_DCONF (config);
    DConfClient *client = dconf->client;
    gchar *path, *gpath, *gname, *gkey;
    GVariant *variant;

    path = g_strdup_printf (DCONF_PREFIX"/%s", section);
    gpath = _to_gsettings_path (path);
    g_free (path);

    if (_has_prefixes (gpath, dconf->preserve_name_prefixes)) {
        gname = (char *) name;
    } else {
        gname = _to_gsettings_name (name);
    }
    gkey = g_strconcat (gpath, "/", gname, NULL);
    g_free (gpath);
    if (gname != name) {
        g_free (gname);
    }

    variant = dconf_client_read (client, gkey);
    g_free (gkey);

    if (variant == NULL) {
        *error = g_error_new (G_DBUS_ERROR, G_DBUS_ERROR_FAILED,
                        "Config value [%s:%s] does not exist.", section, name);
        return NULL;
    }

    return variant;
}

static GVariant *
ibus_config_dconf_get_values (IBusConfigService *config,
                              const gchar       *section,
                              GError           **error)
{
    IBusConfigDConf *dconf = IBUS_CONFIG_DCONF (config);
    DConfClient *client = dconf->client;
    gchar *dir, *gdir;
    gint len;
    gchar **entries, **p;
    GVariantBuilder *builder;
    gboolean preserve_name;

    dir = g_strdup_printf (DCONF_PREFIX"/%s/", section);
    gdir = _to_gsettings_path (dir);
    g_free (dir);

    preserve_name = _has_prefixes (gdir, dconf->preserve_name_prefixes);

    entries = dconf_client_list (client, gdir, &len);
    builder = g_variant_builder_new (G_VARIANT_TYPE ("a{sv}"));
    for (p = entries; *p != NULL; p++) {
        gchar *gkey = g_strconcat (gdir, *p, NULL);
        GVariant *value = dconf_client_read (client, gkey);
        g_free (gkey);
        if (value) {
            gchar *name = *p;
            if (!preserve_name) {
                name = _from_gsettings_name (*p);
            }
            g_variant_builder_add (builder, "{sv}", name, value);
            if (name != *p) {
                g_free (name);
            }
            g_variant_unref (value);
        }
    }
    g_strfreev (entries);
    g_free (gdir);

    return g_variant_builder_end (builder);
}

static gboolean
ibus_config_dconf_unset_value (IBusConfigService *config,
                               const gchar       *section,
                               const gchar       *name,
                               GError           **error)
{
    return ibus_config_dconf_set_value (config, section, name, NULL, error);
}

IBusConfigDConf *
ibus_config_dconf_new (GDBusConnection *connection)
{
    IBusConfigDConf *config;
    config = (IBusConfigDConf *) g_object_new (IBUS_TYPE_CONFIG_DCONF,
                                               "object-path", IBUS_PATH_CONFIG,
                                               "connection", connection,
                                               NULL);
    return config;
}
