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

#include "ibusproxy.h"
#include "ibusmarshalers.h"
#include "ibusinternal.h"
#include "ibusobject.h"

#define IBUS_PROXY_GET_PRIVATE(o)  \
   (G_TYPE_INSTANCE_GET_PRIVATE ((o), IBUS_TYPE_PROXY, IBusProxyPrivate))

enum {
    DESTROY,
    LAST_SIGNAL,
};

static guint            proxy_signals[LAST_SIGNAL] = { 0 };

/* functions prototype */
static void      ibus_proxy_dispose         (GObject            *object);
static void      ibus_proxy_real_destroy    (IBusProxy          *proxy);

static void      ibus_proxy_connection_closed_cb
                                            (GDBusConnection    *connection,
                                             gboolean            remote_peer_vanished,
                                             GError             *error,
                                             IBusProxy          *proxy);
static void      initable_iface_init        (GInitableIface     *initable_iface);
static void      async_initable_iface_init  (GAsyncInitableIface
                                                                *async_initable_iface);
G_DEFINE_TYPE_WITH_CODE (IBusProxy, ibus_proxy, G_TYPE_DBUS_PROXY,
                         G_IMPLEMENT_INTERFACE (G_TYPE_INITABLE, initable_iface_init)
                         G_IMPLEMENT_INTERFACE (G_TYPE_ASYNC_INITABLE, async_initable_iface_init)
                         );

static void
ibus_proxy_class_init (IBusProxyClass *class)
{
    GObjectClass *gobject_class = G_OBJECT_CLASS (class);

    gobject_class->dispose = ibus_proxy_dispose;

    class->destroy = ibus_proxy_real_destroy;

    /* install signals */
    /**
     * IBusProxy::destroy:
     * @object: An IBusProxy.
     *
     * Destroy and free an IBusProxy
     *
     * See also:  ibus_proxy_destroy().
     *
     * <note><para>Argument @user_data is ignored in this function.</para></note>
     */
    proxy_signals[DESTROY] =
        g_signal_new (I_("destroy"),
            G_TYPE_FROM_CLASS (gobject_class),
            G_SIGNAL_RUN_LAST,
            G_STRUCT_OFFSET (IBusProxyClass, destroy),
            NULL, NULL,
            _ibus_marshal_VOID__VOID,
            G_TYPE_NONE, 0);
}

static void
ibus_proxy_init (IBusProxy *proxy)
{
    proxy->own = TRUE;
}

/**
 * ibus_proxy_dispose:
 *
 * Override GObject's dispose function.
 */
static void
ibus_proxy_dispose (GObject *object)
{
    if (! (IBUS_PROXY_FLAGS (object) & IBUS_IN_DESTRUCTION)) {
        IBUS_PROXY_SET_FLAGS (object, IBUS_IN_DESTRUCTION);
        if (! (IBUS_PROXY_FLAGS (object) & IBUS_DESTROYED)) {
            g_signal_emit (object, proxy_signals[DESTROY], 0);
            IBUS_PROXY_SET_FLAGS (object, IBUS_DESTROYED);
        }
        IBUS_PROXY_UNSET_FLAGS (object, IBUS_IN_DESTRUCTION);
    }

    G_OBJECT_CLASS(ibus_proxy_parent_class)->dispose (object);
}

/**
 * ibus_proxy_real_destroy:
 *
 * Handle "destroy" signal which is emitted by ibus_proxy_dispose.
 */
static void
ibus_proxy_real_destroy (IBusProxy *proxy)
{
    GDBusConnection *connection = g_dbus_proxy_get_connection ((GDBusProxy *) proxy);
    g_assert (connection != NULL);
    if (!g_dbus_connection_is_closed (connection) && proxy->own) {
        g_dbus_proxy_call ((GDBusProxy *)proxy,
                           "org.freedesktop.IBus.Service.Destroy",
                           NULL,
                           G_DBUS_CALL_FLAGS_NONE,
                           -1, NULL, NULL, NULL);
    }
    g_signal_handlers_disconnect_by_func (connection,
                                          (GCallback) ibus_proxy_connection_closed_cb,
                                          proxy);
}

static void
ibus_proxy_connection_closed_cb (GDBusConnection *connection,
                                 gboolean         remote_peer_vanished,
                                 GError          *error,
                                 IBusProxy       *proxy)
{
    ibus_proxy_destroy (proxy);
}

void
ibus_proxy_destroy (IBusProxy *proxy)
{
    g_assert (IBUS_IS_PROXY (proxy));

    if (! (IBUS_PROXY_FLAGS (proxy) & IBUS_IN_DESTRUCTION)) {
        g_object_run_dispose (G_OBJECT (proxy));
    }
}

static gboolean
ibus_proxy_init_finish (IBusProxy  *proxy,
                        GError    **error)
{
    g_assert (IBUS_IS_PROXY (proxy));
    g_assert (error == NULL || *error == NULL);

    GDBusConnection *connection =
            g_dbus_proxy_get_connection ((GDBusProxy *)proxy);

    if (connection == NULL || g_dbus_connection_is_closed (connection)) {
        /*
         * When proxy is created asynchronously, the connection may be closed
         * before proxy is ready. In this case, we need override interfaces
         * GInitable and GAsyncInitable to report the error.
         */
        if (error != NULL)
            *error = g_error_new (G_DBUS_ERROR, G_DBUS_ERROR_FAILED,
                    "Connection is closed.");
        return FALSE;
    }

    g_signal_connect (connection, "closed",
            G_CALLBACK (ibus_proxy_connection_closed_cb), proxy);

    return TRUE;
}


static GInitableIface *initable_iface_parent = NULL;

static gboolean
initable_init (GInitable     *initable,
               GCancellable  *cancellable,
               GError       **error)
{
    if (!initable_iface_parent->init (initable, cancellable, error))
        return FALSE;
    return ibus_proxy_init_finish ((IBusProxy *)initable, error);
}

static void
initable_iface_init (GInitableIface *initable_iface)
{
    initable_iface_parent = g_type_interface_peek_parent (initable_iface);
    initable_iface->init = initable_init;
}

static GAsyncInitableIface *async_initable_iface_parent = NULL;

static void
async_initable_init_async (GAsyncInitable      *initable,
                           gint                 io_priority,
                           GCancellable        *cancellable,
                           GAsyncReadyCallback  callback,
                           gpointer             user_data)
{
    async_initable_iface_parent->init_async (initable,
            io_priority, cancellable, callback, user_data);
}

static gboolean
async_initable_init_finish (GAsyncInitable  *initable,
                            GAsyncResult    *res,
                            GError         **error)
{
    if (!async_initable_iface_parent->init_finish (initable, res, error))
        return FALSE;
    return ibus_proxy_init_finish ((IBusProxy *)initable, error);
}

static void
async_initable_iface_init (GAsyncInitableIface *async_initable_iface)
{
    async_initable_iface_parent = g_type_interface_peek_parent (async_initable_iface);
    async_initable_iface->init_async = async_initable_init_async;
    async_initable_iface->init_finish = async_initable_init_finish;
}
