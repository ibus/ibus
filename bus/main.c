/* -*- mode: C; c-basic-offset: 4; indent-tabs-mode: nil; -*- */
/* vim:set et sts=4: */
/* ibus - The Input Bus
 * Copyright (C) 2008-2013 Peng Huang <shawn.p.huang@gmail.com>
 * Copyright (C) 2013-2025 Takao Fujiwara <takao.fujiwara1@gmail.com>
 * Copyright (C) 2008-2023 Red Hat, Inc.
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
#include <config.h>
#include <fcntl.h>
#include <glib.h>
#include <gio/gio.h>
#include <ibus.h>
#include <locale.h>
#include <pwd.h>
#include <signal.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#ifdef HAVE_SYS_PRCTL_H
#include <sys/prctl.h>
#endif

#ifdef G_OS_UNIX
#include <glib-unix.h>
#include <grp.h>
#endif

#include "global.h"
#include "ibusimpl.h"
#include "server.h"

static gboolean daemonize = FALSE;
static gboolean single = FALSE;
static gboolean xim = FALSE;
static gboolean replace = FALSE;
static gboolean restart = FALSE;
static gchar *panel = "default";
static gchar *emoji_extension = "default";
static gchar *config = "default";
static gchar *desktop = "gnome";

static gchar *panel_extension_disable_users[] = {
    "gnome-initial-setup",
    "liveuser"
};
static gchar *panel_extension_disable_groups[] = {
    "gdm",
};

static void
show_version_and_quit (void)
{
    g_print ("%s - Version %s\n", g_get_application_name (), VERSION);
    exit (EXIT_SUCCESS);
}

static const GOptionEntry entries[] =
{
    { "version",   'V', G_OPTION_FLAG_NO_ARG, G_OPTION_ARG_CALLBACK, show_version_and_quit, "Show the application's version.", NULL },
    { "daemonize", 'd', 0, G_OPTION_ARG_NONE,   &daemonize, "run ibus as background process.", NULL },
    { "single",    's', 0, G_OPTION_ARG_NONE,   &single,    "do not execute panel and config module.", NULL },
    { "xim",       'x', 0, G_OPTION_ARG_NONE,   &xim,       "execute ibus XIM server.", NULL },
    { "desktop",   'n', 0, G_OPTION_ARG_STRING, &desktop,   "specify the name of desktop session. [default=gnome]", "name" },
    { "panel",     'p', 0, G_OPTION_ARG_STRING, &panel,     "specify the cmdline of panel program. pass 'disable' not to start a panel program.", "cmdline" },
    { "emoji-extension", 'E', 0, G_OPTION_ARG_STRING, &emoji_extension, "specify the cmdline of emoji extension program. pass 'disable' not to start an extension program.", "cmdline" },
    { "config",    'c', 0, G_OPTION_ARG_STRING, &config,    "specify the cmdline of config program. pass 'disable' not to start a config program.", "cmdline" },
    { "address",   'a', 0, G_OPTION_ARG_STRING, &g_address,   "specify the address of ibus daemon.", "address" },
    { "replace",   'r', 0, G_OPTION_ARG_NONE,   &replace,   "if there is an old ibus-daemon is running, it will be replaced.", NULL },
    { "cache",     't', 0, G_OPTION_ARG_STRING, &g_cache,   "specify the cache mode. [auto/refresh/none]", NULL },
    { "timeout",   'o', 0, G_OPTION_ARG_INT,    &g_gdbus_timeout, "gdbus reply timeout in milliseconds. pass -1 to use the default timeout of gdbus.", "timeout [default is 15000]" },
    { "mem-profile", 'm', 0, G_OPTION_ARG_NONE,   &g_mempro,   "enable memory profile, send SIGUSR2 to print out the memory profile.", NULL },
    { "restart",     'R', 0, G_OPTION_ARG_NONE,   &restart,    "restart panel and config processes when they die.", NULL },
    { "verbose",   'v', 0, G_OPTION_ARG_NONE,   &g_verbose,   "verbose.", NULL },
    { NULL },
};

/**
 * execute_cmdline:
 * @cmdline: An absolute path of the executable and its parameters, e.g.  "/usr/lib/ibus/ibus-x11 --kill-daemon".
 * @returns: TRUE if both parsing cmdline and executing the command succeed.
 *
 * Execute cmdline. Child process's stdin, stdout, and stderr are attached to /dev/null.
 * You don't have to handle SIGCHLD from the child process since glib will do.
 */
static gboolean
execute_cmdline (const gchar *cmdline)
{
    g_assert (cmdline);

    gint argc = 0;
    gchar **argv = NULL;
    GError *error = NULL;
    if (!g_shell_parse_argv (cmdline, &argc, &argv, &error)) {
        g_warning ("Can not parse cmdline `%s` exec: %s", cmdline, error->message);
        g_error_free (error);
        return FALSE;
    }

    error = NULL;
    gboolean retval = g_spawn_async (NULL, argv, NULL,
                            G_SPAWN_STDOUT_TO_DEV_NULL | G_SPAWN_STDERR_TO_DEV_NULL,
                            NULL, NULL,
                            NULL, &error);
    g_strfreev (argv);

    if (!retval) {
        g_warning ("Can not execute cmdline `%s`: %s", cmdline, error->message);
        g_error_free (error);
        return FALSE;
    }

    return TRUE;
}

#ifndef HAVE_DAEMON
static void
closeall (gint fd)
{
    gint fdlimit = sysconf(_SC_OPEN_MAX);

    while (fd < fdlimit) {
      close(fd++);
    }
}

static gint
daemon (gint nochdir, gint noclose)
{
    switch (fork()) {
        case 0:  break;
        case -1: return -1;
        default: _exit(0);
    }

    if (setsid() < 0) {
      return -1;
    }

    switch (fork()) {
        case 0:  break;
        case -1: return -1;
        default: _exit(0);
    }

    if (!nochdir) {
      chdir("/");
    }

    if (!noclose) {
        closeall(0);
        open("/dev/null",O_RDWR);
        dup(0); dup(0);
    }
    return 0;
}
#endif

#ifdef HAVE_SYS_PRCTL_H
static gboolean
_on_sigusr1 (void)
{
    g_warning ("The parent process died.");
    bus_server_quit (FALSE);
    return G_SOURCE_REMOVE;
}
#endif

static inline void
exit_and_free_context (int             status,
                       GOptionContext *context)
{
    g_option_context_free (context);
    exit (status);
}

gint
main (gint argc, gchar **argv)
{
    int i;
    const gchar *username = ibus_get_user_name ();
    const gchar *groupname = NULL;
#ifdef HAVE_GETGRGID_R
    char buffer[4096];
    struct group gbuf;
#endif

    setlocale (LC_ALL, "");

    GOptionContext *context = g_option_context_new ("- ibus daemon");
    g_option_context_add_main_entries (context, entries, "ibus-daemon");

    g_argv = g_strdupv (argv);
    GError *error = NULL;
    if (!g_option_context_parse (context, &argc, &argv, &error)) {
        g_printerr ("Option parsing failed: %s\n", error->message);
	g_error_free (error);
        exit_and_free_context (EXIT_FAILURE, context);
    }
    if (g_gdbus_timeout < -1) {
        g_printerr ("Bad timeout (must be >= -1): %d\n", g_gdbus_timeout);
        exit_and_free_context (EXIT_FAILURE, context);
    }

    if (g_mempro) {
        g_warning ("--mem-profile no longer works with the GLib 2.46 or later");
    }

    /* check uid */
    {
        struct passwd *pwd = getpwuid (getuid ());

        if (pwd == NULL || g_strcmp0 (pwd->pw_name, username) != 0) {
            g_printerr ("Please run ibus-daemon with login user! Do not run ibus-daemon with sudo or su.\n");
            exit_and_free_context (EXIT_FAILURE, context);
        }
    }

    /* get group name */
    {
        struct group *grp = NULL;
#ifdef HAVE_GETGRGID_R
        /* MT-Safe locale */
        getgrgid_r (getgid (), &gbuf, buffer, sizeof(buffer), &grp);
#else
        /* MT-Unsafe race:grgid locale */
        grp = getgrgid (getgid ());
#endif

        if (grp && grp->gr_name && grp->gr_name[0])
            groupname = grp->gr_name;
        else
            g_warning ("Couldn't get group name");
    }

    /* daemonize process */
    if (daemonize) {
        if (daemon (1, 0) != 0) {
            g_printerr ("Cannot daemonize ibus.\n");
            exit_and_free_context (EXIT_FAILURE, context);
        }
    }

    /* create a new process group. this is important to kill all of its children by SIGTERM at a time in bus_ibus_impl_destroy. */
    setpgid (0, 0);

    ibus_init ();

#ifndef HAVE_PRODUCT_BUILD
    g_setenv ("PYTHONWARNINGS", "always", FALSE);
#endif

    ibus_set_log_handler (g_verbose);

    /* check if ibus-daemon is running in this session */
    if (ibus_get_address () != NULL) {
        IBusBus *bus = ibus_bus_new ();

        if (ibus_bus_is_connected (bus)) {
            if (!replace) {
                g_printerr ("current session already has an ibus-daemon.\n");
                exit_and_free_context (EXIT_FAILURE, context);
            }
            ibus_bus_exit (bus, FALSE);
            while (ibus_bus_is_connected (bus)) {
                g_main_context_iteration (NULL, TRUE);
            }
        }
        g_object_unref (bus);
    }

    bus_server_init ();
    for (i = 0; i < G_N_ELEMENTS(panel_extension_disable_users); i++) {
        if (!g_strcmp0 (username, panel_extension_disable_users[i]) != 0) {
            emoji_extension = "disable";
            break;
        }
    }
    for (i = 0; i < G_N_ELEMENTS(panel_extension_disable_groups); i++) {
        if (g_strcmp0 (groupname, panel_extension_disable_groups[i]) == 0) {
            emoji_extension = "disable";
            break;
        }
    }
    if (!single) {
        /* execute config component */
        if (g_strcmp0 (config, "default") == 0) {
            BusComponent *component;
            component = bus_ibus_impl_lookup_component_by_name (
                    BUS_DEFAULT_IBUS, IBUS_SERVICE_CONFIG);
            if (component) {
                bus_component_set_restart (component, restart);
            }
            if (component == NULL || !bus_component_start (component, g_verbose)) {
                g_printerr ("Can not execute default config program\n");
                exit_and_free_context (EXIT_FAILURE, context);
            }
        } else if (g_strcmp0 (config, "disable") != 0 && g_strcmp0 (config, "") != 0) {
            if (!execute_cmdline (config))
                exit_and_free_context (EXIT_FAILURE, context);
        }

        /* execute panel component */
        if (g_strcmp0 (panel, "default") == 0) {
            BusComponent *component;
            component = bus_ibus_impl_lookup_component_by_name (
                    BUS_DEFAULT_IBUS, IBUS_SERVICE_PANEL);
            if (component) {
                bus_component_set_restart (component, restart);
            }
            if (component == NULL || !bus_component_start (component, g_verbose)) {
                g_printerr ("Can not execute default panel program\n");
                exit_and_free_context (EXIT_FAILURE, context);
            }
        } else if (g_strcmp0 (panel, "disable") != 0 && g_strcmp0 (panel, "") != 0) {
            if (!execute_cmdline (panel))
                exit_and_free_context (EXIT_FAILURE, context);
        }
    }

#ifdef EMOJI_DICT
    if (g_strcmp0 (emoji_extension, "default") == 0) {
        BusComponent *component;
        component = bus_ibus_impl_lookup_component_by_name (
                BUS_DEFAULT_IBUS, IBUS_SERVICE_PANEL_EXTENSION);
        if (component) {
            bus_component_set_restart (component, restart);
        }
        if (component != NULL &&
            !bus_component_start (component, g_verbose)) {
            g_printerr ("Can not execute default panel program\n");
            exit_and_free_context (EXIT_FAILURE, context);
        }
    } else if (g_strcmp0 (emoji_extension, "disable") != 0 &&
               g_strcmp0 (emoji_extension, "") != 0) {
        if (!execute_cmdline (emoji_extension))
            exit_and_free_context (EXIT_FAILURE, context);
    }
#endif

    /* execute ibus xim server */
    if (xim) {
        if (!execute_cmdline (LIBEXECDIR "/ibus-x11 --kill-daemon"))
            exit_and_free_context (EXIT_FAILURE, context);
    }

    if (!daemonize) {
        if (getppid () == 1) {
            g_warning ("The parent process died.");
            exit_and_free_context (EXIT_SUCCESS, context);
        }
#ifdef HAVE_SYS_PRCTL_H
#ifdef G_OS_UNIX
       /* Currently ibus-x11 detects XIOError and assume the error as the
        * desktop session is closed and ibus-x11 calls Exit D-Bus method to
        * exit ibus-daemon. But a few desktop sessions cause XError before
        * XIOError and GTK does not allow to bind XError by applications and
        * GTK calls gdk_x_error() with XError.
        *
        * E.g. GdkX11Screen calls XGetSelectionOwner() for "_XSETTINGS_S?"
        * atoms during the logout but the selection owner already becomes
        * NULL and the NULL window causes XError with
        * gdk_x11_window_foreign_new_for_display().
        *
        * Since ibus-x11 exits with XError before XIOError, gnome-shell
        * can detects the exit of ibus-daemon a little earlier and
        * gnome-shell restarts ibus-daemon but gnome-shell dies soon.
        * Then gnome-shell dies but ibus-daemon is alive, it's a problem.
        * Because it causes double ibus-x11 of GDM and a login user
        * and double XSetSelectionOwner() is not allowed for the unique
        * "ibus" atom and the user cannot use XIM but not GtkIMModule.
        *
        * Probably we could fix the ibus process problem if we would fix
        * XError about the X selection owner or stop to restart ibus-daemon
        * in gonme-shell when the session is logging out.
        * Maybe using SessionManager.LogoutRemote() or
        * global.screen.get_display().get_xdisplay()
        * But I assume there are other scenarios to cause the problem.
        *
        * And I decided ibus-daemon always exits with the parent's death here
        * to avoid unexpected ibus restarts during the logout.
        */
        if (prctl (PR_SET_PDEATHSIG, SIGUSR1))
            g_printerr ("Cannot bind SIGUSR1 for parent death\n");
        else
            g_unix_signal_add (SIGUSR1, (GSourceFunc)_on_sigusr1, NULL);
#endif
#endif
    }
    bus_server_run ();
    g_option_context_free (context);
    return 0;
}
