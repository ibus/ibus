/* -*- mode: C; c-basic-offset: 4; indent-tabs-mode: nil; -*- */
/* vim:set et sts=4: */
/* ibus - The Input Bus
 * Copyright (C) 2019-2023 Takao Fujiwara <takao.fujiwara1@gmail.com>
 * Copyright (C) 2013 Intel Corporation
 * Copyright (C) 2013-2023 Red Hat, Inc.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License
 * as published by the Free Software Foundation; either version 2.1 of
 * the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
 * 02110-1301 USA.
 */

#include "config.h"

#include <errno.h>
#include <glib-object.h>
#include <ibus.h>
#include <string.h>
#include <wayland-client.h>
#include <xkbcommon/xkbcommon.h>

#include "input-method-unstable-v1-client-protocol.h"
#include "ibuswaylandim.h"

struct _IBusWaylandIMPrivate
{
    struct zwp_input_method_v1 *input_method;
    struct zwp_input_method_context_v1 *context;
    struct wl_keyboard *keyboard;

    IBusInputContext *ibuscontext;
    IBusText *preedit_text;
    guint preedit_cursor_pos;
    IBusModifierType modifiers;

    struct xkb_context *xkb_context;

    struct xkb_keymap *keymap;
    struct xkb_state *state;

    xkb_mod_mask_t shift_mask;
    xkb_mod_mask_t lock_mask;
    xkb_mod_mask_t control_mask;
    xkb_mod_mask_t mod1_mask;
    xkb_mod_mask_t mod2_mask;
    xkb_mod_mask_t mod3_mask;
    xkb_mod_mask_t mod4_mask;
    xkb_mod_mask_t mod5_mask;
    xkb_mod_mask_t super_mask;
    xkb_mod_mask_t hyper_mask;
    xkb_mod_mask_t meta_mask;

    uint32_t serial;

    GCancellable *cancellable;
};

struct _IBusWaylandKeyEvent
{
    struct zwp_input_method_context_v1 *context;
    uint32_t serial;
    uint32_t time;
    uint32_t key;
    enum wl_keyboard_key_state state;
};
typedef struct _IBusWaylandKeyEvent IBusWaylandKeyEvent;

struct _IBusWaylandSource
{
    GSource source;
    GPollFD pfd;
    uint32_t mask;
    struct wl_display *display;
};
typedef struct _IBusWaylandSource IBusWaylandSource;

G_DEFINE_TYPE_WITH_PRIVATE (IBusWaylandIM, ibus_wayland_im, IBUS_TYPE_OBJECT)

struct wl_display *_display = NULL;
struct wl_registry *_registry = NULL;
static IBusBus *_bus = NULL;

static gboolean _use_sync_mode = FALSE;

static void         ibus_wayland_im_destroy (IBusObject *object);

static gboolean
_get_boolean_env (const gchar *name,
                  gboolean     defval)
{
    const gchar *value = g_getenv (name);

    if (value == NULL)
      return defval;

    if (g_strcmp0 (value, "") == 0 ||
        g_strcmp0 (value, "0") == 0 ||
        g_strcmp0 (value, "false") == 0 ||
        g_strcmp0 (value, "False") == 0 ||
        g_strcmp0 (value, "FALSE") == 0)
      return FALSE;

    return TRUE;
}

static gboolean
ibus_wayland_source_prepare (GSource *base,
                             gint    *timeout)
{
    IBusWaylandSource *source = (IBusWaylandSource *) base;

    *timeout = -1;

    wl_display_flush (source->display);

    return FALSE;
}

static gboolean
ibus_wayland_source_check (GSource *base)
{
    IBusWaylandSource *source = (IBusWaylandSource *) base;

    if (source->pfd.revents & (G_IO_ERR | G_IO_HUP))
        g_error ("Lost connection to wayland compositor");

    return source->pfd.revents;
}

static gboolean
ibus_wayland_source_dispatch (GSource    *base,
                              GSourceFunc callback,
                              gpointer    data)
{
    IBusWaylandSource *source = (IBusWaylandSource *) base;

    if (source->pfd.revents) {
        wl_display_dispatch (source->display);
        source->pfd.revents = 0;
    }

    return TRUE;
}

static void
ibus_wayland_source_finalize (GSource *source)
{
}

static GSourceFuncs ibus_wayland_source_funcs = {
    ibus_wayland_source_prepare,
    ibus_wayland_source_check,
    ibus_wayland_source_dispatch,
    ibus_wayland_source_finalize
};

GSource *
ibus_wayland_source_new (struct wl_display *display)
{
    GSource *source;
    IBusWaylandSource *wlsource;

    source = g_source_new (&ibus_wayland_source_funcs,
                           sizeof (IBusWaylandSource));
    wlsource = (IBusWaylandSource *) source;

    wlsource->display = display;
    wlsource->pfd.fd = wl_display_get_fd (display);
    wlsource->pfd.events = G_IO_IN | G_IO_ERR | G_IO_HUP;
    g_source_add_poll (source, &wlsource->pfd);

    return source;
}

static void
_context_commit_text_cb (IBusInputContext *context,
                         IBusText         *text,
                         IBusWaylandIM    *wlim)
{
    zwp_input_method_context_v1_commit_string (wlim->priv->context,
                                               wlim->priv->serial,
                                               text->text);
}

static void
_context_forward_key_event_cb (IBusInputContext *context,
                               guint             keyval,
                               guint             keycode,
                               guint             modifiers,
                               IBusWaylandIM    *wlim)
{
    uint32_t state;

    if (modifiers & IBUS_RELEASE_MASK)
        state = WL_KEYBOARD_KEY_STATE_RELEASED;
    else
        state = WL_KEYBOARD_KEY_STATE_PRESSED;

    zwp_input_method_context_v1_keysym (wlim->priv->context,
                                        wlim->priv->serial,
                                        0,
                                        keyval,
                                        state,
                                        modifiers);
}

static void
_context_show_preedit_text_cb (IBusInputContext *context,
                               IBusWaylandIM    *wlim)
{
    IBusWaylandIMPrivate *priv = wlim->priv;
    /* CURSOR is byte offset.  */
    uint32_t cursor =
        g_utf8_offset_to_pointer (priv->preedit_text->text,
                                  priv->preedit_cursor_pos) -
        priv->preedit_text->text;

    zwp_input_method_context_v1_preedit_cursor (priv->context,
                                                cursor);
    zwp_input_method_context_v1_preedit_string (priv->context,
                                                priv->serial,
                                                priv->preedit_text->text,
                                                priv->preedit_text->text);
}

static void
_context_hide_preedit_text_cb (IBusInputContext *context,
                               IBusWaylandIM    *wlim)
{
    zwp_input_method_context_v1_preedit_string (wlim->priv->context,
                                                wlim->priv->serial,
                                                "",
                                                "");
}

static void
_context_update_preedit_text_cb (IBusInputContext *context,
                                 IBusText         *text,
                                 gint              cursor_pos,
                                 gboolean          visible,
                                 IBusWaylandIM    *wlim)
{
    IBusWaylandIMPrivate *priv = wlim->priv;
    if (priv->preedit_text)
        g_object_unref (priv->preedit_text);
    priv->preedit_text = g_object_ref_sink (text);
    priv->preedit_cursor_pos = cursor_pos;

    if (visible)
        _context_show_preedit_text_cb (context, wlim);
    else
        _context_hide_preedit_text_cb (context, wlim);
}

static void
handle_surrounding_text (void                               *data,
                         struct zwp_input_method_context_v1 *context,
                         const char                         *text,
                         uint32_t                            cursor,
                         uint32_t                            anchor)
{
#if ENABLE_SURROUNDING
    IBusWaylandIM *wlim = data;
    IBusWaylandIMPrivate *priv = wlim->priv;

    if (priv->ibuscontext != NULL &&
        ibus_input_context_needs_surrounding_text (priv->ibuscontext)) {
        /* CURSOR_POS and ANCHOR_POS are character offset.  */
        guint cursor_pos = g_utf8_pointer_to_offset (text, text + cursor);
        guint anchor_pos = g_utf8_pointer_to_offset (text, text + anchor);
        IBusText *ibustext = ibus_text_new_from_string (text);

        ibus_input_context_set_surrounding_text (priv->ibuscontext,
                                                 ibustext,
                                                 cursor_pos,
                                                 anchor_pos);
    }
#endif
}

static void
handle_reset (void                           *data,
              struct zwp_input_method_context_v1 *context)
{
}

static void
handle_content_type (void                               *data,
                     struct zwp_input_method_context_v1 *context,
                     uint32_t                            hint,
                     uint32_t                            purpose)
{
}

static void
handle_invoke_action (void                               *data,
                      struct zwp_input_method_context_v1 *context,
                      uint32_t                            button,
                      uint32_t                            index)
{
}

static void
handle_commit_state (void                               *data,
                     struct zwp_input_method_context_v1 *context,
                     uint32_t                            serial)
{
    IBusWaylandIM *wlim = data;

    wlim->priv->serial = serial;
}

static void
handle_preferred_language (void                               *data,
                           struct zwp_input_method_context_v1 *context,
                           const char                         *language)
{
}

static const struct zwp_input_method_context_v1_listener context_listener = {
    handle_surrounding_text,
    handle_reset,
    handle_content_type,
    handle_invoke_action,
    handle_commit_state,
    handle_preferred_language
};

static void
input_method_keyboard_keymap (void               *data,
                              struct wl_keyboard *wl_keyboard,
                              uint32_t            format,
                              int32_t             fd,
                              uint32_t            size)
{
    IBusWaylandIM *wlim = data;
    IBusWaylandIMPrivate *priv = wlim->priv;
    GMappedFile *map;
    GError *error = NULL;

    if (format != WL_KEYBOARD_KEYMAP_FORMAT_XKB_V1) {
        close(fd);
        return;
    }

    map = g_mapped_file_new_from_fd (fd, FALSE, &error);
    if (map == NULL) {
        if (error) {
            g_warning ("Failed to map file fd %s", error->message);
            g_error_free (error);
        }
        close (fd);
        return;
    }

    priv->keymap =
        xkb_map_new_from_string (priv->xkb_context,
                                 g_mapped_file_get_contents (map),
                                 XKB_KEYMAP_FORMAT_TEXT_V1,
                                 0);

    g_mapped_file_unref (map);
    close(fd);

    if (!priv->keymap) {
        return;
    }

    priv->state = xkb_state_new (priv->keymap);
    if (!priv->state) {
        xkb_map_unref (priv->keymap);
        return;
    }

    priv->shift_mask =
        1 << xkb_map_mod_get_index (priv->keymap, "Shift");
    priv->lock_mask =
        1 << xkb_map_mod_get_index (priv->keymap, "Lock");
    priv->control_mask =
        1 << xkb_map_mod_get_index (priv->keymap, "Control");
    priv->mod1_mask =
        1 << xkb_map_mod_get_index (priv->keymap, "Mod1");
    priv->mod2_mask =
        1 << xkb_map_mod_get_index (priv->keymap, "Mod2");
    priv->mod3_mask =
        1 << xkb_map_mod_get_index (priv->keymap, "Mod3");
    priv->mod4_mask =
        1 << xkb_map_mod_get_index (priv->keymap, "Mod4");
    priv->mod5_mask =
        1 << xkb_map_mod_get_index (priv->keymap, "Mod5");
    priv->super_mask =
        1 << xkb_map_mod_get_index (priv->keymap, "Super");
    priv->hyper_mask =
        1 << xkb_map_mod_get_index (priv->keymap, "Hyper");
    priv->meta_mask =
        1 << xkb_map_mod_get_index (priv->keymap, "Meta");
}

static void
_process_key_event_done (GObject      *object,
                         GAsyncResult *res,
                         gpointer      user_data)
{
    IBusInputContext *context = (IBusInputContext *)object;
    IBusWaylandKeyEvent *event = (IBusWaylandKeyEvent *) user_data;

    GError *error = NULL;
    gboolean retval = ibus_input_context_process_key_event_async_finish (
            context,
            res,
            &error);

    if (error != NULL) {
        g_warning ("Process Key Event failed: %s.", error->message);
        g_error_free (error);
    }

    if (!retval) {
        zwp_input_method_context_v1_key (event->context,
                                         event->serial,
                                         event->time,
                                         event->key,
                                         event->state);
    }

    g_free (event);
}

static void
input_method_keyboard_key (void               *data,
                           struct wl_keyboard *wl_keyboard,
                           uint32_t            serial,
                           uint32_t            time,
                           uint32_t            key,
                           uint32_t            state)
{
    IBusWaylandIM *wlim = data;
    IBusWaylandIMPrivate *priv= wlim->priv;
    uint32_t code;
    uint32_t num_syms;
    const xkb_keysym_t *syms;
    xkb_keysym_t sym;
    guint32 modifiers;

    if (!priv->state)
        return;

    if (!priv->ibuscontext) {
        zwp_input_method_context_v1_key (priv->context,
                                         serial,
                                         time,
                                         key,
                                         state);
        return;
    }
        
    code = key + 8;
    num_syms = xkb_key_get_syms (priv->state, code, &syms);

    sym = XKB_KEY_NoSymbol;
    if (num_syms == 1)
        sym = syms[0];

    modifiers = priv->modifiers;
    if (state == WL_KEYBOARD_KEY_STATE_RELEASED)
        modifiers |= IBUS_RELEASE_MASK;

    if (_use_sync_mode) {
        gboolean retval =
            ibus_input_context_process_key_event (priv->ibuscontext,
                                                  sym,
                                                  code,
                                                  modifiers);
        if (!retval) {
            zwp_input_method_context_v1_key (priv->context,
                                             serial,
                                             time,
                                             key,
                                             state);
        }
    } else {
        IBusWaylandKeyEvent *event = g_new (IBusWaylandKeyEvent, 1);
        event->context = priv->context;
        event->serial = serial;
        event->time = time;
        event->key = key;
        event->state = state;
        ibus_input_context_process_key_event_async (priv->ibuscontext,
                                                    sym,
                                                    code,
                                                    modifiers,
                                                    -1,
                                                    NULL,
                                                    _process_key_event_done,
                                                    event);
    }
}

static void
input_method_keyboard_modifiers (void               *data,
                                 struct wl_keyboard *wl_keyboard,
                                 uint32_t            serial,
                                 uint32_t            mods_depressed,
                                 uint32_t            mods_latched,
                                 uint32_t            mods_locked,
                                 uint32_t            group)
{
    IBusWaylandIM *wlim = data;
    IBusWaylandIMPrivate *priv = wlim->priv;
    struct zwp_input_method_context_v1 *context = priv->context;
    xkb_mod_mask_t mask;

    xkb_state_update_mask (priv->state, mods_depressed,
                           mods_latched, mods_locked, 0, 0, group);
    mask = xkb_state_serialize_mods (priv->state,
                                     XKB_STATE_DEPRESSED |
                                     XKB_STATE_LATCHED);

    priv->modifiers = 0;
    if (mask & priv->shift_mask)
        priv->modifiers |= IBUS_SHIFT_MASK;
    if (mask & priv->lock_mask)
        priv->modifiers |= IBUS_LOCK_MASK;
    if (mask & priv->control_mask)
        priv->modifiers |= IBUS_CONTROL_MASK;
    if (mask & priv->mod1_mask)
        priv->modifiers |= IBUS_MOD1_MASK;
    if (mask & priv->mod2_mask)
        priv->modifiers |= IBUS_MOD2_MASK;
    if (mask & priv->mod3_mask)
        priv->modifiers |= IBUS_MOD3_MASK;
    if (mask & priv->mod4_mask)
        priv->modifiers |= IBUS_MOD4_MASK;
    if (mask & priv->mod5_mask)
        priv->modifiers |= IBUS_MOD5_MASK;
    if (mask & priv->super_mask)
        priv->modifiers |= IBUS_SUPER_MASK;
    if (mask & priv->hyper_mask)
        priv->modifiers |= IBUS_HYPER_MASK;
    if (mask & priv->meta_mask)
        priv->modifiers |= IBUS_META_MASK;

    zwp_input_method_context_v1_modifiers (context, serial,
                                           mods_depressed, mods_latched,
                                           mods_locked, group);
}

static const struct wl_keyboard_listener keyboard_listener = {
    input_method_keyboard_keymap,
    NULL, /* enter */
    NULL, /* leave */
    input_method_keyboard_key,
    input_method_keyboard_modifiers
};

static void
_create_input_context_done (GObject      *object,
                            GAsyncResult *res,
                            gpointer      user_data)
{
    IBusWaylandIM *wlim = (IBusWaylandIM *) user_data;
    IBusWaylandIMPrivate *priv = wlim->priv;

    GError *error = NULL;
    IBusInputContext *context = ibus_bus_create_input_context_async_finish (
            _bus, res, &error);

    if (priv->cancellable != NULL) {
        g_object_unref (priv->cancellable);
        priv->cancellable = NULL;
    }

    if (context == NULL) {
        g_warning ("Create input context failed: %s.", error->message);
        g_error_free (error);
    }
    else {
        priv->ibuscontext = context;

        g_signal_connect (priv->ibuscontext, "commit-text",
                          G_CALLBACK (_context_commit_text_cb),
                          wlim);
        g_signal_connect (priv->ibuscontext, "forward-key-event",
                          G_CALLBACK (_context_forward_key_event_cb),
                          wlim);

        g_signal_connect (priv->ibuscontext, "update-preedit-text",
                          G_CALLBACK (_context_update_preedit_text_cb),
                          wlim);
        g_signal_connect (priv->ibuscontext, "show-preedit-text",
                          G_CALLBACK (_context_show_preedit_text_cb),
                          wlim);
        g_signal_connect (priv->ibuscontext, "hide-preedit-text",
                          G_CALLBACK (_context_hide_preedit_text_cb),
                          wlim);
    
#ifdef ENABLE_SURROUNDING
        ibus_input_context_set_capabilities (priv->ibuscontext,
                                             IBUS_CAP_FOCUS |
                                             IBUS_CAP_PREEDIT_TEXT |
                                             IBUS_CAP_SURROUNDING_TEXT);
#else
        ibus_input_context_set_capabilities (priv->ibuscontext,
                                             IBUS_CAP_FOCUS |
                                             IBUS_CAP_PREEDIT_TEXT);
#endif

        ibus_input_context_focus_in (priv->ibuscontext);
    }
}

static void
input_method_activate (void                               *data,
                       struct zwp_input_method_v1         *input_method,
                       struct zwp_input_method_context_v1 *context)
{
    IBusWaylandIM *wlim = data;
    IBusWaylandIMPrivate *priv = wlim->priv;

    if (priv->context) {
        zwp_input_method_context_v1_destroy (priv->context);
        priv->context = NULL;
    }

    priv->serial = 0;
    priv->context = context;

    zwp_input_method_context_v1_add_listener (context, &context_listener, wlim);
    priv->keyboard = zwp_input_method_context_v1_grab_keyboard (context);
    wl_keyboard_add_listener (priv->keyboard,
                              &keyboard_listener,
                              wlim);

    if (priv->ibuscontext) {
        g_object_unref (priv->ibuscontext);
        priv->ibuscontext = NULL;
    }

    priv->cancellable = g_cancellable_new ();
    ibus_bus_create_input_context_async (_bus,
                                         "wayland",
                                         -1,
                                         priv->cancellable,
                                         _create_input_context_done,
                                         wlim);
}

static void
input_method_deactivate (void                               *data,
                         struct zwp_input_method_v1         *input_method,
                         struct zwp_input_method_context_v1 *context)
{
    IBusWaylandIM *wlim = data;
    IBusWaylandIMPrivate *priv = wlim->priv;

    if (priv->cancellable) {
        /* Cancel any ongoing create input context request.  */
        g_cancellable_cancel (priv->cancellable);
        g_object_unref (priv->cancellable);
        priv->cancellable = NULL;
    }

    if (priv->ibuscontext) {
        ibus_input_context_focus_out (priv->ibuscontext);
        g_object_unref (priv->ibuscontext);
        priv->ibuscontext = NULL;
    }

    if (priv->preedit_text) {
        g_object_unref (priv->preedit_text);
        priv->preedit_text = NULL;
    }

    if (priv->context) {
        zwp_input_method_context_v1_destroy (priv->context);
        priv->context = NULL;
    }
}

static const struct zwp_input_method_v1_listener input_method_listener = {
    input_method_activate,
    input_method_deactivate
};

static void
registry_handle_global (void               *data,
                        struct wl_registry *registry,
                        uint32_t            name,
                        const char         *interface,
                        uint32_t            version)
{
    IBusWaylandIM *wlim = data;
    IBusWaylandIMPrivate *priv = wlim->priv;

    if (!g_strcmp0 (interface, "zwp_input_method_v1")) {
        priv->input_method =
            wl_registry_bind (registry, name,
                              &zwp_input_method_v1_interface, 1);
        zwp_input_method_v1_add_listener (priv->input_method,
                                          &input_method_listener, wlim);
    }
}

static void
registry_handle_global_remove (void               *data,
                               struct wl_registry *registry,
                               uint32_t            name)
{
}

static const struct wl_registry_listener registry_listener = {
    registry_handle_global,
    registry_handle_global_remove
};


static void
ibus_wayland_im_class_init (IBusWaylandIMClass *class)
{
    IBusObjectClass *ibus_object_class = IBUS_OBJECT_CLASS (class);
    ibus_object_class->destroy = ibus_wayland_im_destroy;
}

static void
ibus_wayland_im_init (IBusWaylandIM *wlim)
{
    IBusWaylandIMPrivate *priv;
    GSource *source;

    wlim->priv = priv = ibus_wayland_im_get_instance_private (wlim);
    _display = wl_display_connect (NULL);
    if (_display == NULL) {
        g_error ("Failed to connect to Wayland server: %s\n",
                 g_strerror (errno));
    }

    _registry = wl_display_get_registry (_display);
    wl_registry_add_listener (_registry, &registry_listener, wlim);
    wl_display_roundtrip (_display);
    if (priv->input_method == NULL) {
        g_error ("No input_method global\n");
    }

    priv->xkb_context = xkb_context_new (0);
    if (priv->xkb_context == NULL) {
        g_error ("Failed to create XKB context\n");
    }

    _use_sync_mode = _get_boolean_env ("IBUS_ENABLE_SYNC_MODE", FALSE);

    source = ibus_wayland_source_new (_display);
    g_source_set_priority (source, G_PRIORITY_DEFAULT);
    g_source_set_can_recurse (source, TRUE);
    g_source_attach (source, NULL);
}


static void
ibus_wayland_im_destroy (IBusObject *object)
{
    g_debug ("IBusWaylandIM is destroyed.");
    IBUS_OBJECT_CLASS (ibus_wayland_im_parent_class)->destroy (object);
}


IBusWaylandIM *
ibus_wayland_im_new (IBusBus *bus)
{
    GObject *object = g_object_new (IBUS_TYPE_WAYLAND_IM,
                                    NULL);
    _bus = bus;
    return IBUS_WAYLAND_IM (object);
}

