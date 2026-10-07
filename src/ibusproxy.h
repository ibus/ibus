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

#ifndef __IBUS_PROXY_H_
#define __IBUS_PROXY_H_

/**
 * SECTION: ibusproxy
 * @short_description: Base proxy object.
 * @stability: Stable
 *
 * An IBusProxy is the base of all proxy objects,
 * which communicate the corresponding #IBusServices on the other end of
 * IBusConnection.
 * For example, IBus clients (such as editors, web browsers) invoke the proxy
 * object,
 * IBusInputContext to communicate with the InputContext service of the
 * ibus-daemon.
 *
 * Almost all services have corresponding proxies, except very simple services.
 */

#include <gio/gio.h>

/*
 * Type macros.
 */

/* define GOBJECT macros */
#define IBUS_TYPE_PROXY             \
    (ibus_proxy_get_type ())
#define IBUS_PROXY(obj)             \
    (G_TYPE_CHECK_INSTANCE_CAST ((obj), IBUS_TYPE_PROXY, IBusProxy))
#define IBUS_PROXY_CLASS(klass)     \
    (G_TYPE_CHECK_CLASS_CAST ((klass), IBUS_TYPE_PROXY, IBusProxyClass))
#define IBUS_IS_PROXY(obj)          \
    (G_TYPE_CHECK_INSTANCE_TYPE ((obj), IBUS_TYPE_PROXY))
#define IBUS_IS_PROXY_CLASS(klass)  \
    (G_TYPE_CHECK_CLASS_TYPE ((klass), IBUS_TYPE_PROXY))
#define IBUS_PROXY_GET_CLASS(obj)   \
    (G_TYPE_INSTANCE_GET_CLASS ((obj), IBUS_TYPE_PROXY, IBusProxyClass))

G_BEGIN_DECLS

typedef struct _IBusProxy IBusProxy;
typedef struct _IBusProxyClass IBusProxyClass;

#define IBUS_PROXY_FLAGS(obj)             (IBUS_PROXY (obj)->flags)
#define IBUS_PROXY_SET_FLAGS(obj,flag)    G_STMT_START{ (IBUS_PROXY_FLAGS (obj) |= (flag)); }G_STMT_END
#define IBUS_PROXY_UNSET_FLAGS(obj,flag)  G_STMT_START{ (IBUS_PROXY_FLAGS (obj) &= ~(flag)); }G_STMT_END
#define IBUS_PROXY_DESTROYED(obj)         (IBUS_PROXY_FLAGS (obj) & IBUS_DESTROYED)

/**
 * IBusProxy:
 *
 * An opaque data type representing an IBusProxy.
 */
struct _IBusProxy {
    GDBusProxy parent;
    /* instance members */
    guint32 flags;
    gboolean own;
};

struct _IBusProxyClass {
    GDBusProxyClass parent;

    /* class members */
    void    (* destroy)     (IBusProxy      *proxy);
    /*< private >*/
    /* padding */
    gpointer pdummy[7];
};

GType   ibus_proxy_get_type (void);

/**
 * ibus_proxy_destroy:
 * @proxy: An #IBusProxy
 *
 * Dispose the proxy object. If the dbus connection is alive and the own
 * variable above is TRUE (which is the default),
 * org.freedesktop.IBus.Service.Destroy method will be called.
 * Note that "destroy" signal might be emitted when ibus_proxy_destroy is
 * called or the underlying dbus connection for the proxy is terminated.
 * In the callback of the destroy signal, you might have to call something
 * like 'g_object_unref(the_proxy);'.
 */
void    ibus_proxy_destroy  (IBusProxy      *proxy);

G_END_DECLS
#endif

