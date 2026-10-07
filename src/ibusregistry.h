/* -*- mode: C; c-basic-offset: 4; indent-tabs-mode: nil; -*- */
/* vim:set et sts=4: */
/* bus - The Input Bus
 * Copyright (C) 2013 Peng Huang <shawn.p.huang@gmail.com>
 * Copyright (C) 2013 Takao Fujiwara <takao.fujiwara1@gmail.com>
 * Copyright (C) 2013 Red Hat, Inc.
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

#ifndef __IBUS_REGISTRY_H_
#define __IBUS_REGISTRY_H_

/**
 * SECTION: ibusregistry
 * @short_description: Registry cache handling.
 * @title: IBusRegistry
 * @stability: Stable
 *
 * An #IBusRegistry loads IBus component files and generates the cache files.
 *
 * see_also: #IBusComponent
 */

#include "ibuscomponent.h"

/*
 * Type macros.
 */

/* define GOBJECT macros */
#define IBUS_TYPE_REGISTRY            \
    (ibus_registry_get_type ())
#define IBUS_REGISTRY(obj)            \
    (G_TYPE_CHECK_INSTANCE_CAST ((obj), IBUS_TYPE_REGISTRY, IBusRegistry))
#define IBUS_REGISTRY_CLASS(klass)    \
    (G_TYPE_CHECK_CLASS_CAST ((klass), IBUS_TYPE_REGISTRY, IBusRegistryClass))
#define IBUS_IS_REGISTRY(obj)         \
    (G_TYPE_CHECK_INSTANCE_TYPE ((obj), IBUS_TYPE_REGISTRY))
#define IBUS_IS_REGISTRY_CLASS(klass) \
    (G_TYPE_CHECK_CLASS_TYPE ((klass), IBUS_TYPE_REGISTRY))
#define IBUS_REGISTRY_GET_CLASS(obj)  \
    (G_TYPE_INSTANCE_GET_CLASS ((obj), IBUS_TYPE_REGISTRY, IBusRegistryClass))

G_BEGIN_DECLS

typedef struct _IBusRegistry IBusRegistry;
typedef struct _IBusRegistryPrivate IBusRegistryPrivate;
typedef struct _IBusRegistryClass IBusRegistryClass;

/**
 * IBusRegistry:
 *
 * Registry cache handling.
 * You can load the registry from compose files or a cache file.
 */
struct _IBusRegistry {
    /* instance members */
    /*< private >*/
    IBusSerializable parent;
    IBusRegistryPrivate *priv;
};

struct _IBusRegistryClass {
    /* class members */
    /*< private >*/
    IBusSerializableClass parent;
};

GType            ibus_registry_get_type         (void);

/**
 * ibus_registry_new:
 *
 * Creates a new #IBusRegistry
 *
 * Returns: A newly allocated #IBusRegistry.
 */
IBusRegistry    *ibus_registry_new              (void);

/**
 * ibus_registry_load:
 * @registry: An #IBusRegistry.
 *
 * Read all XML files in a IBus component directory (typically
 * /usr/share/ibus/component/ *.xml) and update the registry object.
 * IBUS_COMPONENT_PATH environment valuable is also available for
 * the custom component directories, whose delimiter is ':'.
 */
void             ibus_registry_load             (IBusRegistry   *registry);

/**
 * ibus_registry_load_in_dir:
 * @registry: An #IBusRegistry.
 * @dirname: IBus component directory which includes XML files.
 *
 * Read all XML files in @dirname, create a #IBusComponent object for each file,
 * and add the component objects to the registry.
 * If @dirname is "/usr/share/ibus/component", this API and
 * ibus_registry_load() are same.
 */
void             ibus_registry_load_in_dir      (IBusRegistry   *registry,
                                                 const gchar    *dirname);

/**
 * ibus_registry_load_cache:
 * @registry: An #IBusRegistry.
 * @is_user: %TRUE if the registry cache is loaded in the user directory.
 *
 * Load the user or system registry cache.
 *
 * Returns: %TRUE if the cache exists and is loaded successfully,
 *           %FALSE otherwise.
 */
gboolean         ibus_registry_load_cache       (IBusRegistry   *registry,
                                                 gboolean        is_user);

/**
 * ibus_registry_load_cache_file:
 * @registry: An #IBusRegistry.
 * @filename: The file path of the registry cache
 *
 * Load the registry cache @filename.
 *
 * Returns: %TRUE if the cache exists and is loaded successfully,
 *           %FALSE otherwise.
 */
gboolean         ibus_registry_load_cache_file  (IBusRegistry   *registry,
                                                 const gchar    *filename);

/**
 * ibus_registry_save_cache:
 * @registry: An #IBusRegistry.
 * @is_user: %TRUE if the registry cache is saved in the user directory.
 *
 * Save the registry in a user directory or system directory.
 *
 * Returns: %TRUE if the cache is saved successfully, %FALSE otherwise.
 */
gboolean         ibus_registry_save_cache       (IBusRegistry   *registry,
                                                 gboolean        is_user);

/**
 * ibus_registry_save_cache_file:
 * @registry: An #IBusRegistry.
 * @filename: The file path of the registry cache
 *
 * Save the registry cache @filename.
 *
 * Returns: %TRUE if the cache is saved successfully, %FALSE otherwise.
 */
gboolean         ibus_registry_save_cache_file  (IBusRegistry   *registry,
                                                 const gchar    *filename);

/**
 * ibus_registry_output:
 * @registry: An #IBusRegistry.
 * @output: GString that holds the result.
 * @indent: level of indent.
 *
 * Output #IBusRegistry as an XML-formatted string.
 * The output string can be then shown on the screen or written to file.
 */
void             ibus_registry_output           (IBusRegistry   *registry,
                                                 GString        *output,
                                                 int             indent);

/**
 * ibus_registry_check_modification:
 * @registry: An #IBusRegistry.
 *
 * Check if the registry is updated.
 *
 * Returns: %TRUE if mtime is changed; %FALSE otherwise.
 */
gboolean         ibus_registry_check_modification
                                                (IBusRegistry   *registry);

/**
 * ibus_registry_get_components:
 * @registry: An #IBusRegistry.
 *
 * List components.
 *
 * Returns: (transfer container) (element-type IBusComponent):
 * a list of #IBusComponent objects.
 * The caller has to call g_list_free() for the returned list.
 */
GList           *ibus_registry_get_components   (IBusRegistry   *registry);

/**
 * ibus_registry_get_observed_paths:
 * @registry: An #IBusRegistry.
 *
 * List observed paths.
 *
 * Returns: (transfer container) (element-type IBusObservedPath):
 * a list of #IBusObservedPath objects.
 * The caller has to call g_list_free() for the returned list.
 */
GList           *ibus_registry_get_observed_paths
                                                (IBusRegistry   *registry);

/**
 * ibus_registry_start_monitor_changes:
 * @registry: An #IBusRegistry.
 *
 * Start to monitor observed paths.
 */
void             ibus_registry_start_monitor_changes
                                                (IBusRegistry   *registry);

G_END_DECLS
#endif
