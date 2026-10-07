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

#include <ibus.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>

#include "test-client.h"

#ifdef DEBUG
#  define IDEBUG g_debug
#else
#  define IDEBUG(a...)
#endif
/* functions prototype */
static void          bus_test_client_class_init      (BusTestClientClass *class);
static void          bus_test_client_destroy         (IBusObject         *object);

/* static methods*/
static void          _store_modifier_state          (BusTestClient      *client,
                                                     guint               modifier);
static gboolean      _is_shift_set                  (BusTestClient      *client);
static gboolean      _is_modifier_set               (BusTestClient      *client,
                                                     guint               modifier);
static gboolean      _is_modifier_key               (guint               modifier);
static guint         _get_modifiers_to_mask         (BusTestClient      *client);
static gint16        _get_keysym_to_keycode         (guint               keysym);

static void          _bus_disconnected_cb           (IBusBus            *ibusbus,
                                                     BusTestClient      *client);
static void          _bus_disabled_cb               (IBusInputContext   *ibuscontext,
                                                     BusTestClient      *client);

static IBusBus      *_bus = NULL;
static Display      *_xdisplay = NULL;


G_DEFINE_TYPE (BusTestClient, bus_test_client, IBUS_TYPE_OBJECT)

static void
bus_test_client_class_init (BusTestClientClass *class)
{
    IDEBUG ("%s", __FUNCTION__);

    IBusObjectClass *ibus_object_class = IBUS_OBJECT_CLASS (class);

    ibus_object_class->destroy = bus_test_client_destroy;

    /* init display object */
    if (_xdisplay == NULL) {
        _xdisplay = XOpenDisplay (NULL);
    }

    /* init bus object */
    if (_bus == NULL) {
        ibus_set_display (XDisplayString (_xdisplay));
        _bus = ibus_bus_new();
    }

    g_signal_connect (_bus, "disconnected", G_CALLBACK (_bus_disconnected_cb), NULL);
}

static void
bus_test_client_init (BusTestClient *client)
{
    IDEBUG ("%s", __FUNCTION__);
    client->connected = FALSE;
    client->enabled = FALSE;

    g_return_if_fail (ibus_bus_is_connected (_bus));
    client->connected = TRUE;

    client->ibuscontext = ibus_bus_create_input_context (_bus, "test-client");

    g_return_if_fail (client->ibuscontext != NULL);

    g_signal_connect (client->ibuscontext,
                      "disabled",
                      G_CALLBACK (_bus_disabled_cb),
                      client);

    bus_test_client_clear_modifier (client);

    client->caps = IBUS_CAP_FOCUS;
    ibus_input_context_set_capabilities (client->ibuscontext, client->caps);

    ibus_bus_set_global_engine (_bus, "xkb:us::eng");

    client->enabled = TRUE;
}

static void
bus_test_client_destroy (IBusObject *object)
{
    IDEBUG ("%s", __FUNCTION__);
    BusTestClient *client = BUS_TEST_CLIENT (object);

    g_object_unref (client->ibuscontext);
}

BusTestClient *
bus_test_client_new (void)
{
    IDEBUG ("%s", __FUNCTION__);
    BusTestClient *client = BUS_TEST_CLIENT (g_object_new (BUS_TYPE_TEST_CLIENT, NULL));

    if (client->connected && client->enabled) {
        return client;
    } else {
        return NULL;
    }
}

gboolean
bus_test_client_is_enabled (BusTestClient *client)
{
    IDEBUG ("%s", __FUNCTION__);
    return client->enabled;
}

gboolean
bus_test_client_is_connected (BusTestClient *client)
{
    IDEBUG ("%s", __FUNCTION__);
    return client->connected;
}

gboolean
bus_test_client_send_key (BusTestClient *client,
                     guint      keysym)
{
    gboolean is_modifier = _is_modifier_key (keysym);
    gint16 keycode;
    guint state;

    if (is_modifier) {
        IDEBUG ("key: %d is modifier.", keysym);
        gboolean is_modifier_set = _is_modifier_set (client, keysym);
        keycode = _get_keysym_to_keycode (keysym);
        state = _get_modifiers_to_mask (client);

        if (is_modifier_set) {
            state |= IBUS_RELEASE_MASK;
        }
        ibus_input_context_process_key_event (client->ibuscontext,
                                              keysym,
                                              keycode,
                                              state);
        _store_modifier_state (client, keysym);
    } else {
        IDEBUG ("key: %d is not modifier.", keysym);
        /* This is an example code. If you use the keysym >= 0x01000000,
         * gdk_keyval_is_upper may be useful since 
         * XConvertCase supports implementation-independent conversions.
         */
        KeySym xlower = 0;
        KeySym xupper = 0;
        gboolean is_upper = FALSE;

        if (keysym) {
            XConvertCase (keysym, &xlower, &xupper);
            is_upper = ((guint) xupper == keysym);
        }

        gboolean is_shift_set = _is_shift_set (client);

        if (is_upper && !is_shift_set) {
            _store_modifier_state (client, IBUS_KEY_Shift_L);
        }
        keycode = _get_keysym_to_keycode (keysym);
        state = _get_modifiers_to_mask (client);
        ibus_input_context_process_key_event (client->ibuscontext,
                                              keysym,
                                              keycode,
                                              state);
        state |= IBUS_RELEASE_MASK;
        ibus_input_context_process_key_event (client->ibuscontext,
                                              keysym,
                                              keycode,
                                              state);
        if (is_upper && !is_shift_set) {
            _store_modifier_state (client, IBUS_KEY_Shift_L);
        }
    }
    return TRUE;
}

void bus_test_client_clear_modifier (BusTestClient *client)
{
    int i;
    for (i = 0; i < MODIFIER_KEY_NUM; i++) {
        (client->modifier)[i] = FALSE;
    }
}

static void
_store_modifier_state (BusTestClient    *client,
                       guint         modifier)
{
    switch(modifier) {
    case IBUS_KEY_Shift_L:
    case IBUS_KEY_Shift_R:
        /* ShiftMask */
        client->modifier[0] = !client->modifier[0];
        break;
    case IBUS_KEY_Shift_Lock:
    case IBUS_KEY_Caps_Lock:
        /* LockMask */
        client->modifier[1] = !client->modifier[1];
        break;
    case IBUS_KEY_Control_L:
    case IBUS_KEY_Control_R:
        /* ControlMask */
        client->modifier[2] = !client->modifier[2];
        break;
    case IBUS_KEY_Alt_L:
    case IBUS_KEY_Alt_R:
    case IBUS_KEY_Meta_L:
        /* Mod1Mask */
        client->modifier[3] = !client->modifier[3];
        break;
    case IBUS_KEY_Num_Lock:
        /* Mod2Mask */
        client->modifier[4] = !client->modifier[4];
        break;
    case IBUS_KEY_Super_L:
    case IBUS_KEY_Hyper_L:
        /* Mod4Mask */
        client->modifier[5] = !client->modifier[5];
        break;
    case IBUS_KEY_ISO_Level3_Shift:
    case IBUS_KEY_Mode_switch:
        /* Mod5Mask */
        client->modifier[6] = !client->modifier[6];
        break;
    default:
        break;
    }
}

static gint16
_get_keysym_to_keycode (guint keysym)
{
    return XKeysymToKeycode (_xdisplay, keysym);
}

static gboolean
_is_shift_set (BusTestClient *client)
{
    return client->modifier[0];
}

static gboolean
_is_modifier_set (BusTestClient *client,
                  guint      modifier)
{
    switch(modifier) {
    case IBUS_KEY_Shift_L:
    case IBUS_KEY_Shift_R:
        /* ShiftMask */
        return client->modifier[0];
    case IBUS_KEY_Shift_Lock:
    case IBUS_KEY_Caps_Lock:
        /* LockMask */
        return client->modifier[1];
    case IBUS_KEY_Control_L:
    case IBUS_KEY_Control_R:
        /* ControlMask */
        return client->modifier[2];
    case IBUS_KEY_Alt_L:
    case IBUS_KEY_Alt_R:
    case IBUS_KEY_Meta_L:
        /* Mod1Mask */
        return client->modifier[3];
    case IBUS_KEY_Num_Lock:
        /* Mod2Mask */
        return client->modifier[4];
    case IBUS_KEY_Super_L:
    case IBUS_KEY_Hyper_L:
        /* Mod4Mask */
        return client->modifier[5];
    case IBUS_KEY_ISO_Level3_Shift:
    case IBUS_KEY_Mode_switch:
        /* Mod5Mask */
        return client->modifier[6];
    default:
        return FALSE;
    }
}

static gboolean
_is_modifier_key (guint modifier)
{
    switch(modifier) {
    case IBUS_KEY_Shift_L:
    case IBUS_KEY_Shift_R:
    case IBUS_KEY_Shift_Lock:
    case IBUS_KEY_Caps_Lock:
    case IBUS_KEY_Control_L:
    case IBUS_KEY_Control_R:
    case IBUS_KEY_Alt_L:
    case IBUS_KEY_Alt_R:
    case IBUS_KEY_Meta_L:
    case IBUS_KEY_Num_Lock:
    case IBUS_KEY_Super_L:
    case IBUS_KEY_Hyper_L:
    case IBUS_KEY_ISO_Level3_Shift:
    case IBUS_KEY_Mode_switch:
        return TRUE;
    default:
        return FALSE;
    }
}

static guint
_get_modifiers_to_mask (BusTestClient *client)
{
    guint retval = 0;
    if(client->modifier[0])
        retval |= IBUS_SHIFT_MASK;
    if(client->modifier[1])
        retval |= IBUS_LOCK_MASK;
    if(client->modifier[2])
        retval |= IBUS_CONTROL_MASK;
    if(client->modifier[3])
        retval |= IBUS_MOD1_MASK;
    if(client->modifier[4])
        retval |= IBUS_MOD2_MASK;
    if(client->modifier[5])
        retval |= IBUS_MOD4_MASK;
    if(client->modifier[6])
        retval |= IBUS_MOD5_MASK;
    return retval;
}

static void
_bus_disconnected_cb (IBusBus   *ibusbus,
                      BusTestClient *client)
{
    g_assert (IBUS_IS_BUS (ibusbus));
    g_assert (BUS_IS_TEST_CLIENT (client));
    IDEBUG ("%s", __FUNCTION__);
    client->connected = FALSE;
    IDEBUG ("Disconnected ibus daemon");
}

static void
_bus_disabled_cb (IBusInputContext  *ibuscontext,
                  BusTestClient         *client)
{
    g_assert (IBUS_IS_INPUT_CONTEXT (ibuscontext));
    g_assert (BUS_IS_TEST_CLIENT (client));
    IDEBUG ("%s", __FUNCTION__);
    client->enabled = FALSE;
    IDEBUG ("Disabled ibus engine");
}

