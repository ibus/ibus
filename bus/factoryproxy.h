/* -*- mode: C; c-basic-offset: 4; indent-tabs-mode: nil; -*- */
/* vim:set et sts=4: */
/* ibus - The Input Bus
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
#ifndef __BUS_FACTORY_PROXY_H_
#define __BUS_FACTORY_PROXY_H_

#include <ibus.h>
#include "connection.h"
#include "engineproxy.h"

/*
 * Type macros.
 */

/* define GOBJECT macros */
#define BUS_TYPE_FACTORY_PROXY             \
    (bus_factory_proxy_get_type ())
#define BUS_FACTORY_PROXY(obj)             \
    (G_TYPE_CHECK_INSTANCE_CAST ((obj), BUS_TYPE_FACTORY_PROXY, BusFactoryProxy))
#define BUS_FACTORY_PROXY_CLASS(klass)     \
    (G_TYPE_CHECK_CLASS_CAST ((klass), BUS_TYPE_FACTORY_PROXY, BusFactoryProxyClass))
#define BUS_IS_FACTORY_PROXY(obj)          \
    (G_TYPE_CHECK_INSTANCE_TYPE ((obj), BUS_TYPE_FACTORY_PROXY))
#define BUS_IS_FACTORY_PROXY_CLASS(klass)  \
    (G_TYPE_CHECK_CLASS_TYPE ((klass), BUS_TYPE_FACTORY_PROXY))
#define BUS_FACTORY_PROXY_GET_CLASS(obj)   \
    (G_TYPE_INSTANCE_GET_CLASS ((obj), BUS_TYPE_FACTORY_PROXY, BusFactoryProxyClass))

G_BEGIN_DECLS

typedef struct _BusFactoryProxy BusFactoryProxy;
typedef struct _BusFactoryProxyClass BusFactoryProxyClass;

GType            bus_factory_proxy_get_type     (void);

/**
 * bus_factory_proxy_new:
 * @connection: the connection between ibus-daemon and an engine process.
 * @returns: a new proxy object.
 */
BusFactoryProxy *bus_factory_proxy_new          (BusConnection      *connection);

/**
 * bus_factory_proxy_create_engine:
 * @desc: an engine description to create.
 * @timeout: timeout in msec, or -1 to use the default timeout value.
 *
 * Invoke "CreateEngine" method of the "org.freedesktop.IBus.Factory" interface asynchronously.
 */
void             bus_factory_proxy_create_engine
                                                (BusFactoryProxy    *factory,
                                                 IBusEngineDesc     *desc,
                                                 gint                timeout,
                                                 GCancellable       *cancellable,
                                                 GAsyncReadyCallback callback,
                                                 gpointer            user_data);

/**
 * bus_factory_proxy_create_engine_finish:
 * @returns: On success, return an D-Bus object path of the new engine. On error, returns NULL.
 *
 * Get the result of bus_factory_proxy_create_engine call. You have to call this function in the GAsyncReadyCallback function.
 */
gchar           *bus_factory_proxy_create_engine_finish
                                                (BusFactoryProxy    *factory,
                                                 GAsyncResult       *res,
                                                 GError            **error);

G_END_DECLS
#endif

