/* -*- mode: C; c-basic-offset: 4; indent-tabs-mode: nil; -*- */
/* vim:set et sts=4: */
/* bus - The Input Bus
 * Copyright (C) 2010 Google Inc.
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
#include <stdlib.h>
#include <time.h>

#include <ibus.h>
#include <locale.h>
#include <glib.h>
#include "test-client.h"

#define MAX_SEND_KEY_NUM 100
#define MAX_RANDOM_SPACE 5

static gboolean
_sleep_cb (gpointer user_data)
{
    ibus_quit ();
    return FALSE;
}

static void
_sleep (guint millisecond)
{
    g_timeout_add (millisecond, (GSourceFunc) _sleep_cb, NULL);
    ibus_main ();
}

/* ibus stress test
   Send random key press and release event message to ibus-daemon.
   Key kind are a-z and space.
   Check ibus-daemon and ibus engine crash.
*/
gint
main (gint argc, gchar **argv)
{
    GTimer *timer;
    GRand *rnd;
    BusTestClient *client;
    /* num of send space key */
    guint32 seed = (guint32) time (NULL);
    int count = 0;
    int send_key_num = 0;

    setlocale (LC_ALL, "");
    ibus_init ();

    /* need to set active engine */
    client = bus_test_client_new ();
    if (client == NULL) {
        g_printerr ("don't create test-client instance.");
        exit(1);
    }

    timer = g_timer_new ();
    rnd = g_rand_new ();
    g_rand_set_seed (rnd, seed);
    g_print("random seed:%u\n",seed);
    g_timer_start (timer);

    while (1) {
        guint keysym;
        if (send_key_num > MAX_SEND_KEY_NUM) {
            break;
        }
        if (!bus_test_client_is_connected (client)) {
            g_printerr ("ibus-daemon is disconnected\n");
            break;
        }
        if (!bus_test_client_is_enabled (client)) {
            g_printerr ("ibus engine is enabled\n");
            break;
        }

        if(count>0 || g_rand_int_range (rnd, 0, 5) == 0) {
            /* send space key 20% */
            if (count == 0) {
                count = g_rand_int_range (rnd, 0, MAX_RANDOM_SPACE) + 1;
            }
            if (count-- == 1) {
                keysym = IBUS_KEY_Return;
            } else {
                keysym = IBUS_KEY_space;
            }
        } else {
            /* send random a-z key */
            keysym = g_rand_int_range (rnd, 0, 'z'-'a'+1) + 'a';
        }
        bus_test_client_send_key (client, keysym);
        send_key_num += 1;
        /* limit the typing rate to 800 hits/minutes */
        _sleep (1000 * 60 / 800);
    }

    g_print ("%f sec\n", g_timer_elapsed (timer, NULL));
    g_timer_destroy (timer);
    g_rand_free (rnd);

    return 0;
}
