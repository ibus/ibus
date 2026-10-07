/* -*- mode: C; c-basic-offset: 4; indent-tabs-mode: nil; -*- */
/* vim:set et sts=4: */
/* bus - The Input Bus
 * Copyright (C) 2010 Google Inc.
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

#ifndef __BUS_TEST_CLIENT_H_
#define __BUS_TEST_CLIENT_H_

#include <ibus.h>

/*
 * Type macros.
 */

/* define GOBJECT macros */
#define BUS_TYPE_TEST_CLIENT                  \
    (bus_test_client_get_type ())
#define BUS_TEST_CLIENT(obj)                  \
    (G_TYPE_CHECK_INSTANCE_CAST ((obj), BUS_TYPE_TEST_CLIENT, BusTestClient))
#define BUS_TEST_CLIENT_CLASS(klass)          \
    (G_TYPE_CHECK_CLASS_CAST ((klass), BUS_TYPE_TEST_CLIENT, BusTestClientClass))
#define BUS_IS_TEST_CLIENT(obj)               \
    (G_TYPE_CHECK_INSTANCE_TYPE ((obj), BUS_TYPE_TEST_CLIENT))
#define BUS_IS_TEST_CLIENT_CLASS(klass)       \
    (G_TYPE_CHECK_CLASS_TYPE ((klass), BUS_TYPE_TEST_CLIENT))
#define BUS_TEST_CLIENT_GET_CLASS(obj)        \
    (G_TYPE_CHECK_GET_CLASS ((obj), BUS_TYPE_TEST_CLIENT, BusTestClientClass))

#define MODIFIER_KEY_NUM 7

G_BEGIN_DECLS
typedef struct _BusTestClient BusTestClient;
typedef struct _BusTestClientClass BusTestClientClass;

struct _BusTestClient {
    IBusObject parent;
    /* instance members */
    IBusInputContext        *ibuscontext;
    /* modifier key state */
    gboolean                 modifier[MODIFIER_KEY_NUM];

    gint                     caps;
    /* engine is enabled */
    gboolean                 enabled;
    /* ibus-daemon is enabled */
    gboolean                 connected;
    /* private member */
};

struct _BusTestClientClass {
    IBusObjectClass parent;
    /* class members */
};

GType            bus_test_client_get_type       (void);
BusTestClient   *bus_test_client_new            (void);
gboolean         bus_test_client_is_enabled     (BusTestClient      *client);
gboolean         bus_test_client_is_connected   (BusTestClient      *client);
gboolean         bus_test_client_send_key       (BusTestClient      *client,
                                                 guint               keysym);
void             bus_test_client_clear_modifier (BusTestClient      *client);

G_END_DECLS
#endif

