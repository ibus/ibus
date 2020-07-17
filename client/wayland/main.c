/* -*- mode: C; c-basic-offset: 4; indent-tabs-mode: nil; -*- */
/* vim:set et sts=4: */
/* ibus - The Input Bus
 * Copyright (C) 2019 Takao Fujiwara <takao.fujiwara1@gmail.com>
 * Copyright (C) 2013 Intel Corporation
 * Copyright (C) 2013-2019 Red Hat, Inc.
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
#include <ibus.h>
#include <stdlib.h>
#include <string.h>
#include <wayland-client.h>
#include <xkbcommon/xkbcommon.h>

#include "input-method-unstable-v1-client-protocol.h"
#include "input-method-unstable-v2-client-protocol.h"

struct _IBusWaylandIM
{
    struct zwp_input_method_v1 *input_method;
    struct zwp_input_method_context_v1 *context;
    struct wl_keyboard *keyboard;


    struct zwp_input_method_manager_v2 * input_method_manager_v2;
    struct zwp_input_method_v2 *input_method_v2;
    struct zwp_input_method_keyboard_grab_v2 *keyboard_grab;
    char *pending_surrounding_text;
    char *commit_text;
    uint32_t pending_cursor;
    uint32_t pending_anchor;

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
typedef struct _IBusWaylandIM IBusWaylandIM;

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

struct wl_display *_display = NULL;
struct wl_registry *_registry = NULL;
static struct wl_seat *_seat = NULL;
static IBusBus *_bus = NULL;

static gboolean _use_sync_mode = FALSE;

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
    zwp_input_method_context_v1_commit_string (wlim->context,
                                               wlim->serial,
                                               text->text);
}

static void
_input_method_v2_commit_string_cb (IBusInputContext *context,
                         IBusText         *text,
                         IBusWaylandIM    *wlim)
{
    zwp_input_method_v2_commit_string (wlim->input_method_v2,
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

    zwp_input_method_context_v1_keysym (wlim->context,
                                        wlim->serial,
                                        0,
                                        keyval,
                                        state,
                                        modifiers);
}

static void
_context_show_preedit_text_cb (IBusInputContext *context,
                               IBusWaylandIM    *wlim)
{
    /* CURSOR is byte offset.  */
    uint32_t cursor =
        g_utf8_offset_to_pointer (wlim->preedit_text->text,
                                  wlim->preedit_cursor_pos) -
        wlim->preedit_text->text;

    zwp_input_method_context_v1_preedit_cursor (wlim->context,
                                                cursor);
    zwp_input_method_context_v1_preedit_string (wlim->context,
                                                wlim->serial,
                                                wlim->preedit_text->text,
                                                wlim->preedit_text->text);
}

static void
_context_hide_preedit_text_cb (IBusInputContext *context,
                               IBusWaylandIM    *wlim)
{
    zwp_input_method_context_v1_preedit_string (wlim->context,
                                                wlim->serial,
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
    if (wlim->preedit_text)
        g_object_unref (wlim->preedit_text);
    wlim->preedit_text = g_object_ref_sink (text);
    wlim->preedit_cursor_pos = cursor_pos;

    if (visible)
        _context_show_preedit_text_cb (context, wlim);
    else
        _context_hide_preedit_text_cb (context, wlim);
}

// TODO: check the arguments and that we get the right ones here from IBUS...
static void
_input_method_v2_set_preedit_string_cb (IBusInputContext *context,
                                 IBusText         *text,
                                 gint              cursor_pos,
                                 gboolean          visible,
                                 IBusWaylandIM    *wlim)
{
    if (wlim->preedit_text)
        g_object_unref (wlim->preedit_text);
    wlim->preedit_text = g_object_ref_sink (text);
    wlim->preedit_cursor_pos = cursor_pos;

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

    if (wlim->ibuscontext != NULL &&
        ibus_input_context_needs_surrounding_text (wlim->ibuscontext)) {
        /* CURSOR_POS and ANCHOR_POS are character offset.  */
        guint cursor_pos = g_utf8_pointer_to_offset (text, text + cursor);
        guint anchor_pos = g_utf8_pointer_to_offset (text, text + anchor);
        IBusText *ibustext = ibus_text_new_from_string (text);

        ibus_input_context_set_surrounding_text (wlim->ibuscontext,
                                                 ibustext,
                                                 cursor_pos,
                                                 anchor_pos);
    }
#endif
}

static void
input_method_handle_surrounding_text_v2 (void                               *data,
                                         struct zwp_input_method_v2 *input_method,
                                         const char                         *text,
                                         uint32_t                           cursor,
                                         uint32_t                           anchor)
{
#if ENABLE_SURROUNDING
    IBusWaylandIM *wlim = data;

    wlim->pending_surrounding_text = (char*)text;
    wlim->pending_cursor = cursor;
    wlim->pending_anchor = anchor;

    // const char* text = wlim->pending_surrounding_text;
    // uint32_t cursor = wlim->pending_cursor;
    // uint32_t anchor = wlim->pending_anchor;
    // if (wlim->ibuscontext != NULL &&
    //     ibus_input_context_needs_surrounding_text (wlim->ibuscontext)) {
    //     /* CURSOR_POS and ANCHOR_POS are character offsets. */
    //     guint cursor_pos = g_utf8_pointer_to_offset (text, text + cursor);
    //     guint anchor_pos = g_utf8_pointer_to_offset (text, text + anchor);
    //     IBusText *ibustext = ibus_text_new_from_string (text);

    //     ibus_input_context_set_surrounding_text (wlim->ibuscontext,
    //                                              ibustext,
    //                                              cursor_pos,
    //                                              anchor_pos);
    // }

#endif
}

static void
input_method_handle_text_change_cause_v2 (void                                       *data,
                                          struct zwp_input_method_v2  *zwp_input_method_v2,
                                          uint32_t                                   cause)
{
}

static void
input_method_handle_content_type_v2 (void                                       *data,
                                     struct zwp_input_method_v2  *zwp_input_method_v2,
                                     uint32_t                                    hint,
                                     uint32_t                                 purpose)
{
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

    wlim->serial = serial;
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

// TODO: this should apply all pending state
//
// For some reason, as long as the input method is active, the serial
// increases so done must be called at one point in a loop (when the
// example/text-input is running)...
static void
input_method_handle_done_v2 (void                       *data,
                             struct zwp_input_method_v2 *input_method)
{
    IBusWaylandIM *wlim = data;
    uint32_t cursor_pos = wlim->pending_cursor;
    const char* text;

    wlim->serial++;

    if (wlim->preedit_text) {
      text = ibus_text_get_text (wlim->preedit_text);
    } else {
      text = "";
    }

    size_t text_len = strlen (text);
    zwp_input_method_v2_set_preedit_string (wlim->input_method_v2,
        text,
        cursor_pos,
        cursor_pos + text_len);

    zwp_input_method_v2_commit_string (wlim->input_method_v2, "_Commit_");
    // Calling this function seems to result in an endless
    // loop...
    //
    zwp_input_method_v2_commit (input_method, wlim->serial);
}

static void
input_method_handle_unavailable_v2 (void                       *data,
                                    struct zwp_input_method_v2 *input_method)
{
    IBusWaylandIM *wlim = data;
}

static void
input_method_keyboard_keymap (void               *data,
                              struct wl_keyboard *wl_keyboard,
                              uint32_t            format,
                              int32_t             fd,
                              uint32_t            size)
{
    IBusWaylandIM *wlim = data;
    GMappedFile *map;
    GError *error;

    if (format != WL_KEYBOARD_KEYMAP_FORMAT_XKB_V1) {
        close(fd);
        return;
    }

    error = NULL;
    map = g_mapped_file_new_from_fd (fd, FALSE, &error);
    if (map == NULL) {
        close (fd);
        return;
    }

    wlim->keymap =
        xkb_map_new_from_string (wlim->xkb_context,
                                 g_mapped_file_get_contents (map),
                                 XKB_KEYMAP_FORMAT_TEXT_V1,
                                 0);

    g_mapped_file_unref (map);
    close(fd);

    if (!wlim->keymap) {
        return;
    }

    wlim->state = xkb_state_new (wlim->keymap);
    if (!wlim->state) {
        xkb_map_unref (wlim->keymap);
        return;
    }

    wlim->shift_mask =
        1 << xkb_map_mod_get_index (wlim->keymap, "Shift");
    wlim->lock_mask =
        1 << xkb_map_mod_get_index (wlim->keymap, "Lock");
    wlim->control_mask =
        1 << xkb_map_mod_get_index (wlim->keymap, "Control");
    wlim->mod1_mask =
        1 << xkb_map_mod_get_index (wlim->keymap, "Mod1");
    wlim->mod2_mask =
        1 << xkb_map_mod_get_index (wlim->keymap, "Mod2");
    wlim->mod3_mask =
        1 << xkb_map_mod_get_index (wlim->keymap, "Mod3");
    wlim->mod4_mask =
        1 << xkb_map_mod_get_index (wlim->keymap, "Mod4");
    wlim->mod5_mask =
        1 << xkb_map_mod_get_index (wlim->keymap, "Mod5");
    wlim->super_mask =
        1 << xkb_map_mod_get_index (wlim->keymap, "Super");
    wlim->hyper_mask =
        1 << xkb_map_mod_get_index (wlim->keymap, "Hyper");
    wlim->meta_mask =
        1 << xkb_map_mod_get_index (wlim->keymap, "Meta");
}

static void
input_method_keyboard_grab_keymap (void               *data,
                                   struct zwp_input_method_keyboard_grab_v2 *keyboard,
                                   uint32_t            format,
                                   int32_t             fd,
                                   uint32_t            size)
{
    IBusWaylandIM *wlim = data;
    GMappedFile *map;
    GError *error;

    if (format != WL_KEYBOARD_KEYMAP_FORMAT_XKB_V1) {
        close(fd);
        return;
    }

    error = NULL;
    map = g_mapped_file_new_from_fd (fd, FALSE, &error);
    if (map == NULL) {
        close (fd);
        return;
    }

    wlim->keymap =
        xkb_map_new_from_string (wlim->xkb_context,
                                 g_mapped_file_get_contents (map),
                                 XKB_KEYMAP_FORMAT_TEXT_V1,
                                 0);

    g_mapped_file_unref (map);
    close(fd);

    if (!wlim->keymap) {
        return;
    }

    wlim->state = xkb_state_new (wlim->keymap);
    if (!wlim->state) {
        xkb_map_unref (wlim->keymap);
        return;
    }

    wlim->shift_mask =
        1 << xkb_map_mod_get_index (wlim->keymap, "Shift");
    wlim->lock_mask =
        1 << xkb_map_mod_get_index (wlim->keymap, "Lock");
    wlim->control_mask =
        1 << xkb_map_mod_get_index (wlim->keymap, "Control");
    wlim->mod1_mask =
        1 << xkb_map_mod_get_index (wlim->keymap, "Mod1");
    wlim->mod2_mask =
        1 << xkb_map_mod_get_index (wlim->keymap, "Mod2");
    wlim->mod3_mask =
        1 << xkb_map_mod_get_index (wlim->keymap, "Mod3");
    wlim->mod4_mask =
        1 << xkb_map_mod_get_index (wlim->keymap, "Mod4");
    wlim->mod5_mask =
        1 << xkb_map_mod_get_index (wlim->keymap, "Mod5");
    wlim->super_mask =
        1 << xkb_map_mod_get_index (wlim->keymap, "Super");
    wlim->hyper_mask =
        1 << xkb_map_mod_get_index (wlim->keymap, "Hyper");
    wlim->meta_mask =
        1 << xkb_map_mod_get_index (wlim->keymap, "Meta");
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
_process_keyboard_grab_key_event (GObject      *object,
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
    uint32_t code;
    uint32_t num_syms;
    const xkb_keysym_t *syms;
    xkb_keysym_t sym;
    guint32 modifiers;

    if (!wlim->state)
        return;

    if (!wlim->ibuscontext) {
        zwp_input_method_context_v1_key (wlim->context,
                                         serial,
                                         time,
                                         key,
                                         state);
        return;
    }
        
    code = key + 8;
    num_syms = xkb_key_get_syms (wlim->state, code, &syms);

    sym = XKB_KEY_NoSymbol;
    if (num_syms == 1)
        sym = syms[0];

    modifiers = wlim->modifiers;
    if (state == WL_KEYBOARD_KEY_STATE_RELEASED)
        modifiers |= IBUS_RELEASE_MASK;

    if (_use_sync_mode) {
        gboolean retval =
            ibus_input_context_process_key_event (wlim->ibuscontext,
                                                  sym,
                                                  code,
                                                  modifiers);
        if (!retval) {
            zwp_input_method_context_v1_key (wlim->context,
                                             serial,
                                             time,
                                             key,
                                             state);
        }
    } else {
        IBusWaylandKeyEvent *event = g_new (IBusWaylandKeyEvent, 1);
        event->context = wlim->context;
        event->serial = serial;
        event->time = time;
        event->key = key;
        event->state = state;
        ibus_input_context_process_key_event_async (wlim->ibuscontext,
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
input_method_keyboard_grab_key (void               *data,
                                struct zwp_input_method_keyboard_grab_v2 *keyboard,
                                uint32_t            serial,
                                uint32_t            time,
                                uint32_t            key,
                                uint32_t            state)
{
    IBusWaylandIM *wlim = data;
    uint32_t code;
    uint32_t num_syms;
    const xkb_keysym_t *syms;
    xkb_keysym_t sym;
    guint32 modifiers;

    if (!wlim->state)
        return;

    code = key + 8;
    num_syms = xkb_key_get_syms (wlim->state, code, &syms);

    sym = XKB_KEY_NoSymbol;
    if (num_syms == 1)
        sym = syms[0];

    modifiers = wlim->modifiers;
    if (state == WL_KEYBOARD_KEY_STATE_RELEASED)
        modifiers |= IBUS_RELEASE_MASK;

    if (_use_sync_mode) {
        gboolean retval =
            ibus_input_context_process_key_event (wlim->ibuscontext,
                                                  sym,
                                                  code,
                                                  modifiers);
        if (!retval)
           printf("no retval from ibus\n");
    } else {
        IBusWaylandKeyEvent *event = g_new (IBusWaylandKeyEvent, 1);
        event->serial = serial;
        event->time = time;
        event->key = key;
        event->state = state;
        ibus_input_context_process_key_event_async (wlim->ibuscontext,
                                                    sym,
                                                    code,
                                                    modifiers,
                                                    -1,
                                                    NULL,
                                                    // TODO: not sure this will work
                                                    _process_keyboard_grab_key_event,
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
    struct zwp_input_method_context_v1 *context = wlim->context;
    xkb_mod_mask_t mask;

    xkb_state_update_mask (wlim->state, mods_depressed,
                           mods_latched, mods_locked, 0, 0, group);
    mask = xkb_state_serialize_mods (wlim->state,
                                     XKB_STATE_DEPRESSED |
                                     XKB_STATE_LATCHED);

    wlim->modifiers = 0;
    if (mask & wlim->shift_mask)
        wlim->modifiers |= IBUS_SHIFT_MASK;
    if (mask & wlim->lock_mask)
        wlim->modifiers |= IBUS_LOCK_MASK;
    if (mask & wlim->control_mask)
        wlim->modifiers |= IBUS_CONTROL_MASK;
    if (mask & wlim->mod1_mask)
        wlim->modifiers |= IBUS_MOD1_MASK;
    if (mask & wlim->mod2_mask)
        wlim->modifiers |= IBUS_MOD2_MASK;
    if (mask & wlim->mod3_mask)
        wlim->modifiers |= IBUS_MOD3_MASK;
    if (mask & wlim->mod4_mask)
        wlim->modifiers |= IBUS_MOD4_MASK;
    if (mask & wlim->mod5_mask)
        wlim->modifiers |= IBUS_MOD5_MASK;
    if (mask & wlim->super_mask)
        wlim->modifiers |= IBUS_SUPER_MASK;
    if (mask & wlim->hyper_mask)
        wlim->modifiers |= IBUS_HYPER_MASK;
    if (mask & wlim->meta_mask)
        wlim->modifiers |= IBUS_META_MASK;

    zwp_input_method_context_v1_modifiers (context, serial,
                                           mods_depressed, mods_latched,
                                           mods_locked, group);
}

static void
input_method_keyboard_grab_modifiers (void               *data,
                                      struct zwp_input_method_keyboard_grab_v2 *keyboard,
                                      uint32_t            serial,
                                      uint32_t            mods_depressed,
                                      uint32_t            mods_latched,
                                      uint32_t            mods_locked,
                                      uint32_t            group)
{
    IBusWaylandIM *wlim = data;
    xkb_mod_mask_t mask;

    xkb_state_update_mask (wlim->state, mods_depressed,
                           mods_latched, mods_locked, 0, 0, group);
    mask = xkb_state_serialize_mods (wlim->state,
                                     XKB_STATE_DEPRESSED |
                                     XKB_STATE_LATCHED);

    wlim->modifiers = 0;
    if (mask & wlim->shift_mask)
        wlim->modifiers |= IBUS_SHIFT_MASK;
    if (mask & wlim->lock_mask)
        wlim->modifiers |= IBUS_LOCK_MASK;
    if (mask & wlim->control_mask)
        wlim->modifiers |= IBUS_CONTROL_MASK;
    if (mask & wlim->mod1_mask)
        wlim->modifiers |= IBUS_MOD1_MASK;
    if (mask & wlim->mod2_mask)
        wlim->modifiers |= IBUS_MOD2_MASK;
    if (mask & wlim->mod3_mask)
        wlim->modifiers |= IBUS_MOD3_MASK;
    if (mask & wlim->mod4_mask)
        wlim->modifiers |= IBUS_MOD4_MASK;
    if (mask & wlim->mod5_mask)
        wlim->modifiers |= IBUS_MOD5_MASK;
    if (mask & wlim->super_mask)
        wlim->modifiers |= IBUS_SUPER_MASK;
    if (mask & wlim->hyper_mask)
        wlim->modifiers |= IBUS_HYPER_MASK;
    if (mask & wlim->meta_mask)
        wlim->modifiers |= IBUS_META_MASK;
}

static void
input_method_keyboard_grab_repeat_info (void               *data,
                                      struct zwp_input_method_keyboard_grab_v2 *keyboard,
                                      uint32_t            rate,
                                      uint32_t            delay)
{
    IBusWaylandIM *wlim = data;
}


static const struct wl_keyboard_listener keyboard_listener = {
    input_method_keyboard_keymap,
    NULL, /* enter */
    NULL, /* leave */
    input_method_keyboard_key,
    input_method_keyboard_modifiers
};

static const struct zwp_input_method_keyboard_grab_v2_listener keyboard_grab_listener = {
    input_method_keyboard_grab_keymap,
    input_method_keyboard_grab_key,
    input_method_keyboard_grab_modifiers,
    input_method_keyboard_grab_repeat_info
};

static void
_create_input_context_done (GObject      *object,
                            GAsyncResult *res,
                            gpointer      user_data)
{
    IBusWaylandIM *wlim = (IBusWaylandIM *) user_data;

    GError *error = NULL;
    IBusInputContext *context = ibus_bus_create_input_context_async_finish (
            _bus, res, &error);

    if (wlim->cancellable != NULL) {
        g_object_unref (wlim->cancellable);
        wlim->cancellable = NULL;
    }

    if (context == NULL) {
        g_warning ("Create input context failed: %s.", error->message);
        g_error_free (error);
    }
    else {
        wlim->ibuscontext = context;

        g_signal_connect (wlim->ibuscontext, "commit-text",
                          G_CALLBACK (_context_commit_text_cb),
                          wlim);
        g_signal_connect (wlim->ibuscontext, "forward-key-event",
                          G_CALLBACK (_context_forward_key_event_cb),
                          wlim);

        g_signal_connect (wlim->ibuscontext, "update-preedit-text",
                          G_CALLBACK (_context_update_preedit_text_cb),
                          wlim);
        g_signal_connect (wlim->ibuscontext, "show-preedit-text",
                          G_CALLBACK (_context_show_preedit_text_cb),
                          wlim);
        g_signal_connect (wlim->ibuscontext, "hide-preedit-text",
                          G_CALLBACK (_context_hide_preedit_text_cb),
                          wlim);

#ifdef ENABLE_SURROUNDING
        ibus_input_context_set_capabilities (wlim->ibuscontext,
                                             IBUS_CAP_FOCUS |
                                             IBUS_CAP_PREEDIT_TEXT |
                                             IBUS_CAP_SURROUNDING_TEXT);
#else
        ibus_input_context_set_capabilities (wlim->ibuscontext,
                                             IBUS_CAP_FOCUS |
                                             IBUS_CAP_PREEDIT_TEXT);
#endif

        ibus_input_context_focus_in (wlim->ibuscontext);
    }
}

static void
_create_input_method_v2_done (GObject      *object,
                            GAsyncResult *res,
                            gpointer      user_data)
{
    IBusWaylandIM *wlim = (IBusWaylandIM *) user_data;

    GError *error = NULL;
    IBusInputContext *context = ibus_bus_create_input_context_async_finish (
            _bus, res, &error);

    if (wlim->cancellable != NULL) {
        g_object_unref (wlim->cancellable);
        wlim->cancellable = NULL;
    }

    if (context == NULL) {
        g_warning ("Create input context failed: %s.", error->message);
        g_error_free (error);
    }
    else {
        wlim->ibuscontext = context;

        // TODO: all of those should use the v2 interface. It almost
        // looks like they should not be used at all though since
        // everything should be committed on the done event. Then, where
        // is the data from ibus coming from though...
        //
        // Most likely these handle the ibus side and on done we send
        // the prepared strings to the text-input client?
        g_signal_connect (wlim->ibuscontext, "update-preedit-text",
                          G_CALLBACK (_input_method_v2_set_preedit_string_cb),
                          wlim);
        g_signal_connect (wlim->ibuscontext, "commit-text",
                          G_CALLBACK (_input_method_v2_commit_string_cb),
                          wlim);

#ifdef ENABLE_SURROUNDING
        ibus_input_context_set_capabilities (wlim->ibuscontext,
                                             IBUS_CAP_FOCUS |
                                             IBUS_CAP_PREEDIT_TEXT |
                                             IBUS_CAP_SURROUNDING_TEXT);
#else
        ibus_input_context_set_capabilities (wlim->ibuscontext,
                                             IBUS_CAP_FOCUS |
                                             IBUS_CAP_PREEDIT_TEXT);
#endif

        ibus_input_context_focus_in (wlim->ibuscontext);
    }
}


static void
input_method_activate (void                               *data,
                       struct zwp_input_method_v1         *input_method,
                       struct zwp_input_method_context_v1 *context)
{
    IBusWaylandIM *wlim = data;

    if (wlim->context) {
        zwp_input_method_context_v1_destroy (wlim->context);
        wlim->context = NULL;
    }

    wlim->serial = 0;
    wlim->context = context;

    zwp_input_method_context_v1_add_listener (context, &context_listener, wlim);
    wlim->keyboard = zwp_input_method_context_v1_grab_keyboard (context);
    wl_keyboard_add_listener (wlim->keyboard,
                              &keyboard_listener,
                              wlim);

    if (wlim->ibuscontext) {
        g_object_unref (wlim->ibuscontext);
        wlim->ibuscontext = NULL;
    }

    wlim->cancellable = g_cancellable_new ();
    ibus_bus_create_input_context_async (_bus,
                                         "wayland",
                                         -1,
                                         wlim->cancellable,
                                         _create_input_context_done,
                                         wlim);
}

static void
input_method_deactivate (void                               *data,
                         struct zwp_input_method_v1         *input_method,
                         struct zwp_input_method_context_v1 *context)
{
    IBusWaylandIM *wlim = data;

    if (wlim->cancellable) {
        /* Cancel any ongoing create input context request.  */
        g_cancellable_cancel (wlim->cancellable);
        g_object_unref (wlim->cancellable);
        wlim->cancellable = NULL;
    }

    if (wlim->ibuscontext) {
        ibus_input_context_focus_out (wlim->ibuscontext);
        g_object_unref (wlim->ibuscontext);
        wlim->ibuscontext = NULL;
    }

    if (wlim->preedit_text) {
        g_object_unref (wlim->preedit_text);
        wlim->preedit_text = NULL;
    }

    if (wlim->context) {
        zwp_input_method_context_v1_destroy (wlim->context);
        wlim->context = NULL;
    }
}

static void
input_method_activate_v2 (void                               *data,
                          struct zwp_input_method_v2         *input_method)
{
    IBusWaylandIM *wlim = data;
    wlim->serial = 0;

    wlim->keyboard_grab = zwp_input_method_v2_grab_keyboard (input_method);
    zwp_input_method_keyboard_grab_v2_add_listener (wlim->keyboard_grab,
                              &keyboard_grab_listener,
                              wlim);

    if (wlim->ibuscontext) {
        g_object_unref (wlim->ibuscontext);
        wlim->ibuscontext = NULL;
    }

    wlim->cancellable = g_cancellable_new ();

    ibus_bus_create_input_context_async (_bus,
                                         "wayland",
                                         -1,
                                         wlim->cancellable,
                                         _create_input_method_v2_done,
                                         wlim);
}

// TODO
static void
input_method_deactivate_v2 (void                               *data,
                            struct zwp_input_method_v2         *input_method)
{
    IBusWaylandIM *wlim = data;

    if (wlim->cancellable) {
        /* Cancel any ongoing create input context request.  */
        g_cancellable_cancel (wlim->cancellable);
        g_object_unref (wlim->cancellable);
        wlim->cancellable = NULL;
    }

    if (wlim->ibuscontext) {
        ibus_input_context_focus_out (wlim->ibuscontext);
        g_object_unref (wlim->ibuscontext);
        wlim->ibuscontext = NULL;
    }

    if (wlim->preedit_text) {
        g_object_unref (wlim->preedit_text);
        wlim->preedit_text = NULL;
    }
}



static const struct zwp_input_method_v1_listener input_method_listener = {
    input_method_activate,
    input_method_deactivate
};


// TODO: check that we are handling the state similarly to the wlroots
// example
static const struct zwp_input_method_v2_listener input_method_listener_v2 = {
    input_method_activate_v2,
    input_method_deactivate_v2,
    input_method_handle_surrounding_text_v2,
    input_method_handle_text_change_cause_v2,
    input_method_handle_content_type_v2,
    // if this one is not called, no endless loop happens (not that
    // surprising)
    input_method_handle_done_v2,
    input_method_handle_unavailable_v2
};

static void
registry_handle_global (void               *data,
                        struct wl_registry *registry,
                        uint32_t            name,
                        const char         *interface,
                        uint32_t            version)
{
    IBusWaylandIM *wlim = data;
    if (!g_strcmp0 (interface, "zwp_input_method_manager_v2")) {
        wlim->input_method_manager_v2 = wl_registry_bind (registry, name,
              &zwp_input_method_manager_v2_interface, 1);
    } else if (!g_strcmp0 (interface, "zwp_input_method_v1")) {
        wlim->input_method =
            wl_registry_bind (registry, name,
                              &zwp_input_method_v1_interface, 1);
        zwp_input_method_v1_add_listener (wlim->input_method,
                                          &input_method_listener, wlim);
    } else if (!g_strcmp0 (interface, wl_seat_interface.name)) {
      _seat = wl_registry_bind (registry, name, &wl_seat_interface, version);
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
_bus_disconnected_cb (IBusBus *bus,
                      gpointer user_data)
{
    g_debug ("Connection closed by ibus-daemon\n");
    g_clear_object (&_bus);
    ibus_quit ();
}

gint
main (gint    argc,
      gchar **argv)
{
    IBusWaylandIM wlim;
    GSource *source;

    ibus_init ();

    _bus = ibus_bus_new ();
    if (!ibus_bus_is_connected (_bus)) {
        g_printerr ("Cannot connect to ibus-daemon\n");
        return EXIT_FAILURE;
    }

    _display = wl_display_connect (NULL);
    if (_display == NULL) {
        g_printerr ("Failed to connect to Wayland server: %s\n",
                    g_strerror (errno));
        return EXIT_FAILURE;
    }

    _registry = wl_display_get_registry (_display);
    memset (&wlim, 0, sizeof (wlim));
    wl_registry_add_listener (_registry, &registry_listener, &wlim);
    wl_display_roundtrip (_display);

    if (wlim.input_method_manager_v2) {
        wlim.input_method_v2 = zwp_input_method_manager_v2_get_input_method (
          wlim.input_method_manager_v2, _seat);
        zwp_input_method_v2_add_listener (wlim.input_method_v2,
                                          &input_method_listener_v2, &wlim);
    }

    if (wlim.input_method == NULL && wlim.input_method_v2 == NULL) {
        g_printerr ("No input_method global\n");
        return EXIT_FAILURE;
    }

    wlim.xkb_context = xkb_context_new (0);
    if (wlim.xkb_context == NULL) {
        g_printerr ("Failed to create XKB context\n");
        return EXIT_FAILURE;
    }

    _use_sync_mode = _get_boolean_env ("IBUS_ENABLE_SYNC_MODE", FALSE);

    source = ibus_wayland_source_new (_display);
    g_source_set_priority (source, G_PRIORITY_DEFAULT);
    g_source_set_can_recurse (source, TRUE);
    g_source_attach (source, NULL);

    g_signal_connect (_bus, "disconnected",
                      G_CALLBACK (_bus_disconnected_cb), NULL);
    ibus_main ();

    return EXIT_SUCCESS;
}
