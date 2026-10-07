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
#ifndef __BUS_SERVER_H_
#define __BUS_SERVER_H_

#include <ibus.h>

G_BEGIN_DECLS

/**
 * bus_server_init:
 *
 * Initialize GDBus server and write the server address to a file, which is (usually) in ~/.config/ibus/bus/.
 * Note that the function does not call g_main_loop_run.
 */
void         bus_server_init        (void);

/**
 * bus_server_run:
 *
 * Enter the glib main loop. You have to call bus_server_init before calling this function.
 */
void         bus_server_run         (void);

/**
 * bus_server_quit:
 * @restart: TRUE if ibus-daemon restarts.
 *
 * Quit the glib main loop.
 */
void         bus_server_quit        (gboolean restart);

/**
 * bus_server_get_address:
 * @returns: The server address, e.g. "unix:abstract=/tmp/dbus-aEUnr11L,guid=8b343aaa69eabb9b282dce6f4cdbb4aa"
 *
 * Get the server address. This function might return NULL if it is called before initializing the server by
 * calling bus_server_init.
 */
const gchar *bus_server_get_address (void);

G_END_DECLS
#endif
