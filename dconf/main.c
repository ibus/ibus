/* -*- mode: C; c-basic-offset: 4; indent-tabs-mode: nil; -*- */
/* vim:set et sts=4: */
/* ibus - The Input Bus
 * Copyright (C) 2008-2010 Peng Huang <shawn.p.huang@gmail.com>
 * Copyright (C) 2008-2010 Red Hat, Inc.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#include <ibus.h>
#include <stdlib.h>
#include <locale.h>
#include "config-private.h"

static IBusBus *bus = NULL;
static IBusConfigDConf *config = NULL;

/* options */
static gboolean ibus = FALSE;
static gboolean verbose = FALSE;

static const GOptionEntry entries[] =
{
    { "ibus", 'i', 0, G_OPTION_ARG_NONE, &ibus, "component is executed by ibus",
      NULL },
    { "verbose", 'v', 0, G_OPTION_ARG_NONE, &verbose, "verbose", NULL },
    { NULL },
};


static void
ibus_disconnected_cb (IBusBus  *bus,
                      gpointer  user_data)
{
    ibus_quit ();
}

static void
ibus_dconf_start (void)
{
    ibus_init ();
    bus = ibus_bus_new ();
    if (!ibus_bus_is_connected (bus)) {
        exit (1);
    }
    g_signal_connect (bus, "disconnected", G_CALLBACK (ibus_disconnected_cb),
                      NULL);
    config = ibus_config_dconf_new (ibus_bus_get_connection (bus));
    ibus_bus_request_name (bus, IBUS_SERVICE_CONFIG, 0);
    ibus_main ();
}

gint
main (gint argc, gchar **argv)
{
    GError *error = NULL;
    GOptionContext *context;

    setlocale (LC_ALL, "");

    context = g_option_context_new ("- ibus dconf component");

    g_option_context_add_main_entries (context, entries, "ibus-dconf");

    if (!g_option_context_parse (context, &argc, &argv, &error)) {
        g_print ("Option parsing failed: %s\n", error->message);
        exit (-1);
    }

    ibus_set_log_handler (verbose);
    ibus_dconf_start ();

    return 0;
}
