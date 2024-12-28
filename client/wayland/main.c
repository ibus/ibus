/* -*- mode: C; c-basic-offset: 4; indent-tabs-mode: nil; -*- */
/* vim:set et sts=4: */
/* ibus - The Input Bus
 * Copyright (C) 2023-2024 Takao Fujiwara <takao.fujiwara1@gmail.com>
 * Copyright (C) 2023-2024 Red Hat, Inc.
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

#include "config.h"

#include <ibus.h>
#include <wayland-client-core.h>
#include <stdlib.h>

#include "ibuswaylandim.h"

extern GSource *ibus_wayland_source_new (struct wl_display *display);

static void
_bus_disconnected_cb (IBusBus *bus,
                      gpointer user_data)
{
    g_debug ("Connection closed by ibus-daemon\n");
    g_clear_object (&bus);
    ibus_quit ();
}

gint
main (gint    argc,
      gchar **argv)
{
    IBusBus *bus;
    struct wl_display *display;
    IBusWaylandIM *wlim;
    GSource *source;

    ibus_init ();

    bus = ibus_bus_new ();
    if (!ibus_bus_is_connected (bus)) {
        g_printerr ("Cannot connect to ibus-daemon\n");
        return EXIT_FAILURE;
    }

    if (!(display = wl_display_connect (NULL))) {
        g_printerr ("Cannot open Wayland display\n");
        return EXIT_FAILURE;
    }
    wlim = ibus_wayland_im_new ("wl_display", display, "bus", bus, NULL);
    g_return_val_if_fail (wlim, EXIT_FAILURE);

    source = ibus_wayland_source_new (display);
    g_source_set_priority (source, G_PRIORITY_DEFAULT);
    g_source_set_can_recurse (source, TRUE);
    g_source_attach (source, NULL);

    g_signal_connect (bus, "disconnected",
                      G_CALLBACK (_bus_disconnected_cb), NULL);
    ibus_main ();

    return EXIT_SUCCESS;
}

