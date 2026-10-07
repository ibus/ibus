/* -*- mode: C; c-basic-offset: 4; indent-tabs-mode: nil; -*- */
/* vim:set et sts=4: */
/* ibus - The Input Bus
 * Copyright (c) 2010, Google Inc. All rights reserved.
 * Copyright (C) 2010 Peng Huang <shawn.p.huang@gmail.com>
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
#include <string.h>
#include <ibus.h>
#include "config.h"


typedef struct _IBusConfigMemconfClass IBusConfigMemconfClass;

struct _IBusConfigMemconf {
    IBusConfigService parent;
    GHashTable *values;
};

struct _IBusConfigMemconfClass {
    IBusConfigServiceClass parent;
};

/* functions prototype */
static void         ibus_config_memconf_class_init  (IBusConfigMemconfClass *class);
static void         ibus_config_memconf_init        (IBusConfigMemconf      *config);
static void         ibus_config_memconf_destroy     (IBusConfigMemconf      *config);
static gboolean     ibus_config_memconf_set_value   (IBusConfigService      *config,
                                                     const gchar            *section,
                                                     const gchar            *name,
                                                     GVariant               *value,
                                                     GError                **error);
static GVariant    *ibus_config_memconf_get_value   (IBusConfigService      *config,
                                                     const gchar            *section,
                                                     const gchar            *name,
                                                     GError                **error);
static GVariant    *ibus_config_memconf_get_values  (IBusConfigService      *config,
                                                     const gchar            *section,
                                                     GError                **error);
static gboolean     ibus_config_memconf_unset_value (IBusConfigService      *config,
                                                     const gchar            *section,
                                                     const gchar            *name,
                                                     GError                **error);

G_DEFINE_TYPE (IBusConfigMemconf, ibus_config_memconf, IBUS_TYPE_CONFIG_SERVICE)

static void
ibus_config_memconf_class_init (IBusConfigMemconfClass *class)
{
    GObjectClass *object_class = G_OBJECT_CLASS (class);

    IBUS_OBJECT_CLASS (object_class)->destroy = (IBusObjectDestroyFunc) ibus_config_memconf_destroy;
    IBUS_CONFIG_SERVICE_CLASS (object_class)->set_value   = ibus_config_memconf_set_value;
    IBUS_CONFIG_SERVICE_CLASS (object_class)->get_value   = ibus_config_memconf_get_value;
    IBUS_CONFIG_SERVICE_CLASS (object_class)->get_values  = ibus_config_memconf_get_values;
    IBUS_CONFIG_SERVICE_CLASS (object_class)->unset_value = ibus_config_memconf_unset_value;
}

static void
ibus_config_memconf_init (IBusConfigMemconf *config)
{
    config->values = g_hash_table_new_full (g_str_hash,
                                            g_str_equal,
                                            (GDestroyNotify)g_free,
                                            (GDestroyNotify)g_variant_unref);
}

static void
ibus_config_memconf_destroy (IBusConfigMemconf *config)
{
    g_hash_table_destroy (config->values);
    IBUS_OBJECT_CLASS (ibus_config_memconf_parent_class)->destroy ((IBusObject *)config);
}

static gboolean
ibus_config_memconf_set_value (IBusConfigService *config,
                               const gchar       *section,
                               const gchar       *name,
                               GVariant          *value,
                               GError           **error)
{
    g_assert (IBUS_IS_CONFIG_MEMCONF (config));
    g_assert (section);
    g_assert (name);
    g_assert (value);
    g_assert (error == NULL || *error == NULL);

    gchar *key = g_strdup_printf ("%s:%s", section, name);

    g_hash_table_insert (IBUS_CONFIG_MEMCONF (config)->values,
                         key, g_variant_ref_sink (value));

    ibus_config_service_value_changed (config, section, name, value);

    return TRUE;
}

static GVariant *
ibus_config_memconf_get_value (IBusConfigService *config,
                               const gchar       *section,
                               const gchar       *name,
                               GError           **error)
{
    g_assert (IBUS_IS_CONFIG_MEMCONF (config));
    g_assert (section);
    g_assert (name);
    g_assert (error == NULL || *error == NULL);

    gchar *key = g_strdup_printf ("%s:%s", section, name);
    GVariant *value = (GVariant *)g_hash_table_lookup (IBUS_CONFIG_MEMCONF (config)->values, key);
    g_free (key);

    if (value != NULL) {
        g_variant_ref (value);
    }
    else if (error != NULL) {
        *error = g_error_new (G_DBUS_ERROR, G_DBUS_ERROR_FAILED,
                              "Config value [%s:%s] does not exist.", section, name);
    }
    return value;
}

static GVariant *
ibus_config_memconf_get_values (IBusConfigService *config,
                                const gchar       *section,
                                GError           **error)
{
    g_assert (IBUS_IS_CONFIG_MEMCONF (config));
    g_assert (section);
    g_assert (error == NULL || *error == NULL);

    GHashTableIter iter;
    const gchar *key;
    GVariant *value;
    
    GVariantBuilder *builder = g_variant_builder_new (G_VARIANT_TYPE ("a{sv}"));
    g_hash_table_iter_init (&iter, IBUS_CONFIG_MEMCONF (config)->values);
    while (g_hash_table_iter_next (&iter, (gpointer *)&key, (gpointer *)&value)) {
        gchar **v = g_strsplit (key, ":", 2);
        if (g_strcmp0 (v[0], section) == 0) {
            g_variant_builder_add (builder, "{sv}", v[1], value); 
        }
        g_strfreev(v);
    }

    return g_variant_builder_end (builder);
}

static gboolean
ibus_config_memconf_unset_value (IBusConfigService *config,
                                 const gchar       *section,
                                 const gchar       *name,
                                 GError            **error)
{
    g_assert (IBUS_IS_CONFIG_MEMCONF (config));
    g_assert (section);
    g_assert (name);
    g_assert (error == NULL || *error == NULL);

    gchar *key = g_strdup_printf ("%s:%s", section, name);
    gboolean retval = g_hash_table_remove (IBUS_CONFIG_MEMCONF (config)->values, key);
    g_free (key);

    if (retval) {
        ibus_config_service_value_changed (config,
                                           section,
                                           name,
                                           g_variant_new_tuple (NULL, 0));
    }
    else {
        g_set_error (error,
                     G_DBUS_ERROR, G_DBUS_ERROR_FAILED,
                     "Config value [%s:%s] does not exist.",
                     section, name);
    }
    return retval;
}

IBusConfigMemconf *
ibus_config_memconf_new (GDBusConnection *connection)
{
    IBusConfigMemconf *config;
    config = (IBusConfigMemconf *) g_object_new (IBUS_TYPE_CONFIG_MEMCONF,
                                                 "object-path", IBUS_PATH_CONFIG,
                                                 "connection", connection,
                                                 NULL);
    return config;
}
