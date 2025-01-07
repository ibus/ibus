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
#define IBUS_IS_WAYLAND_IM(obj)      \
    (G_TYPE_CHECK_INSTANCE_TYPE ((obj), IBUS_TYPE_WAYLAND_IM))

G_BEGIN_DECLS

typedef struct _IBusWaylandIM IBusWaylandIM;
typedef struct _IBusWaylandIMClass IBusWaylandIMClass;

/**
 * IBusWaylandIM:
 *
 * An IBus Wayland IM Object.
 */
struct _IBusWaylandIM {
    /*< private >*/
    IBusObject parent;
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
 * @first_property_name: Name of the first property.
 * @...: the NULL-terminated arguments of the properties and values.
 *
 * Creates a new #IBusWaylandIM.
 * ibus_wayland_im_new() supports the va_list format.
 * name property is required. e.g.
 * ibus_wayland_im_new("bus", bus, "wl_display", display, NULL)
 *
 * Returns: A newly allocated #IBusWaylandIM.
 */
IBusWaylandIM    *ibus_wayland_im_new         (const gchar
                                                           *first_property_name,
                                                            ...);

/**
 * ibus_wayland_im_set_surface:
 * @wlim: An #IBusWaylandIM.
 * @surface: A struct wl_surface.
 *
 * Set wl_surface to #IBusWaylandIM and return %TRUE if
 * set_overlay_panel() can be called, otherwise %FALSE.
 */
gboolean          ibus_wayland_im_set_surface (IBusWaylandIM *wlim,
                                               gpointer       surface);

G_END_DECLS
#endif
