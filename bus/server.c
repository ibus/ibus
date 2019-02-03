/* -*- mode: C; c-basic-offset: 4; indent-tabs-mode: nil; -*- */
/* vim:set et sts=4: */
/* bus - The Input Bus
 * Copyright (C) 2008-2010 Peng Huang <shawn.p.huang@gmail.com>
 * Copyright (C) 2008-2010 Red Hat, Inc.
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
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301
 * USA
 */
#include "server.h"

#include <gio/gio.h>
#include <stdlib.h>
#include <string.h>

#include "dbusimpl.h"
#include "ibusimpl.h"
#include "global.h"


static GDBusServer *server = NULL;
static GMainLoop *mainloop = NULL;
static BusDBusImpl *dbus = NULL;
static BusIBusImpl *ibus = NULL;
static gchar *address = NULL;
static gboolean _restart = FALSE;

static void
_restart_server (void)
{
    gchar *exe;
    gint fd;

    exe = g_strdup_printf ("/proc/%d/exe", getpid ());
    exe = g_file_read_link (exe, NULL);

    if (exe == NULL)
        exe = BINDIR "/ibus-daemon";

    /* close all fds except stdin, stdout, stderr */
    for (fd = 3; fd <= sysconf (_SC_OPEN_MAX); fd ++) {
        close (fd);
    }

    _restart = FALSE;
    execv (exe, g_argv);

    /* If the server binary is replaced while the server is running,
     * "readlink /proc/[pid]/exe" might return a path with " (deleted)"
     * suffix. */
    const gchar suffix[] = " (deleted)";
    if (g_str_has_suffix (exe, suffix)) {
        exe [strlen (exe) - sizeof (suffix) + 1] = '\0';
        execv (exe, g_argv);
    }
    g_warning ("execv %s failed!", g_argv[0]);
    exit (-1);
}

/**
 * bus_new_connection_cb:
 * @user_data: always NULL.
 * @returns: TRUE when the function can handle the connection.
 *
 * Handle incoming connections.
 */
static gboolean
bus_new_connection_cb (GDBusServer     *server,
                       GDBusConnection *dbus_connection,
                       gpointer         user_data)
{
    BusConnection *connection = bus_connection_new (dbus_connection);
    bus_dbus_impl_new_connection (dbus, connection);

    if (g_object_is_floating (connection)) {
        /* bus_dbus_impl_new_connection couldn't handle the connection. just delete the connection and return TRUE
         * (so that other connection handler will not handle the deleted connection.) */
        ibus_object_destroy ((IBusObject *)connection);
        g_object_unref (connection);
    }
    return TRUE;
}

static void
_server_connect_start_portal_cb (GObject      *source_object,
                                 GAsyncResult *res,
                                 gpointer      user_data)
{
    GVariant *result;
    GError *error = NULL;

    result = g_dbus_connection_call_finish (G_DBUS_CONNECTION (source_object),
                                            res,
                                            &error);
    if (result != NULL) {
        g_variant_unref (result);
    } else {
        g_print ("portal is not running: %s\n", error->message);
        g_error_free (error);
    }
}

static void
bus_acquired_handler (GDBusConnection *connection,
                      const gchar     *name,
                      gpointer         user_data)
{
    g_dbus_connection_call (connection,
                            IBUS_SERVICE_PORTAL,
                            IBUS_PATH_IBUS,
                            "org.freedesktop.DBus.Peer",
                            "Ping",
                            g_variant_new ("()"),
                            G_VARIANT_TYPE ("()"),
                            G_DBUS_CALL_FLAGS_NONE,
                            -1,
                            NULL /* cancellable */,
                            (GAsyncReadyCallback)
                                    _server_connect_start_portal_cb,
                            NULL);
}

void
bus_server_init (void)
{
    GError *error = NULL;

    dbus = bus_dbus_impl_get_default ();
    ibus = bus_ibus_impl_get_default ();
    bus_dbus_impl_register_object (dbus, (IBusService *)ibus);

    /* init server */
    GDBusServerFlags flags = G_DBUS_SERVER_FLAGS_AUTHENTICATION_ALLOW_ANONYMOUS;
    gchar *guid = g_dbus_generate_guid ();
    if (!g_str_has_prefix (g_address, "unix:tmpdir=") &&
        !g_str_has_prefix (g_address, "unix:path=")) {
        g_error ("Your socket address does not have the format unix:tmpdir=$DIR "
                 "or unix:path=$FILE; %s", g_address);
    }
    server =  g_dbus_server_new_sync (
                    g_address, /* the place where the socket file lives, e.g. /tmp, abstract namespace, etc. */
                    flags, guid,
                    NULL /* observer */,
                    NULL /* cancellable */,
                    &error);
    if (server == NULL) {
        g_error ("g_dbus_server_new_sync() is failed with address %s "
                 "and guid %s: %s",
                 g_address, guid, error->message);
    }
    g_free (guid);

    g_signal_connect (server, "new-connection", G_CALLBACK (bus_new_connection_cb), NULL);

    g_dbus_server_start (server);

    address = g_strdup_printf ("%s,guid=%s",
                               g_dbus_server_get_client_address (server),
                               g_dbus_server_get_guid (server));

    /* write address to file */
    ibus_write_address (address);

    /* own a session bus name so that third parties can easily track our life-cycle */
    g_bus_own_name (G_BUS_TYPE_SESSION, IBUS_SERVICE_IBUS,
                    G_BUS_NAME_OWNER_FLAGS_NONE,
                    bus_acquired_handler,
                    NULL, NULL, NULL, NULL);
}

const gchar *
bus_server_get_address (void)
{
    return address;
}

void
bus_server_run (void)
{
    g_return_if_fail (server);

    /* create and run main loop */
    mainloop = g_main_loop_new (NULL, FALSE);
    g_main_loop_run (mainloop);

    /* bus_server_quit is called. stop server */
    g_dbus_server_stop (server);

    ibus_object_destroy ((IBusObject *)dbus);
    ibus_object_destroy ((IBusObject *)ibus);

    /* release resources */
    g_object_unref (server);
    g_main_loop_unref (mainloop);
    mainloop = NULL;
    g_free (address);
    address = NULL;

    /* When _ibus_exit() is called, bus_ibus_impl_destroy() needs
     * to be called so that waitpid() prevents the processes from
     * becoming the daemons. So we run execv() after
     * ibus_object_destroy(ibus) is called here. */
    if (_restart) {
        _restart_server ();

        /* should not reach here */
        g_assert_not_reached ();
    }
}

void
bus_server_quit (gboolean restart)
{
    _restart = restart;
    if (mainloop)
        g_main_loop_quit (mainloop);
}
