/* -*- mode: C; c-basic-offset: 4; indent-tabs-mode: nil; -*- */
/* vim:set et sts=4: */
/* bus - The Input Bus
 * Copyright (C) 2008-2010 Peng Huang <shawn.p.huang@gmail.com>
 * Copyright (C) 2008-2010 Red Hat, Inc.
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

#include <unistd.h>
#include "connection.h"
#include "matchrule.h"

struct _BusConnection {
    IBusObject parent;

    /* instance members */

    /* underlying GDBus connetion */
    GDBusConnection *connection;
    /* a unique name of the connection like ":1.0" */
    gchar *unique_name;
    /* list for well known names */
    GList  *names;

    guint  filter_id;
};

struct _BusConnectionClass {
  IBusObjectClass parent;

  /* class members */
};

/* static guint            _signals[LAST_SIGNAL] = { 0 }; */

/* functions prototype */
static void     bus_connection_destroy      (BusConnection      *connection);
static void     bus_connection_set_dbus_connection
                                            (BusConnection      *connection,
                                             GDBusConnection    *dbus_connection);
static void     bus_connection_dbus_connection_closed_cb
                                            (GDBusConnection    *dbus_connection,
                                             gboolean            remote_peer_vanished,
                                             GError             *error,
                                             BusConnection      *connection);
static GQuark   bus_connection_quark        (void);

#define BUS_CONNECTION_QUARK (bus_connection_quark ())

G_DEFINE_TYPE (BusConnection, bus_connection, IBUS_TYPE_OBJECT)

static void
bus_connection_class_init (BusConnectionClass *class)
{
    IBusObjectClass *ibus_object_class = IBUS_OBJECT_CLASS (class);

    ibus_object_class->destroy = (IBusObjectDestroyFunc) bus_connection_destroy;
}

static void
bus_connection_init (BusConnection *connection)
{
}

static void
bus_connection_destroy (BusConnection *connection)
{
    if (connection->connection) {
        /* disconnect from closed signal */
        g_signal_handlers_disconnect_by_func (connection->connection,
                G_CALLBACK (bus_connection_dbus_connection_closed_cb), connection);

        /* remove filter */
        bus_connection_set_filter (connection, NULL, NULL, NULL);

        /* disconnect busconnection with dbus connection */
        g_object_set_qdata ((GObject *)connection->connection, BUS_CONNECTION_QUARK, NULL);
        if (!g_dbus_connection_is_closed (connection->connection)) {
            g_dbus_connection_close (connection->connection, NULL, NULL, NULL);
        }
        g_object_unref (connection->connection);
        connection->connection = NULL;
    }

    if (connection->unique_name) {
        g_free (connection->unique_name);
        connection->unique_name = NULL;
    }

    g_list_free_full (connection->names, g_free);
    connection->names = NULL;

    IBUS_OBJECT_CLASS(bus_connection_parent_class)->destroy (IBUS_OBJECT (connection));
}

static void
bus_connection_dbus_connection_closed_cb (GDBusConnection *dbus_connection,
                                          gboolean         remote_peer_vanished,
                                          GError          *error,
                                          BusConnection   *connection)
{
    ibus_object_destroy ((IBusObject *) connection);
}

static void
bus_connection_set_dbus_connection (BusConnection   *connection,
                                    GDBusConnection *dbus_connection)
{
    connection->connection = dbus_connection;
    g_object_ref (connection->connection);
    g_object_set_qdata_full ((GObject *) dbus_connection,
                             BUS_CONNECTION_QUARK,
                             g_object_ref (connection),
                             (GDestroyNotify) g_object_unref);
    g_signal_connect (connection->connection, "closed",
                G_CALLBACK (bus_connection_dbus_connection_closed_cb), connection);
}

static GQuark
bus_connection_quark (void)
{
    static GQuark quark = 0;
    if (quark == 0) {
        quark = g_quark_from_static_string ("BUS_CONNECTION");
    }
    return quark;
}

BusConnection *
bus_connection_new (GDBusConnection *dbus_connection)
{
    g_return_val_if_fail (bus_connection_lookup (dbus_connection) == NULL, NULL);
    BusConnection *connection = BUS_CONNECTION (g_object_new (BUS_TYPE_CONNECTION, NULL));
    bus_connection_set_dbus_connection (connection, dbus_connection);
    return connection;
}

BusConnection *
bus_connection_lookup (GDBusConnection *dbus_connection)
{
    g_return_val_if_fail (G_IS_DBUS_CONNECTION (dbus_connection), NULL);
    return (BusConnection *) g_object_get_qdata ((GObject *) dbus_connection,
                    BUS_CONNECTION_QUARK);
}

const gchar *
bus_connection_get_unique_name (BusConnection   *connection)
{
    return connection->unique_name;
}

void
bus_connection_set_unique_name (BusConnection   *connection,
                                const gchar     *name)
{
    g_assert (connection->unique_name == NULL); /* we don't allow rewriting the unique_name. */
    connection->unique_name = g_strdup (name);
}

const GList *
bus_connection_get_names (BusConnection   *connection)
{
    return connection->names;
}

const gchar *
bus_connection_add_name (BusConnection     *connection,
                         const gchar       *name)
{
    gchar *new_name;

    new_name = g_strdup (name);
    connection->names = g_list_append (connection->names, new_name);

    return new_name;
}

gboolean
bus_connection_remove_name (BusConnection     *connection,
                            const gchar       *name)
{
    GList *list = g_list_find_custom (connection->names, name, (GCompareFunc) g_strcmp0);

    if (list) {
        g_free (list->data);
        connection->names = g_list_delete_link (connection->names, list);
        return TRUE;
    }
    return FALSE;
}

gboolean
bus_connection_has_name (BusConnection     *connection,
                         const gchar       *name)
{
    GList *list = g_list_find_custom (connection->names, name, (GCompareFunc) g_strcmp0);
    return list != NULL;
}

GDBusConnection *
bus_connection_get_dbus_connection (BusConnection *connection)
{
    g_assert (BUS_IS_CONNECTION (connection));
    return connection->connection;
}

void
bus_connection_set_filter (BusConnection             *connection,
                           GDBusMessageFilterFunction filter_func,
                           gpointer                   user_data,
                           GDestroyNotify             user_data_free_func)
{
    g_assert (BUS_IS_CONNECTION (connection));

    if (connection->filter_id != 0) {
        g_dbus_connection_remove_filter (connection->connection, connection->filter_id);
        connection->filter_id = 0;
    }

    if (filter_func != NULL) {
        connection->filter_id = g_dbus_connection_add_filter (connection->connection,
                                                              filter_func,
                                                              user_data,
                                                              user_data_free_func);
        /* Note: g_dbus_connection_add_filter seems not to return zero as a valid id. */
    }
}
