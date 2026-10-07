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

static IBusBus *bus;
static void
call_next_async_function (IBusInputContext *context);

static gboolean
fatal_handler(const gchar *log_domain,
              GLogLevelFlags log_level,
              const gchar *message,
              gpointer user_data)
{
    if (!g_strcmp0 (message, "org.freedesktop.IBus.InputContext.GetEngine: GDBus.Error:org.freedesktop.DBus.Error.Failed: Input context does not have engine."))
        return FALSE; /* do not call abort. */
    return TRUE;
}

static void
call_basic_ipcs (IBusInputContext *context)
{
    ibus_input_context_set_cursor_location (context, 0, 0, 1, 1);
    ibus_input_context_set_capabilities (context, IBUS_CAP_FOCUS);
    ibus_input_context_property_activate (context, "dummy.prop.name", PROP_STATE_CHECKED);
    ibus_input_context_reset (context);

    /* When enable() is called, ibus-daemon may start a global (or preloaded,
     * or default) engine in an asynchronous manner and return immediately.
     * Therefore, it is not guaranteed that ibus_input_context_is_enabled()
     * returns TRUE. */

    ibus_input_context_focus_in (context);
}

static void
test_input_context (void)
{
    IBusInputContext *context;
    GLogLevelFlags flags;
    IBusEngineDesc *engine_desc;
    gchar *active_engine_name = NULL;
    gchar *current_ic;

    context = ibus_bus_create_input_context (bus, "test");
    call_basic_ipcs (context);

    /* "No global engine." warning is not critical message. */
    flags = g_log_set_always_fatal (G_LOG_LEVEL_CRITICAL);
    engine_desc = ibus_bus_get_global_engine (bus);
    g_log_set_always_fatal (flags);
    if (engine_desc != NULL) {
        active_engine_name = g_strdup (ibus_engine_desc_get_name(engine_desc));
        g_object_unref (engine_desc);
        engine_desc = NULL;
    } else {
        active_engine_name = g_strdup ("dummy");
    }
    g_assert (active_engine_name);
    g_debug ("Use '%s' for testing.", active_engine_name);

    ibus_input_context_set_engine (context, active_engine_name);
    current_ic = ibus_bus_current_input_context (bus);
    g_assert (!strcmp (current_ic, g_dbus_proxy_get_object_path ((GDBusProxy *)context)));

    g_test_log_set_fatal_handler (fatal_handler, NULL);
    engine_desc = ibus_input_context_get_engine (context);
    if (engine_desc) {
        /* FIXME race condition between ibus_input_context_set_engine and _get_engine.
         * ibus_input_context_get_engine is not guaranteed to return non-NULL
         * (even if we use synchronous set_engine()) because ibus-daemon sets a context
         * engine in an asynchronous manner. See _context_request_engine_cb in
         * ibusimpl.c which handles context_signals[REQUEST_ENGINE] signal. */
        g_assert (!strcmp (active_engine_name, ibus_engine_desc_get_name(engine_desc)));
        g_object_unref (engine_desc);
        engine_desc = NULL;
    }
    ibus_input_context_process_key_event (context, 0, 0, 0);

    /* An engine is set. Try to call basic IPCs again. */
    call_basic_ipcs (context);

    g_free (current_ic);
    g_object_unref (context);

    g_free (active_engine_name);
}

static void
finish_get_engine_async (GObject *source_object,
                         GAsyncResult *res,
                         gpointer user_data)
{
    IBusInputContext *context = IBUS_INPUT_CONTEXT (source_object);
    GError *error = NULL;
    IBusEngineDesc *desc = ibus_input_context_get_engine_async_finish (context,
                                                                       res,
                                                                       &error);
    if (desc) {
        g_object_unref (desc);
    }
    g_debug ("ibus_context_get_engine_async_finish: OK");
    call_next_async_function (context);
}

static void
start_get_engine_async (IBusInputContext *context)
{
    ibus_input_context_get_engine_async (context,
                                         -1, /* timeout */
                                         NULL, /* cancellable */
                                         finish_get_engine_async,
                                         NULL); /* user_data */
}

static void
finish_process_key_event_async (GObject *source_object,
                         GAsyncResult *res,
                         gpointer user_data)
{
    IBusInputContext *context = IBUS_INPUT_CONTEXT (source_object);
    GError *error = NULL;
    gboolean result = ibus_input_context_process_key_event_async_finish (context,
                                                                         res,
                                                                         &error);
    g_assert (result || error == NULL);
    g_debug ("ibus_context_process_key_event_async_finish: OK");
    call_next_async_function (context);
}

static void
start_process_key_event_async (IBusInputContext *context)
{
    ibus_input_context_process_key_event_async (context,
                                                0, 0, 0,
                                                -1, /* timeout */
                                                NULL, /* cancellable */
                                                finish_process_key_event_async,
                                                NULL); /* user_data */
}

static gboolean
test_async_apis_finish (gpointer user_data)
{
    ibus_quit ();
    return FALSE;
}

static void
test_async_apis (void)
{
    g_debug ("start");
    IBusInputContext *context;
    context = ibus_bus_create_input_context (bus, "test");
    call_basic_ipcs (context);

    call_next_async_function (context);
    ibus_main ();
}

static void
call_next_async_function (IBusInputContext *context)
{
    static void (*async_functions[])(IBusInputContext *) = {
        start_get_engine_async,
        start_process_key_event_async,
    };
    static guint index = 0;

    // Use g_timeout_add to make sure test_async_apis finishes even if async_functions is empty.
    if (index >= G_N_ELEMENTS (async_functions))
        g_timeout_add (1, test_async_apis_finish, NULL);
    else
        (*async_functions[index++])(context);
}

gint
main (gint    argc,
      gchar **argv)
{
    gint result;
    ibus_init ();
    g_test_init (&argc, &argv, NULL);
    bus = ibus_bus_new ();

    g_test_add_func ("/ibus/input_context", test_input_context);
    g_test_add_func ("/ibus/input_context_async_with_callback", test_async_apis);

    result = g_test_run ();
    g_object_unref (bus);

    return result;
}
