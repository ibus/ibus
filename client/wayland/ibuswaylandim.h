/* -*- mode: C; c-basic-offset: 4; indent-tabs-mode: nil; -*- */
/* vim:set et sts=4: */
/* ibus - The Input Bus
 * Copyright (C) 2023 Takao Fujiwara <takao.fujiwara1@gmail.com>
 * Copyright (C) 2023 Red Hat, Inc.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License
 * as published by the Free Software Foundation; either version 2.1 of
 * the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
 * 02110-1301 USA.
 */

#ifndef __IBUS_WAYLAND_IM_H_
#define __IBUS_WAYLAND_IM_H_

/**
 * SECTION: ibuswaylandim
 * @short_description: IBus Wayland input method protocol
 * @stability: Unstable
 *
 * An IBus Wayland IM Object.
 */

#include <glib.h>
#include <gio/gio.h>
#include <ibus.h>

#define IBUS_TYPE_WAYLAND_IM         \
    (ibus_wayland_im_get_type ())
#define IBUS_WAYLAND_IM(obj)         \
    (G_TYPE_CHECK_INSTANCE_CAST ((obj), IBUS_TYPE_WAYLAND_IM, IBusWaylandIM))

G_BEGIN_DECLS

typedef struct _IBusWaylandIM IBusWaylandIM;
typedef struct _IBusWaylandIMClass IBusWaylandIMClass;
typedef struct _IBusWaylandIMPrivate IBusWaylandIMPrivate;

/**
 * IBusWaylandIM:
 *
 * An IBus Wayland IM Object.
 */
struct _IBusWaylandIM {
    /*< private >*/
    IBusObject parent;
    IBusWaylandIMPrivate *priv;
};

struct _IBusWaylandIMClass {
    /*< private >*/
    IBusObjectClass parent;

    /* padding */
    gpointer pdummy[5];
};


GType            ibus_wayland_im_get_type          (void);

/**
 * ibus_wayland_im_new:
 * @bus: An #IBusBus.
 *
 * Creates a new #IBusWaylandIM with an #IBusBus.
 *
 * Returns: A newly allocated #IBusWaylandIM.
 */
IBusWaylandIM    *ibus_wayland_im_new    (IBusBus            *bus);

G_END_DECLS
#endif

