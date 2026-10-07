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
#include "factoryproxy.h"

#include "dbusimpl.h"
#include "global.h"
#include "marshalers.h"
#include "types.h"

struct _BusFactoryProxy {
    IBusProxy parent;
    /* instance members */
};

struct _BusFactoryProxyClass {
    IBusProxyClass parent;
    /* class members */
};

/* functions prototype */
static void      bus_factory_proxy_destroy      (IBusProxy        *proxy);

G_DEFINE_TYPE (BusFactoryProxy, bus_factory_proxy, IBUS_TYPE_PROXY)

static void
bus_factory_proxy_class_init (BusFactoryProxyClass *class)
{
    IBUS_PROXY_CLASS (class)->destroy = bus_factory_proxy_destroy;
}

static void
bus_factory_proxy_init (BusFactoryProxy *factory)
{
}

static void
bus_factory_proxy_destroy (IBusProxy *proxy)
{
    IBUS_PROXY_CLASS (bus_factory_proxy_parent_class)->destroy (IBUS_PROXY (proxy));
}

BusFactoryProxy *
bus_factory_proxy_new (BusConnection *connection)
{
    g_assert (BUS_IS_CONNECTION (connection));
    BusFactoryProxy *factory;

    GDBusProxyFlags flags = G_DBUS_PROXY_FLAGS_DO_NOT_AUTO_START |
                            G_DBUS_PROXY_FLAGS_DO_NOT_LOAD_PROPERTIES;
    factory = (BusFactoryProxy *) g_initable_new (
            BUS_TYPE_FACTORY_PROXY,
            NULL, NULL,
            "g-object-path",     IBUS_PATH_FACTORY,
            "g-interface-name",  IBUS_INTERFACE_FACTORY,
            "g-connection",      bus_connection_get_dbus_connection (connection),
            "g-default-timeout", g_gdbus_timeout,
            "g-flags",           flags,
            NULL);

    return factory;
}

void
bus_factory_proxy_create_engine (BusFactoryProxy    *factory,
                                 IBusEngineDesc     *desc,
                                 gint                timeout,
                                 GCancellable       *cancellable,
                                 GAsyncReadyCallback callback,
                                 gpointer            user_data)
{
    g_assert (BUS_IS_FACTORY_PROXY (factory));
    g_assert (IBUS_IS_ENGINE_DESC (desc));
    g_assert (cancellable == NULL || G_IS_CANCELLABLE (cancellable));

    g_dbus_proxy_call ((GDBusProxy *) factory,
                       "CreateEngine",
                       g_variant_new ("(s)", ibus_engine_desc_get_name (desc)),
                       G_DBUS_CALL_FLAGS_NONE,
                       timeout,
                       cancellable,
                       callback,
                       user_data);
}

gchar *
bus_factory_proxy_create_engine_finish (BusFactoryProxy  *factory,
                                        GAsyncResult     *res,
                                        GError          **error)
{

    GVariant *retval = g_dbus_proxy_call_finish ((GDBusProxy *) factory,
                                                 res,
                                                 error);
    if (retval == NULL)
        return NULL;

    gchar *object_path = NULL;
    g_variant_get (retval, "(o)", &object_path);
    g_variant_unref (retval);

    return object_path;
}


