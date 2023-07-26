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
#include <sys/time.h>
#include <wayland-client.h>
#include <xkbcommon/xkbcommon.h>

#include "input-method-unstable-v1-client-protocol.h"
#include "ibuswaylandim.h"

enum {
    PROP_0 = 0,
    PROP_BUS,
    PROP_DISPLAY,
    PROP_LOG,
    PROP_VERBOSE
};

typedef struct _IBusWaylandIMPrivate IBusWaylandIMPrivate;
struct _IBusWaylandIMPrivate
{
    FILE *log;
    gboolean verbose;
    struct wl_display *display;
    struct zwp_input_method_v1 *input_method;
    struct zwp_input_method_context_v1 *context;
    struct wl_keyboard *keyboard;
    struct zwp_input_panel_v1 *panel;
    struct zwp_input_panel_surface_v1 *panel_surface;

    IBusBus *ibusbus;
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
    uint32_t modifiers;
    IBusWaylandIM *wlim;
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

struct wl_registry *_registry = NULL;

static gboolean _use_sync_mode = FALSE;

static GObject     *ibus_wayland_im_constructor        (GType          type,
                                                        guint          n_params,
                                                        GObjectConstructParam
                                                                      *params);
static void         ibus_wayland_im_set_property       (IBusWaylandIM *wlim,
                                                        guint          prop_id,
                                                        const GValue  *value,
                                                        GParamSpec    *pspec);
static void         ibus_wayland_im_get_property       (IBusWaylandIM *wlim,
                                                        guint          prop_id,
                                                        GValue        *value,
                                                        GParamSpec    *pspec);
static void         ibus_wayland_im_destroy            (IBusObject    *object);


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
    IBusWaylandIMPrivate *priv;
    g_return_if_fail (IBUS_IS_WAYLAND_IM (wlim));
    priv = ibus_wayland_im_get_instance_private (wlim);
    zwp_input_method_context_v1_commit_string (priv->context,
                                               priv->serial,
                                               text->text);
}


static void
_context_forward_key_event_cb (IBusInputContext *context,
                               guint             keyval,
                               guint             keycode,
                               guint             modifiers,
                               IBusWaylandIM    *wlim)
{
    IBusWaylandIMPrivate *priv;
    uint32_t state;

    g_return_if_fail (IBUS_IS_WAYLAND_IM (wlim));
    priv = ibus_wayland_im_get_instance_private (wlim);
    if (modifiers & IBUS_RELEASE_MASK)
        state = WL_KEYBOARD_KEY_STATE_RELEASED;
    else
        state = WL_KEYBOARD_KEY_STATE_PRESSED;

    zwp_input_method_context_v1_keysym (priv->context,
                                        priv->serial,
                                        0,
                                        keyval,
                                        state,
                                        modifiers);
}


static void
_context_show_preedit_text_cb (IBusInputContext *context,
                               IBusWaylandIM    *wlim)
{
    IBusWaylandIMPrivate *priv;
    uint32_t cursor;
    g_return_if_fail (IBUS_IS_WAYLAND_IM (wlim));
    priv = ibus_wayland_im_get_instance_private (wlim);
    /* CURSOR is byte offset.  */
    cursor =
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
    IBusWaylandIMPrivate *priv;
    g_return_if_fail (IBUS_IS_WAYLAND_IM (wlim));
    priv = ibus_wayland_im_get_instance_private (wlim);
    zwp_input_method_context_v1_preedit_string (priv->context,
                                                priv->serial,
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
    IBusWaylandIMPrivate *priv;
    g_return_if_fail (IBUS_IS_WAYLAND_IM (wlim));
    priv = ibus_wayland_im_get_instance_private (wlim);
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
    IBusWaylandIMPrivate *priv;

    g_return_if_fail (IBUS_IS_WAYLAND_IM (wlim));
    priv = ibus_wayland_im_get_instance_private (wlim);
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
    IBusWaylandIMPrivate *priv;
    g_return_if_fail (IBUS_IS_WAYLAND_IM (wlim));
    priv = ibus_wayland_im_get_instance_private (wlim);
    priv->serial = serial;
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


static struct xkb_keymap *
create_user_xkb_keymap (struct xkb_context *xkb_context,
                        IBusEngineDesc     *desc)
{
    struct xkb_rule_names names;
    const gchar *layout;

    g_assert (xkb_context);
    g_assert (desc);
    names.rules = "evdev";
    names.model = "pc105";
    layout = ibus_engine_desc_get_layout (desc);
    if (!layout || *layout == '\0' || !g_strcmp0 (layout, "default"))
        return NULL;
    names.layout = layout;
    names.variant = ibus_engine_desc_get_layout_variant (desc);
    return xkb_keymap_new_from_names (xkb_context, &names, 0);
}


static struct xkb_keymap *
create_system_xkb_keymap (struct xkb_context *xkb_context,
                          uint32_t            format,
                          int32_t             fd,
                          uint32_t            size)
{
    GMappedFile *map;
    GError *error = NULL;
    struct xkb_keymap *xkb_keymap;

    g_assert (xkb_context);
    if (format != WL_KEYBOARD_KEYMAP_FORMAT_XKB_V1) {
        close(fd);
        return NULL;
    }

    map = g_mapped_file_new_from_fd (fd, FALSE, &error);
    if (map == NULL) {
        if (error) {
            g_warning ("Failed to map file fd %s", error->message);
            g_error_free (error);
        }
        close (fd);
        return NULL;
    }

    xkb_keymap = xkb_map_new_from_string (xkb_context,
                                          g_mapped_file_get_contents (map),
                                          XKB_KEYMAP_FORMAT_TEXT_V1,
                                          0);
    g_mapped_file_unref (map);
    close(fd);
    return xkb_keymap;
}


static gboolean
ibus_wayland_im_update_xkb_state (IBusWaylandIM     *wlim,
                                  struct xkb_keymap *keymap)
{
    IBusWaylandIMPrivate *priv;
    struct xkb_state *state;

    g_return_val_if_fail (IBUS_IS_WAYLAND_IM (wlim), FALSE);
    priv = ibus_wayland_im_get_instance_private (wlim);
    g_return_val_if_fail (keymap, FALSE);
    g_return_val_if_fail ((state = xkb_state_new (keymap)), FALSE);

    priv->shift_mask =
        1 << xkb_map_mod_get_index (keymap, "Shift");
    priv->lock_mask =
        1 << xkb_map_mod_get_index (keymap, "Lock");
    priv->control_mask =
        1 << xkb_map_mod_get_index (keymap, "Control");
    priv->mod1_mask =
        1 << xkb_map_mod_get_index (keymap, "Mod1");
    priv->mod2_mask =
        1 << xkb_map_mod_get_index (keymap, "Mod2");
    priv->mod3_mask =
        1 << xkb_map_mod_get_index (keymap, "Mod3");
    priv->mod4_mask =
        1 << xkb_map_mod_get_index (keymap, "Mod4");
    priv->mod5_mask =
        1 << xkb_map_mod_get_index (keymap, "Mod5");
    priv->super_mask =
        1 << xkb_map_mod_get_index (keymap, "Super");
    priv->hyper_mask =
        1 << xkb_map_mod_get_index (keymap, "Hyper");
    priv->meta_mask =
        1 << xkb_map_mod_get_index (keymap, "Meta");

    if (priv->state)
        xkb_state_unref (priv->state);
    if (priv->keymap)
        xkb_keymap_unref (priv->keymap);
    priv->keymap = keymap;
    priv->state = state;
    return TRUE;
}


static void
_bus_global_engine_changed_cb (IBusBus       *bus,
                               gchar         *engine_name,
                               IBusWaylandIM *wlim)
{
    IBusWaylandIMPrivate *priv;
    IBusEngineDesc *desc;
    struct xkb_keymap *keymap;

    g_return_if_fail (IBUS_IS_BUS (bus));
    g_return_if_fail (IBUS_IS_WAYLAND_IM (wlim));
    priv = ibus_wayland_im_get_instance_private (wlim);
    desc = ibus_bus_get_global_engine (bus);
    g_assert (desc);
    g_assert (!g_strcmp0 (ibus_engine_desc_get_name (desc), engine_name));
    keymap = create_user_xkb_keymap (priv->xkb_context, desc);
    if (keymap && !ibus_wayland_im_update_xkb_state (wlim, keymap))
        xkb_keymap_unref (keymap);
}


static void
input_method_keyboard_keymap (void               *data,
                              struct wl_keyboard *wl_keyboard,
                              uint32_t            format,
                              int32_t             fd,
                              uint32_t            size)
{
    IBusWaylandIM *wlim = data;
    IBusWaylandIMPrivate *priv;
    struct xkb_keymap *keymap;

    g_return_if_fail (IBUS_IS_WAYLAND_IM (wlim));
    priv = ibus_wayland_im_get_instance_private (wlim);

    if (priv->keymap && priv->state)
        return;
    keymap = create_system_xkb_keymap (priv->xkb_context, format, fd, size);
    if (keymap && !ibus_wayland_im_update_xkb_state (wlim, keymap))
        xkb_keymap_unref (keymap);
}


static gboolean
ibus_wayland_im_post_key (IBusWaylandIM *wlim,
                          uint32_t       key,
                          uint32_t       modifiers,
                          uint32_t       state,
                          gboolean       filtered)
{
    IBusWaylandIMPrivate *priv;
    uint32_t code = key + 8;
    uint32_t ch;

    g_return_val_if_fail (IBUS_IS_WAYLAND_IM (wlim), FALSE);
    priv = ibus_wayland_im_get_instance_private (wlim);
    if (!priv->state)
        return FALSE;
    if (!filtered && (state != WL_KEYBOARD_KEY_STATE_RELEASED)) {
        ch = xkb_state_key_get_utf32 (priv->state, code);
        if (!(modifiers & IBUS_MODIFIER_MASK & ~IBUS_SHIFT_MASK) &&
            ch && ch != '\n' && ch != '\b' && ch != '\r' && ch != '\033' &&
            ch != '\x7f') {
            gchar buff[8] = { 0, };
            buff[g_unichar_to_utf8 (ch, buff)] = '\0';
            zwp_input_method_context_v1_commit_string (priv->context,
                                                       priv->serial,
                                                       buff);
            filtered = TRUE;
        }
    }
    xkb_state_update_key (priv->state, code,
                          (state == WL_KEYBOARD_KEY_STATE_RELEASED)
                          ? XKB_KEY_UP : XKB_KEY_DOWN);
    return filtered;
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
    IBusWaylandIMPrivate *priv;

    if (error != NULL) {
        g_warning ("Process Key Event failed: %s.", error->message);
        g_error_free (error);
    }
    g_return_if_fail (event);
    g_return_if_fail (IBUS_IS_WAYLAND_IM (event->wlim));

    priv = ibus_wayland_im_get_instance_private (event->wlim);
    /* Should ignore key events after input_method_deactivate() is called
     * even if context->ref_count is not 0 yet but priv->ibuscontext is null
     * because of the async time lag.
     */
    if (priv->ibuscontext) {
        retval = ibus_wayland_im_post_key (event->wlim,
                                           event->key,
                                           event->modifiers,
                                           event->state,
                                           retval);
    }
    /* Check retral from ibus_wayland_im_post_key() */
    if (priv->ibuscontext && !retval) {
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
    IBusWaylandIMPrivate *priv;
    uint32_t code;
    uint32_t num_syms;
    const xkb_keysym_t *syms;
    xkb_keysym_t sym;
    uint32_t modifiers;

    g_return_if_fail (IBUS_IS_WAYLAND_IM (wlim));
    priv = ibus_wayland_im_get_instance_private (wlim);
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
        retval = ibus_wayland_im_post_key (wlim, key, modifiers, state, retval);
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
        event->modifiers = modifiers & ~IBUS_RELEASE_MASK;
        event->state = state;
        event->wlim = wlim;
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
    IBusWaylandIMPrivate *priv;
    xkb_mod_mask_t mask;

    g_return_if_fail (IBUS_IS_WAYLAND_IM (wlim));
    priv = ibus_wayland_im_get_instance_private (wlim);
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

    zwp_input_method_context_v1_modifiers (priv->context, serial,
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
    IBusWaylandIMPrivate *priv;
    GError *error = NULL;
    IBusInputContext *context;

    g_return_if_fail (IBUS_IS_WAYLAND_IM (wlim));
    priv = ibus_wayland_im_get_instance_private (wlim);
    context = ibus_bus_create_input_context_async_finish (
            priv->ibusbus, res, &error);
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
    IBusWaylandIMPrivate *priv;

    g_return_if_fail (IBUS_IS_WAYLAND_IM (wlim));
    priv = ibus_wayland_im_get_instance_private (wlim);
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
    ibus_bus_create_input_context_async (priv->ibusbus,
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
    IBusWaylandIMPrivate *priv;

    g_return_if_fail (IBUS_IS_WAYLAND_IM (wlim));
    priv = ibus_wayland_im_get_instance_private (wlim);
    if (priv->cancellable) {
        /* Cancel any ongoing create input context request.  */
        g_cancellable_cancel (priv->cancellable);
        g_object_unref (priv->cancellable);
        priv->cancellable = NULL;
    }


    if (priv->ibuscontext) {
        ibus_input_context_focus_out (priv->ibuscontext);
        g_signal_handlers_disconnect_by_func (
                priv->ibuscontext,
                G_CALLBACK (_context_commit_text_cb),
                wlim);
        g_signal_handlers_disconnect_by_func (
                priv->ibuscontext,
                G_CALLBACK (_context_forward_key_event_cb),
                wlim);
        g_signal_handlers_disconnect_by_func (
                priv->ibuscontext,
                G_CALLBACK (_context_update_preedit_text_cb),
                wlim);
        g_signal_handlers_disconnect_by_func (
                priv->ibuscontext,
                G_CALLBACK (_context_show_preedit_text_cb),
                wlim);
        g_signal_handlers_disconnect_by_func (
                priv->ibuscontext,
                G_CALLBACK (_context_hide_preedit_text_cb),
                wlim);
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
    IBusWaylandIMPrivate *priv;

    g_return_if_fail (IBUS_IS_WAYLAND_IM (wlim));
    priv = ibus_wayland_im_get_instance_private (wlim);
    if (priv->verbose) {
        fprintf (priv->log, "wl_reistry gets interface: %s\n", interface);
        fflush (priv->log);
    }
    if (!g_strcmp0 (interface, "zwp_input_method_v1")) {
        priv->input_method =
            wl_registry_bind (registry, name,
                              &zwp_input_method_v1_interface, 1);
        zwp_input_method_v1_add_listener (priv->input_method,
                                          &input_method_listener, wlim);
    }
    if (!g_strcmp0 (interface, "zwp_input_panel_v1")) {
        priv->panel =
            wl_registry_bind (registry, name,
                              &zwp_input_panel_v1_interface, 1);
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
    GObjectClass *gobject_class = G_OBJECT_CLASS (class);
    IBusObjectClass *ibus_object_class = IBUS_OBJECT_CLASS (class);

    gobject_class->constructor = ibus_wayland_im_constructor;
    gobject_class->set_property =
            (GObjectSetPropertyFunc)ibus_wayland_im_set_property;
    gobject_class->get_property =
            (GObjectGetPropertyFunc)ibus_wayland_im_get_property;
    ibus_object_class->destroy = ibus_wayland_im_destroy;

    /* install properties */
    /**
     * IBusWaylandIM:bus:
     *
     * The #IBusBus
     */
    g_object_class_install_property (gobject_class,
                    PROP_BUS,
                    g_param_spec_object ("bus",
                        "IBusBus",
                        "The #IBusBus",
                        IBUS_TYPE_BUS,
                        G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));

    /**
     * IBusWaylandIM:wl_display:
     *
     * The struct wl_display
     */
    g_object_class_install_property (gobject_class,
                    PROP_DISPLAY,
                    g_param_spec_pointer ("wl_display",
                        "wl_display",
                        "The struct wl_display",
                        G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));

    /**
     * IBusWaylandIM:log:
     *
     * The FILE of the logging file
     * The default is $XDG_CACHE_HOME/wayland.log
     */
    g_object_class_install_property (gobject_class,
                    PROP_LOG,
                    g_param_spec_pointer ("log",
                        "loggin file",
                        "The FILE of the logging file",
                        G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));

    /**
     * IBusWaylandIM:verbose:
     *
     * The verbose logging mode
     * %TRUE if output the logging file with verbose, otherwise %FALSE.
     */
    g_object_class_install_property (gobject_class,
                    PROP_VERBOSE,
                    g_param_spec_boolean ("verbose",
                        "verbose mode",
                        "The verbose logging mode",
                        FALSE,
                        G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));

}


static gboolean
ibus_wayland_im_open_log (IBusWaylandIM *wlim)
{
    IBusWaylandIMPrivate *priv;
    char *directory;
    char *path;
    struct timeval time_val;
    struct tm local_time;

    priv = ibus_wayland_im_get_instance_private (wlim);
    directory = g_build_filename (g_get_user_cache_dir (), "ibus", NULL);
    g_return_val_if_fail (directory, FALSE);
    errno = 0;
    if (g_mkdir_with_parents (directory, 0700) != 0) {
        g_error ("mkdir is failed in: %s: %s",
                 directory, g_strerror (errno));
        return FALSE;
    }
    path = g_build_filename (directory, "wayland.log", NULL);
    g_free (directory);
    if (!(priv->log = fopen (path, "w"))) {
        g_error ("Cannot open log file: %s: %s",
                 path, g_strerror (errno));
        return FALSE;
    }
    g_free (path);
    gettimeofday (&time_val, NULL);
    localtime_r (&time_val.tv_sec, &local_time);
    fprintf (priv->log, "Start %02d:%02d:%02d.%6d\n",
             local_time.tm_hour,
             local_time.tm_min,
             local_time.tm_sec,
             (int)time_val.tv_usec);
    fflush (priv->log);
    return TRUE;
}


static GObject*
ibus_wayland_im_constructor (GType                  type,
                             guint                  n_params,
                             GObjectConstructParam *params)
{
    GObject *object;
    IBusWaylandIM *wlim;
    IBusWaylandIMPrivate *priv;
    GSource *source;
    IBusEngineDesc *desc;
    struct xkb_keymap *keymap = NULL;

    object = G_OBJECT_CLASS (ibus_wayland_im_parent_class)->constructor (
            type, n_params, params);
    /* make bus object sink */
    g_object_ref_sink (object);
    wlim = IBUS_WAYLAND_IM (object);
    priv = ibus_wayland_im_get_instance_private (wlim);
    if (!priv->log)
        g_assert (ibus_wayland_im_open_log (wlim));
    if (!priv->display)
        priv->display = wl_display_connect (NULL);
    if (!priv->display) {
        g_error ("Failed to connect to Wayland server: %s\n",
                 g_strerror (errno));
    }

    _registry = wl_display_get_registry (priv->display);
    wl_registry_add_listener (_registry, &registry_listener, wlim);
    wl_display_roundtrip (priv->display);
    if (priv->input_method == NULL) {
        g_error ("No input_method global\n");
    }

    priv->xkb_context = xkb_context_new (0);
    if (priv->xkb_context == NULL) {
        g_error ("Failed to create XKB context\n");
    }

    if (!priv->ibusbus || !ibus_bus_is_connected (priv->ibusbus)) {
        g_warning ("Cannot connect to ibus-daemon");
        g_object_unref (object);
        return NULL;
    }
    desc = ibus_bus_get_global_engine (priv->ibusbus);
    if (desc)
        keymap = create_user_xkb_keymap (priv->xkb_context, desc);
    if (keymap && !ibus_wayland_im_update_xkb_state (wlim, keymap))
        xkb_keymap_unref (keymap);
    ibus_bus_set_watch_ibus_signal (priv->ibusbus, TRUE);
    g_signal_connect (priv->ibusbus, "global-engine-changed",
                      G_CALLBACK (_bus_global_engine_changed_cb),
                      object);

    _use_sync_mode = _get_boolean_env ("IBUS_ENABLE_SYNC_MODE", FALSE);

    source = ibus_wayland_source_new (priv->display);
    g_source_set_priority (source, G_PRIORITY_DEFAULT);
    g_source_set_can_recurse (source, TRUE);
    g_source_attach (source, NULL);

    return object;
}


static void
ibus_wayland_im_init (IBusWaylandIM *wlim)
{
}


static void
ibus_wayland_im_destroy (IBusObject *object)
{
    IBusWaylandIM *wlim = (IBusWaylandIM *)object;
    IBusWaylandIMPrivate *priv;

    g_debug ("IBusWaylandIM is destroyed.");
    g_return_if_fail (IBUS_IS_WAYLAND_IM (object));
    priv = ibus_wayland_im_get_instance_private (wlim);
    if (priv->input_method)
        g_clear_pointer (&priv->input_method, zwp_input_method_v1_destroy);
    if (priv->panel)
        g_clear_pointer (&priv->panel, zwp_input_panel_v1_destroy);
    IBUS_OBJECT_CLASS (ibus_wayland_im_parent_class)->destroy (object);
}


static void
ibus_wayland_im_set_property (IBusWaylandIM *wlim,
                              guint          prop_id,
                              const GValue  *value,
                              GParamSpec    *pspec)
{
    IBusWaylandIMPrivate *priv;
    g_return_if_fail (IBUS_IS_WAYLAND_IM (wlim));

    priv = ibus_wayland_im_get_instance_private (wlim);
    switch (prop_id) {
    case PROP_BUS:
        g_assert (priv->ibusbus == NULL);
        priv->ibusbus = g_value_get_object (value);
        if (!IBUS_IS_BUS (priv->ibusbus)) {
            g_warning ("bus is not IBusBus.");
            priv->ibusbus = NULL;
        }
        break;
    case PROP_DISPLAY:
        g_assert (priv->display == NULL);
        priv->display = g_value_get_pointer (value);
        break;
    case PROP_LOG:
        g_assert (priv->log == NULL);
        priv->log = g_value_get_pointer (value);
        break;
    case PROP_VERBOSE:
        g_assert (!priv->verbose);
        priv->verbose = g_value_get_boolean (value);
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (wlim, prop_id, pspec);
    }
}


static void
ibus_wayland_im_get_property (IBusWaylandIM *wlim,
                              guint          prop_id,
                              GValue        *value,
                              GParamSpec    *pspec)
{
    IBusWaylandIMPrivate *priv;
    g_return_if_fail (IBUS_IS_WAYLAND_IM (wlim));

    priv = ibus_wayland_im_get_instance_private (wlim);
    switch (prop_id) {
    case PROP_BUS:
        g_value_set_object (value, priv->ibusbus);
        break;
    case PROP_DISPLAY:
        g_value_set_pointer (value, priv->display);
        break;
    case PROP_LOG:
        g_value_set_pointer (value, priv->log);
        break;
    case PROP_VERBOSE:
        g_value_set_boolean (value, priv->verbose);
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (wlim, prop_id, pspec);
    }
}


IBusWaylandIM *
ibus_wayland_im_new (const gchar *first_property_name, ...)
{
    va_list var_args;
    GObject *object;

    g_assert (first_property_name);

    va_start (var_args, first_property_name);
    object = g_object_new_valist (IBUS_TYPE_WAYLAND_IM,
                                  first_property_name,
                                  var_args);
    va_end (var_args);

    return IBUS_WAYLAND_IM (object);
}

gboolean
ibus_wayland_im_set_surface (IBusWaylandIM *wlim,
                             void          *surface)
{
    IBusWaylandIMPrivate *priv;
    struct wl_surface *_surface = surface;

    g_return_val_if_fail (wlim, FALSE);
    g_return_val_if_fail (_surface, FALSE);
    priv = ibus_wayland_im_get_instance_private (wlim);
    if (!priv->panel) {
        g_warning ("Need zwp_input_panel_v1 before the surface setting.");
        return FALSE;
    }
    priv->panel_surface =
        zwp_input_panel_v1_get_input_panel_surface(priv->panel, _surface);
    g_return_val_if_fail (priv->panel_surface, FALSE);
    zwp_input_panel_surface_v1_set_overlay_panel (priv->panel_surface);
    return TRUE;
}
