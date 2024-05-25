/* -*- mode: C; c-basic-offset: 4; indent-tabs-mode: nil; -*- */
/* vim:set et sts=4: */
/* bus - The Input Bus
 * Copyright (C) 2008-2010 Peng Huang <shawn.p.huang@gmail.com>
 * Copyright (C) 2008-2010 Red Hat, Inc.
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
#ifndef __BUS_CONNECTION_H_
#define __BUS_CONNECTION_H_

#include <ibus.h>

/*
 * Type macros.
 */

/* define GOBJECT macros */
#define BUS_TYPE_CONNECTION             \
    (bus_connection_get_type ())
#define BUS_CONNECTION(obj)             \
    (G_TYPE_CHECK_INSTANCE_CAST ((obj), BUS_TYPE_CONNECTION, BusConnection))
#define BUS_CONNECTION_CLASS(klass)     \
    (G_TYPE_CHECK_CLASS_CAST ((klass), BUS_TYPE_CONNECTION, BusConnectionClass))
#define BUS_IS_CONNECTION(obj)          \
    (G_TYPE_CHECK_INSTANCE_TYPE ((obj), BUS_TYPE_CONNECTION))
#define BUS_IS_CONNECTION_CLASS(klass)  \
    (G_TYPE_CHECK_CLASS_TYPE ((klass), BUS_TYPE_CONNECTION))
#define BUS_CONNECTION_GET_CLASS(obj)   \
    (G_TYPE_INSTANCE_GET_CLASS ((obj), BUS_TYPE_CONNECTION, BusConnectionClass))

/* bus_dbus_impl_hello() sets the client unique names ":1.1" or later. */
#define IBUS_NAME_OWNER_NAME ":1.0"

G_BEGIN_DECLS

typedef struct _BusConnection BusConnection;
typedef struct _BusConnectionClass BusConnectionClass;

GType            bus_connection_get_type            (void);

/**
 * bus_connection_new:
 *
 * Create a BusConnection object from a low-level GDBus connection.
 */
BusConnection   *bus_connection_new                 (GDBusConnection    *connection);

/**
 * bus_connection_lookup:
 *
 * Lookup the BusConnection object which corresponds to the low-level connection.
 */
BusConnection   *bus_connection_lookup              (GDBusConnection    *connection);

const gchar     *bus_connection_get_unique_name     (BusConnection      *connection);
void             bus_connection_set_unique_name     (BusConnection      *connection,
                                                     const gchar        *name);

/**
 * bus_connection_get_names:
 *
 * Get the list of well-known names of the connection.
 */
const GList     *bus_connection_get_names           (BusConnection      *connection);

/**
 * bus_connection_add_name:
 * @name: a well-known name for the connection.
 * @returns: g_strdup (name)
 *
 * Add the well-known name to the connection.
 */
const gchar     *bus_connection_add_name            (BusConnection      *connection,
                                                     const gchar        *name);

/**
 * bus_connection_remove_name:
 * @name: a well-known name for the connection.
 * @returns: TRUE on success.
 *
 * Remove the well-known name from the connection.
 */
gboolean         bus_connection_remove_name         (BusConnection      *connection,
                                                     const gchar        *name);

/**
 * bus_connection_has_name:
 * @name: a well-known name for the connection.
 * @returns: TRUE if found the name.
 *
 * Lookup the well-known name from the connection.
 */
gboolean         bus_connection_has_name            (BusConnection      *connection,
                                                     const gchar        *name);

/**
 * bus_connection_get_dbus_connection:
 *
 * Get the underlying GDBus connection.
 */
GDBusConnection *bus_connection_get_dbus_connection (BusConnection      *connection);

/**
 * bus_connection_set_filter:
 *
 * Set a filter function which will be called on all incoming and outgoing messages on the connection.
 * WARNING - this filter function could be called by the GDBus's worker thread. So the function should not call thread unsafe IBus functions.
 */
void             bus_connection_set_filter          (BusConnection      *connection,
                                                     GDBusMessageFilterFunction
                                                                         filter_func,
                                                     gpointer            user_data,
                                                     GDestroyNotify      user_data_free_func);

G_END_DECLS
#endif

