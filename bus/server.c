/* -*- mode: C; c-basic-offset: 4; indent-tabs-mode: nil; -*- */
/* vim:set et sts=4: */
/* bus - The Input Bus
 * Copyright (C) 2008-2010 Peng Huang <shawn.p.huang@gmail.com>
 * Copyright (C) 2011-2019 Takao Fujiwara <takao.fujiwara1@gmail.com>
 * Copyright (C) 2008-2019 Red Hat, Inc.
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

#include <errno.h>
#include <glib/gstdio.h>
#include <gio/gio.h>
#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <errno.h>
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
    ssize_t r;
    int MAXSIZE = 0xFFF;
    char proclnk[MAXSIZE];
    char filename[MAXSIZE];

    exe = g_strdup_printf ("/proc/%d/exe", getpid ());
    exe = g_file_read_link (exe, NULL);

    if (exe == NULL)
        exe = BINDIR "/ibus-daemon";

    /* close all fds except stdin, stdout, stderr */
    for (fd = 3; fd <= sysconf (_SC_OPEN_MAX); fd ++) {
        errno = 0;
        /* only close valid fds */
        if (fcntl(fd, F_GETFD) != -1 || errno != EBADF) {
            sprintf(proclnk, "/proc/self/fd/%d", fd);
            r = readlink(proclnk, filename, MAXSIZE);
            if (r < 0) {
                continue;
            }
            filename[r] = '\0';

            /* Do not close 'anon_inode:inotify' fds, that may crash in glib */
            if (strcmp(filename, "anon_inode:inotify") != 0) {
                close (fd);
            }
        }
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
 * bus_allow_mechanism_cb:
 * @observer: A #GDBusAuthObserver.
 * @mechanism: The name of the mechanism.
 * @user_data: always %NULL.
 *
 * Check if @mechanism can be used to authenticate the other peer.
 * Returns: %TRUE if the peer's mechanism is allowed.
 */
static gboolean
bus_allow_mechanism_cb (GDBusAuthObserver     *observer,
                        const gchar           *mechanism,
                        G_GNUC_UNUSED gpointer user_data)
{
    if (g_strcmp0 (mechanism, "EXTERNAL") == 0)
        return TRUE;
    return FALSE;
}

/**
 * bus_authorize_authenticated_peer_cb:
 * @observer: A #GDBusAuthObserver.
 * @stream: A #GIOStream.
 * @credentials: A #GCredentials.
 * @user_data: always %NULL.
 *
 * Check if a peer who has already authenticated should be authorized.
 * Returns: %TRUE if the peer's credential is authorized.
 */
static gboolean
bus_authorize_authenticated_peer_cb (GDBusAuthObserver     *observer,
                                     GIOStream             *stream,
                                     GCredentials          *credentials,
                                     G_GNUC_UNUSED gpointer user_data)
{
    gboolean authorized = FALSE;
    if (credentials) {
        GCredentials *own_credentials = g_credentials_new ();
        if (g_credentials_is_same_user (credentials, own_credentials, NULL))
            authorized = TRUE;
        g_object_unref (own_credentials);
    }
    return authorized;
}

/**
 * bus_new_connection_cb:
 * @observer: A #GDBusAuthObserver.
 * @dbus_connection: A #GDBusconnection.
 * @user_data: always %NULL.
 *
 * Handle incoming connections.
 * Returns: %TRUE when the function can handle the connection.
 */
static gboolean
bus_new_connection_cb (GDBusServer           *server,
                       GDBusConnection       *dbus_connection,
                       G_GNUC_UNUSED gpointer user_data)
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
_server_connect_start_portal_cb (GObject               *source_object,
                                 GAsyncResult          *res,
                                 G_GNUC_UNUSED gpointer user_data)
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
bus_acquired_handler (GDBusConnection       *connection,
                      const gchar           *name,
                      G_GNUC_UNUSED gpointer user_data)
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

static gchar *
_bus_extract_address (void)
{
    gchar *socket_address = g_strdup (g_address);
    gchar *p;

#define IF_REPLACE_VARIABLE_WITH_FUNC(variable, func, format)           \
    if ((p = g_strstr_len (socket_address, -1, (variable)))) {          \
        gchar *sub1 = g_strndup (socket_address, p - socket_address);   \
        gchar *sub2 = g_strdup (p + strlen (variable));                 \
        gchar *tmp = g_strdup_printf ("%s" format "%s",                 \
                                      sub1, (func) (), sub2);           \
        g_free (sub1);                                                  \
        g_free (sub2);                                                  \
        g_free (socket_address);                                        \
        socket_address = tmp;                                           \
    }

    IF_REPLACE_VARIABLE_WITH_FUNC ("$XDG_RUNTIME_DIR",
                                   g_get_user_runtime_dir,
                                   "%s")
    else
    IF_REPLACE_VARIABLE_WITH_FUNC ("$XDG_CACHE_HOME",
                                   g_get_user_cache_dir,
                                   "%s")
    else
    IF_REPLACE_VARIABLE_WITH_FUNC ("$UID", getuid, "%d")

#undef IF_REPLACE_VARIABLE_WITH_FUNC

    return socket_address;
}

void
bus_server_init (void)
{
#define IBUS_UNIX_TMPDIR        "unix:tmpdir="
#define IBUS_UNIX_PATH          "unix:path="
#define IBUS_UNIX_ABSTRACT      "unix:abstract="
#define IBUS_UNIX_DIR           "unix:dir="

    gchar *socket_address;
    GDBusServerFlags flags = G_DBUS_SERVER_FLAGS_NONE;
    gchar *guid;
    GDBusAuthObserver *observer;
    GError *error = NULL;
    gchar *unix_dir = NULL;

    dbus = bus_dbus_impl_get_default ();
    ibus = bus_ibus_impl_get_default ();
    bus_dbus_impl_register_object (dbus, (IBusService *)ibus);

    /* init server */
    socket_address = _bus_extract_address ();

#define IF_GET_UNIX_DIR(prefix)                                         \
    if (g_str_has_prefix (socket_address, (prefix))) {                  \
        unix_dir = g_strdup (socket_address + strlen (prefix));         \
    }

    IF_GET_UNIX_DIR (IBUS_UNIX_TMPDIR)
    else
    IF_GET_UNIX_DIR (IBUS_UNIX_PATH)
    else
    IF_GET_UNIX_DIR (IBUS_UNIX_ABSTRACT)
    else
    IF_GET_UNIX_DIR (IBUS_UNIX_DIR)
    else {
        g_error ("Your socket address \"%s\" does not correspond with "
                 "one of the following formats; "
                 IBUS_UNIX_TMPDIR "DIR, " IBUS_UNIX_PATH "FILE, "
                 IBUS_UNIX_ABSTRACT "FILE, " IBUS_UNIX_DIR "DIR.",
                 socket_address);
    }
    if (!g_file_test (unix_dir, G_FILE_TEST_EXISTS | G_FILE_TEST_IS_DIR)) {
        /* Require mkdir for BSD system. */
        if (g_mkdir_with_parents (unix_dir, 0) != 0) {
            g_error ("mkdir is failed in: %s: %s",
                     unix_dir, g_strerror (errno));
        }
        /* The mode 0700 can eliminate malicious users change the mode.
         * `chmod` runs for the last directory only not to change the modes
         * of the parent directories. E.g. "/tmp/ibus".
         */
        if (g_chmod (unix_dir, 0700) != 0) {
            g_error ("chmod(700) is failed in: %s: %s",
                     unix_dir, g_strerror (errno));
        }
    }
    g_free (unix_dir);
    guid = g_dbus_generate_guid ();
    observer = g_dbus_auth_observer_new ();
    server =  g_dbus_server_new_sync (
                    /* the place where the socket file lives, e.g. /tmp,
                     * abstract namespace, etc. */
                    socket_address,
                    flags, guid,
                    observer,
                    NULL /* cancellable */,
                    &error);
    if (server == NULL) {
        g_error ("g_dbus_server_new_sync() is failed with address %s "
                 "and guid %s: %s",
                 socket_address, guid, error->message);
    }
    g_free (socket_address);
    g_free (guid);

    g_signal_connect (observer, "allow-mechanism",
                      G_CALLBACK (bus_allow_mechanism_cb), NULL);
    g_signal_connect (observer, "authorize-authenticated-peer",
                      G_CALLBACK (bus_authorize_authenticated_peer_cb), NULL);
    g_object_unref (observer);
    g_signal_connect (server, "new-connection",
                      G_CALLBACK (bus_new_connection_cb), NULL);

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

#undef IF_GET_UNIX_DIR
#undef IBUS_UNIX_TMPDIR
#undef IBUS_UNIX_PATH
#undef IBUS_UNIX_ABSTRACT
#undef IBUS_UNIX_DIR
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
