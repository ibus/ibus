/* -*- mode: C; c-basic-offset: 4; indent-tabs-mode: nil; -*- */
/* vim:set et sts=4: */
/* ibus - The Input Bus
 * Copyright (C) 2008-2013 Peng Huang <shawn.p.huang@gmail.com>
 * Copyright (C) 2008-2013 Red Hat, Inc.
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

#ifndef __IBUS_CONFIG_H_
#define __IBUS_CONFIG_H_

/**
 * SECTION: ibusconfig
 * @title: IBusConfig
 * @short_description: IBus engine configuration module.
 *
 * An IBusConfig provides engine configuration methods
 * such as get and set the configure settings to configuration file.
 *
 * Currently, IBusConfig supports gconf.
 */

#include "ibusproxy.h"

/*
 * Type macros.
 */

/* define GOBJECT macros */
#define IBUS_TYPE_CONFIG             \
    (ibus_config_get_type ())
#define IBUS_CONFIG(obj)             \
    (G_TYPE_CHECK_INSTANCE_CAST ((obj), IBUS_TYPE_CONFIG, IBusConfig))
#define IBUS_CONFIG_CLASS(klass)     \
    (G_TYPE_CHECK_CLASS_CAST ((klass), IBUS_TYPE_CONFIG, IBusConfigClass))
#define IBUS_IS_CONFIG(obj)          \
    (G_TYPE_CHECK_INSTANCE_TYPE ((obj), IBUS_TYPE_CONFIG))
#define IBUS_IS_CONFIG_CLASS(klass)  \
    (G_TYPE_CHECK_CLASS_TYPE ((klass), IBUS_TYPE_CONFIG))
#define IBUS_CONFIG_GET_CLASS(obj)   \
    (G_TYPE_INSTANCE_GET_CLASS ((obj), IBUS_TYPE_CONFIG, IBusConfigClass))

G_BEGIN_DECLS

typedef struct _IBusConfig IBusConfig;
typedef struct _IBusConfigClass IBusConfigClass;
typedef struct _IBusConfigPrivate IBusConfigPrivate;

/**
 * IBusConfig:
 *
 * An opaque data type representing an IBusConfig.
 */
struct _IBusConfig {
    /*< private >*/
    IBusProxy parent;
    IBusConfigPrivate *priv;

    /* instance members */
};

struct _IBusConfigClass {
    /*< private >*/
    IBusProxyClass parent;

    /* class members */
};

GType            ibus_config_get_type       (void);

/**
 * ibus_config_new:
 * @connection: A #GDBusConnection.
 * @cancellable: A #GCancellable or %NULL.
 * @error: Return location for error or %NULL.
 *
 * Create a new #IBusConfig from existing #GDBusConnection.
 *
 * Returns: A newly allocated #IBusConfig corresponding to @connection.
 */
IBusConfig      *ibus_config_new            (GDBusConnection    *connection,
                                             GCancellable       *cancellable,
                                             GError            **error);

/**
 * ibus_config_new_async:
 * @connection: An #GDBusConnection.
 * @cancellable: A #GCancellable or %NULL.
 * @callback: A #GAsyncReadyCallback to call when the request is satisfied.
 *      The callback should not be %NULL.
 * @user_data: The data to pass to callback.
 *
 * New an #IBusConfig asynchronously.
 */
void             ibus_config_new_async      (GDBusConnection    *connection,
                                             GCancellable       *cancellable,
                                             GAsyncReadyCallback callback,
                                             gpointer            user_data);

/**
 * ibus_config_new_async_finish:
 * @res: A #GAsyncResult obtained from the #GAsyncReadyCallback pass to
 *      ibus_config_new_async().
 * @error: Return location for error or %NULL.
 *
 * Finishes an operation started with ibus_config_new_async().
 *
 * Returns: A newly allocated #IBusConfig.
 */
IBusConfig      *ibus_config_new_async_finish
                                            (GAsyncResult       *res,
                                             GError            **error);

/**
 * ibus_config_get_value:
 * @config: An #IBusConfig
 * @section: Section name of the configuration option.
 * @name: Name of the configure option.
 *
 * Get the value of a configuration option synchronously.
 *
 * GConf stores configure options in a tree-like structure,
 * and the IBus related setting is at /desktop/ibus,
 * thus, @section here is a path from there,
 * while @name is the key of that configuration option.
 *
 * ibus-chewing, for example, stores its setting in /desktop/ibus/engine/Chewing,
 * so the section name for it is "engine/Chewing".
 * See also: ibus_config_set_value().
 *
 * Returns: A #GVariant or %NULL. Free with g_variant_unref().
 */
GVariant        *ibus_config_get_value      (IBusConfig         *config,
                                             const gchar        *section,
                                             const gchar        *name);

/**
 * ibus_config_get_value_async:
 * @config: An #IBusConfig
 * @section: Section name of the configuration option.
 * @name: Name of the configure option.
 * @timeout_ms: The timeout in milliseconds or -1 to use the default timeout.
 * @cancellable: A #GCancellable or %NULL.
 * @callback: Callback function to invoke when the return value is ready.
 * @user_data: The data to pass to callback.
 *
 * Get the value of a configuration option asynchronously.
 *
 * See also: ibus_config_get_value().
 */
void             ibus_config_get_value_async (IBusConfig         *config,
                                              const gchar        *section,
                                              const gchar        *name,
                                              gint                timeout_ms,
                                              GCancellable       *cancellable,
                                              GAsyncReadyCallback callback,
                                              gpointer            user_data);

/**
 * ibus_config_get_value_async_finish:
 * @config: A #IBusConfig.
 * @result: A #GAsyncResult.
 * @error: Return location for error or %NULL.
 * 
 * Finish get value of a configuration option.
 *
 * Returns: A #GVariant or %NULL if error is set. Free with g_variant_unref().
 *
 * See also: ibus_config_get_value_async().
 */
GVariant        *ibus_config_get_value_async_finish
                                            (IBusConfig         *config,
                                             GAsyncResult       *result,
                                             GError            **error);

/**
 * ibus_config_get_values:
 * @config: An #IBusConfig
 * @section: Section name of the configuration option.
 *
 * Get all values in a section synchronously.
 *
 * Returns: A #GVariant or %NULL. Free with g_variant_unref().
 *
 * See also: ibus_config_set_value().
 */
GVariant        *ibus_config_get_values     (IBusConfig         *config,
                                             const gchar        *section);

/**
 * ibus_config_get_values_async:
 * @config: An #IBusConfig
 * @section: Section name of the configuration option.
 * @timeout_ms: The timeout in milliseconds or -1 to use the default timeout.
 * @cancellable: A #GCancellable or %NULL.
 * @callback: Callback function to invoke when the return value is ready.
 * @user_data: The data to pass to callback.
 *
 * Get all values in a section asynchronously.
 *
 * See also: ibus_config_get_values().
 */
void             ibus_config_get_values_async(IBusConfig         *config,
                                              const gchar        *section,
                                              gint                timeout_ms,
                                              GCancellable       *cancellable,
                                              GAsyncReadyCallback callback,
                                              gpointer            user_data);

/**
 * ibus_config_get_values_async_finish:
 * @config: A #IBusConfig.
 * @result: A #GAsyncResult.
 * @error: Return location for error or %NULL.
 * 
 * Finish get values in a section.
 *
 * Returns: A #GVariant or %NULL if error is set. Free with g_variant_unref().
 *
 * See also: ibus_config_get_values_async().
 */
GVariant        *ibus_config_get_values_async_finish
                                            (IBusConfig         *config,
                                             GAsyncResult       *result,
                                             GError            **error);

/**
 * ibus_config_set_value:
 * @config: An #IBusConfig
 * @section: Section name of the configuration option.
 * @name: Name of the configure option its self.
 * @value: A #GVariant that holds the value. If the value is floating, the
 * function takes ownership of it.
 *
 * Set the value of a configuration option synchronously.
 *
 * Returns: %TRUE if succeed; %FALSE otherwise.
 *
 * See also: ibus_config_get_value().
 */
gboolean         ibus_config_set_value      (IBusConfig         *config,
                                             const gchar        *section,
                                             const gchar        *name,
                                             GVariant           *value);

/**
 * ibus_config_set_value_async:
 * @config: An #IBusConfig
 * @section: Section name of the configuration option.
 * @name: Name of the configure option.
 * @value: A #GVariant that holds the value. If the value is floating, the
 * function takes ownership of it.
 * @timeout_ms: The timeout in milliseconds or -1 to use the default timeout.
 * @cancellable: A #GCancellable or %NULL.
 * @callback: Callback function to invoke when the return value is ready.
 * @user_data: The data to pass to callback.
 *
 * Set the value of a configuration option asynchronously.
 *
 * See also: ibus_config_set_value().
 */
void             ibus_config_set_value_async (IBusConfig         *config,
                                              const gchar        *section,
                                              const gchar        *name,
                                              GVariant           *value,
                                              gint                timeout_ms,
                                              GCancellable       *cancellable,
                                              GAsyncReadyCallback callback,
                                              gpointer            user_data);

/**
 * ibus_config_set_value_async_finish:
 * @config: A #IBusConfig.
 * @result: A #GAsyncResult.
 * @error: Return location for error or %NULL.
 * 
 * Finish set value of a configuration option.
 *
 * Returns: %TRUE or %FALSE if error is set.
 *
 * See also: ibus_config_set_value_async().
 */
gboolean         ibus_config_set_value_async_finish
                                            (IBusConfig         *config,
                                             GAsyncResult       *result,
                                             GError            **error);

/**
 * ibus_config_unset:
 * @config: An #IBusConfig
 * @section: Section name of the configuration option.
 * @name: Name of the configure option its self.
 *
 * Remove an entry of a configuration option.
 *
 * Returns: %TRUE if succeed; %FALSE otherwise.
 *
 * See also: ibus_config_get_value().
 */
gboolean         ibus_config_unset      (IBusConfig         *config,
                                         const gchar        *section,
                                         const gchar        *name);

/* FIXME add an asynchronous version of unset */

/**
 * ibus_config_watch:
 * @config: An #IBusConfig
 * @section: (allow-none): Section name of the configuration option.
 * @name: (allow-none): Name of the configure option its self.
 *
 * Subscribe to the configuration option change notification.
 *
 * Until this function is called, every change will be notified to the
 * client through #IBusConfig::value-changed signal.  Clients should
 * call ibus_config_watch() with the sections they are interested in,
 * to reduce the number of D-Bus messages.
 *
 * Returns: %TRUE if succeed; %FALSE otherwise.
 *
 * See also: ibus_config_unwatch().
 */
gboolean         ibus_config_watch      (IBusConfig         *config,
                                         const gchar        *section,
                                         const gchar        *name);

/* FIXME add an asynchronous version of watch */

/**
 * ibus_config_unwatch:
 * @config: An #IBusConfig
 * @section: (allow-none): Section name of the configuration option.
 * @name: (allow-none): Name of the configure option its self.
 *
 * Unsubscribe from the configuration option change notification.
 *
 * Returns: %TRUE if succeed; %FALSE otherwise.
 *
 * See also: ibus_config_watch.
 */
gboolean         ibus_config_unwatch    (IBusConfig         *config,
                                         const gchar        *section,
                                         const gchar        *name);

/* FIXME add an asynchronous version of unwatch */

G_END_DECLS
#endif

