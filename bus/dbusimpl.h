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
#ifndef __BUS_DBUS_IMPL_H_
#define __BUS_DBUS_IMPL_H_

#include <gio/gio.h>
#include <ibus.h>
#include "connection.h"

/*
 * Type macros.
 */

/* define GOBJECT macros */
#define BUS_TYPE_DBUS_IMPL             \
    (bus_dbus_impl_get_type ())
#define BUS_DBUS_IMPL(obj)             \
    (G_TYPE_CHECK_INSTANCE_CAST ((obj), BUS_TYPE_DBUS_IMPL, BusDBusImpl))
#define BUS_DBUS_IMPL_CLASS(klass)     \
    (G_TYPE_CHECK_CLASS_CAST ((klass), BUS_TYPE_DBUS_IMPL, BusDBusImplClass))
#define BUS_IS_DBUS_IMPL(obj)          \
    (G_TYPE_CHECK_INSTANCE_TYPE ((obj), BUS_TYPE_DBUS_IMPL))
#define BUS_IS_DBUS_IMPL_CLASS(klass)  \
    (G_TYPE_CHECK_CLASS_TYPE ((klass), BUS_TYPE_DBUS_IMPL))
#define BUS_DBUS_IMPL_GET_CLASS(obj)   \
    (G_TYPE_INSTANCE_GET_CLASS ((obj), BUS_TYPE_DBUS_IMPL, BusDBusImplClass))

#define BUS_DEFAULT_DBUS \
    (bus_dbus_impl_get_default ())

G_BEGIN_DECLS

typedef struct _BusDBusImpl BusDBusImpl;
typedef struct _BusDBusImplClass BusDBusImplClass;

GType            bus_dbus_impl_get_type         (void);

/**
 * bus_dbus_impl_get_default:
 * @returns: a BusDBusImpl object which is a singleton.
 *
 * Instantiate a BusDBusImpl object (if necessary) and return the object.
 */
BusDBusImpl     *bus_dbus_impl_get_default      (void);

/**
 * bus_dbus_impl_new_connection:
 * @connection: A new connection.
 * @returns: TRUE
 *
 * Register all IBusServices (e.g. DBus, IBus, IBus.InputContext) to the connection so that the service_method_call function
 * for each service could be called.
 */
gboolean         bus_dbus_impl_new_connection   (BusDBusImpl    *dbus,
                                                 BusConnection  *connection);
/**
 * bus_dbus_impl_get_connection_by_name:
 * @name: A connection name like ":1.0" and "org.freedesktop.IBus.Panel".
 * @returns: A BusConnection object which corresponds to the name.
 *
 * Search for an active connection whose name is name. If not found, return NULL.
 */
BusConnection   *bus_dbus_impl_get_connection_by_name
                                                (BusDBusImpl    *dbus,
                                                 const gchar    *name);

/**
 * bus_dbus_impl_forward_message:
 *
 * Push the message to the queue (dbus->forward_queue) and schedule a idle function call (bus_dbus_impl_forward_message_idle_cb) which
 * actually forwards the message to the destination. Note that the destination of the message is embedded in the message.
 */
void             bus_dbus_impl_forward_message  (BusDBusImpl    *dbus,
                                                 BusConnection  *connection,
                                                 GDBusMessage   *message);

/**
 * bus_dbus_impl_dispatch_message_by_rule:
 *
 * Push the message to the queue (dbus->dispatch_queue) and schedule a idle function call (bus_dbus_impl_dispatch_message_by_rule_idle_cb)
 * which actually dispatch the message by rule.
 */
void             bus_dbus_impl_dispatch_message_by_rule
                                                (BusDBusImpl    *dbus,
                                                 GDBusMessage   *message,
                                                 BusConnection  *skip_connection);

/**
 * bus_dbus_impl_register_object:
 * @object: A new service which implements IBusService, like BusIBusImpl and BusInputContext.
 * @returns: FALSE if dbus is already destroyed. otherwise TRUE.
 *
 * Add the IBusService to the daemon. See bus_dbus_impl_new_connection for details.
 */
gboolean         bus_dbus_impl_register_object  (BusDBusImpl    *dbus,
                                                 IBusService    *object);

/**
 * bus_dbus_impl_unregister_object:
 * @object: A new service which implements IBusService, like BusIBusImpl and BusInputContext.
 * @returns: FALSE if dbus is already destroyed. otherwise TRUE.
 *
 * Remove the IBusService from the daemon.
 */
gboolean         bus_dbus_impl_unregister_object(BusDBusImpl    *dbus,
                                                 IBusService    *object);
G_END_DECLS
#endif

