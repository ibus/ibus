/* -*- mode: C; c-basic-offset: 4; indent-tabs-mode: nil; -*- */
/* vim:set et sts=4: */
/* ibus - The Input Bus
 * Copyright (C) 2011 Google, Inc.
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

#include <string.h>
#include "ibus.h"

static IBusBus *bus = NULL;

static void
create_finish_success (GObject      *object,
                       GAsyncResult *res,
                       gpointer      user_data)
{
    g_assert (object == (GObject *)bus);
    g_assert (user_data == NULL);

    GError *error = NULL;
    IBusInputContext *context = NULL;
    context = ibus_bus_create_input_context_async_finish (bus, res, &error);

    g_assert (IBUS_IS_INPUT_CONTEXT (context));
    g_object_unref (context);
    ibus_quit ();
}

static void
create_finish_failed (GObject      *object,
                      GAsyncResult *res,
                      gpointer      user_data)
{
    g_assert (object == (GObject *)bus);
    g_assert (user_data == NULL);

    GError *error = NULL;
    IBusInputContext *context = NULL;
    context = ibus_bus_create_input_context_async_finish (bus, res, &error);

    g_assert (context == NULL);
    g_assert (error != NULL);
    g_debug ("error = %s", error->message);
    g_error_free (error);
    ibus_quit ();
}

static void
test_success (void)
{
    ibus_bus_create_input_context_async (bus,
                                         "test",
                                         -1,
                                         NULL,
                                         create_finish_success,
                                         NULL);
    ibus_main ();

}

static void
test_failed (void)
{
    ibus_bus_create_input_context_async (bus,
                                         "test",
                                         1000,
                                         NULL,
                                         create_finish_failed,
                                         NULL);
    GDBusConnection *connection = ibus_bus_get_connection (bus);
    g_dbus_connection_flush_sync (connection, NULL, NULL);
    g_dbus_connection_close_sync (connection, NULL, NULL);
    ibus_main ();
}

gint
main (gint    argc,
      gchar **argv)
{
    gint result;
    ibus_init ();
    g_test_init (&argc, &argv, NULL);
    bus = ibus_bus_new ();

    g_test_add_func ("/ibus/input_context_async_create_success", test_success);
    g_test_add_func ("/ibus/input_context_async_create_failed",  test_failed);

    result = g_test_run ();
    g_object_unref (bus);

    return result;
}
