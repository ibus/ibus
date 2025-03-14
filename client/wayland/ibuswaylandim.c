/* -*- mode: C; c-basic-offset: 4; indent-tabs-mode: nil; -*- */
/* vim:set et sts=4: */
/* ibus - The Input Bus
 * Copyright (C) 2019-2025 Takao Fujiwara <takao.fujiwara1@gmail.com>
 * Copyright (C) 2013 Intel Corporation
 * Copyright (C) 2013-2025 Red Hat, Inc.
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
#include <ibusinternal.h>
#include <string.h>
#include <sys/time.h>
#include <wayland-client.h>
#include <xkbcommon/xkbcommon.h>

#include "input-method-unstable-v1-client-protocol.h"
#include "input-method-unstable-v2-client-protocol.h"
#include "text-input-unstable-v1-client-protocol.h"
#include "virtual-keyboard-unstable-v1-client-protocol.h"
#include "ibuswaylandim.h"

enum {
    PROP_0 = 0,
    PROP_BUS,
    PROP_DISPLAY,
    PROP_LOG,
    PROP_VERBOSE
};

enum {
    IBUS_FOCUS_IN,
    IBUS_FOCUS_OUT,
    LAST_SIGNAL,
};

typedef enum
{
    INPUT_METHOD_V1,
    INPUT_METHOD_V2,
} IMProtocolVersion;

struct zwp_input_method_context_union {
    union {
        struct zwp_input_method_context_v1 *context_v1;
        struct zwp_input_method_v2 *input_method_v2;
    } u;
};

struct zwp_keyboard_union {
    union {
        struct wl_keyboard *keyboard_v1;
        struct zwp_input_method_keyboard_grab_v2 *keyboard_v2;
    } u;
};

struct zwp_input_method_union {
    union {
        struct zwp_input_method_v1 *input_method_v1;
        struct zwp_input_method_v2 *input_method_v2;
    } u;
};

typedef struct _IBusWaylandSeat IBusWaylandSeat;
struct _IBusWaylandSeat
{
    struct wl_seat *seat;
    uint32_t wl_name;
    char *name;

    /* Input Method V2 */
    struct zwp_input_method_v2 *input_method_v2;
    struct zwp_input_method_keyboard_grab_v2 *keyboard_v2;
    struct zwp_virtual_keyboard_v1 *virtual_keyboard;
    struct zwp_input_popup_surface_v2 *input_popup_surface;
    gboolean active;
    gboolean pending_activate;
    gboolean pending_deactivate;
};

typedef struct _IBusWaylandIMPrivate IBusWaylandIMPrivate;
struct _IBusWaylandIMPrivate
{
    FILE *log;
    gboolean verbose;
    struct wl_display *display;
    IMProtocolVersion version;

    GPtrArray *seats;
    IBusWaylandSeat *seat;

    /* Input Method V1 */
    struct zwp_input_method_v1 *input_method_v1;
    struct zwp_input_method_context_v1 *context;
    struct wl_keyboard *keyboard_v1;
    struct zwp_input_panel_v1 *panel;
    struct zwp_input_panel_surface_v1 *panel_surface;

    /* Input Method V2 */
    struct zwp_input_method_manager_v2 *input_method_manager_v2;

    IBusBus *ibusbus;
    IBusInputContext *ibuscontext;
    IBusText *preedit_text;
    guint preedit_cursor_pos;
    guint preedit_mode;
    IBusModifierType modifiers;

    struct xkb_context *xkb_context;

    struct xkb_keymap *keymap;
    struct xkb_state *state;
    struct xkb_state *state_system;

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
    int32_t repeat_rate;
    int32_t repeat_delay;

    GCancellable *cancellable;
};

struct _IBusWaylandKeyEvent
{
    struct zwp_input_method_context_v1 *context;
    uint32_t serial;
    uint32_t time;
    uint32_t key;
    enum wl_keyboard_key_state state;
    xkb_keysym_t sym;
    uint32_t modifiers;
    IBusWaylandIM *wlim;
    int count;
    guint count_cb_id;
    guint repeat_rate_id;
    char *ibus_object_path;
    gboolean retval;
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

static struct wl_registry *_registry;
static struct zwp_virtual_keyboard_manager_v1  *_virtual_keyboard_manager;

static char _use_sync_mode = 1;

static guint wayland_im_signals[LAST_SIGNAL] = { 0 };

static void         input_method_deactivate
                              (void                               *data,
                               struct zwp_input_method_union      *input_method,
                               struct zwp_input_method_context_v1 *context);
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
static gboolean     ibus_wayland_im_post_key           (IBusWaylandIM *wlim,
                                                        uint32_t       key,
                                                        uint32_t
                                                                      modifiers,
                                                        uint32_t       state,
                                                        gboolean
                                                                      filtered);


static char
_get_char_env (const gchar *name,
               char         defval)
{
    const gchar *value = g_getenv (name);

    if (value == NULL)
        return defval;

    if (g_strcmp0 (value, "") == 0 ||
        g_strcmp0 (value, "0") == 0 ||
        g_strcmp0 (value, "false") == 0 ||
        g_strcmp0 (value, "False") == 0 ||
        g_strcmp0 (value, "FALSE") == 0) {
        return 0;
    } else if (!g_strcmp0 (value, "2")) {
        return 2;
    }

    return 1;
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
ibus_wayland_im_commit_text (IBusWaylandIM *wlim,
                             const char    *str)
{
    IBusWaylandIMPrivate *priv;
    g_return_if_fail (IBUS_IS_WAYLAND_IM (wlim));
    priv = ibus_wayland_im_get_instance_private (wlim);
    switch (priv->version) {
    case INPUT_METHOD_V1:
        zwp_input_method_context_v1_commit_string (priv->context,
                                                   priv->serial,
                                                   str);
        break;
    case INPUT_METHOD_V2:
        zwp_input_method_v2_commit_string (priv->seat->input_method_v2, str);
        zwp_input_method_v2_commit (priv->seat->input_method_v2, priv->serial);
        break;
    default:
        g_assert_not_reached ();
    }
}


static void
ibus_wayland_im_key (IBusWaylandIM *wlim,
                     uint32_t       serial,
                     uint32_t       time,
                     uint32_t       key,
                     uint32_t       state)
{
    IBusWaylandIMPrivate *priv;
    g_return_if_fail (IBUS_IS_WAYLAND_IM (wlim));
    priv = ibus_wayland_im_get_instance_private (wlim);
    switch (priv->version) {
    case INPUT_METHOD_V1:
        zwp_input_method_context_v1_key (priv->context,
                                         serial,
                                         time,
                                         key,
                                         state);
        break;
    case INPUT_METHOD_V2:
        zwp_virtual_keyboard_v1_key (priv->seat->virtual_keyboard,
                                     time, key, state);
        break;
    default:
        g_assert_not_reached ();
    }
}


static void
ibus_wayland_im_keysym (IBusWaylandIM *wlim,
                        uint32_t       serial,
                        guint          keyval,
                        uint32_t       state,
                        guint          modifiers)
{
    IBusWaylandIMPrivate *priv;
    g_return_if_fail (IBUS_IS_WAYLAND_IM (wlim));
    priv = ibus_wayland_im_get_instance_private (wlim);
    switch (priv->version) {
    case INPUT_METHOD_V1:
        zwp_input_method_context_v1_keysym (priv->context,
                                            serial,
                                            0,
                                            keyval,
                                            state,
                                            modifiers);
        break;
    case INPUT_METHOD_V2:
        g_warning ("TODO");
        break;
    default:
        g_assert_not_reached ();
    }
}


static void
_context_commit_text_cb (IBusInputContext *context,
                         IBusText         *text,
                         IBusWaylandIM    *wlim)
{
    ibus_wayland_im_commit_text (wlim, text->text);
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

    ibus_wayland_im_keysym (wlim,
                            priv->serial,
                            keyval,
                            state,
                            modifiers);
}


/**
 * ibus_wayland_im_update_preedit_style:
 * @wlim: An #IBusWaylandIM
 *
 * Convert RGB values to IBusAttrPreedit at first.
 * Convert IBusAttrPreedit to zwp_text_input_v1_preedit_style at second.
 *
 * RF. https://github.com/ibus/ibus/wiki/Wayland-Colors
 */
static void
ibus_wayland_im_update_preedit_style (IBusWaylandIM *wlim)
{
    IBusWaylandIMPrivate *priv;
    IBusAttrList *attrs;
    guint i;
    const char *str;
    uint32_t whole_wstyle = ZWP_TEXT_INPUT_V1_PREEDIT_STYLE_DEFAULT;
    uint32_t prev_start = 0;
    uint32_t prev_end = 0;

    g_return_if_fail (IBUS_IS_WAYLAND_IM (wlim));
    priv = ibus_wayland_im_get_instance_private (wlim);
    if (!priv->preedit_text)
        return;
    attrs = priv->preedit_text->attrs;
    if (!attrs)
        return;
    for (i = 0; ; i++) {
        IBusAttribute *attr = ibus_attr_list_get (attrs, i);
        IBusAttrPreedit istyle = IBUS_ATTR_PREEDIT_DEFAULT;
        uint32_t wstyle = ZWP_TEXT_INPUT_V1_PREEDIT_STYLE_DEFAULT;
        uint32_t start, end;
        if (attr == NULL)
                break;
        switch (attr->type) {
        case IBUS_ATTR_TYPE_UNDERLINE:
            istyle = IBUS_ATTR_PREEDIT_WHOLE;
            break;
        case IBUS_ATTR_TYPE_FOREGROUND:
            switch (attr->value) {
            case 0x7F7F7F: /* typing-booster */
                istyle = IBUS_ATTR_PREEDIT_PREDICTION;
                break;
            case 0xF90F0F: /* table */
                istyle = IBUS_ATTR_PREEDIT_PREFIX;
                break;
            case 0x1EDC1A: /* table */
                istyle = IBUS_ATTR_PREEDIT_SUFFIX;
                break;
            case 0xA40000: /* typing-booster, table */
                istyle = IBUS_ATTR_PREEDIT_ERROR_SPELLING;
                break;
            case 0xFF00FF: /* typing-booster */
                istyle = IBUS_ATTR_PREEDIT_ERROR_COMPOSE;
                break;
            case 0x0: /* Japanese */
            case 0xFF000000:
                break;
            case 0xFFFFFF: /* hangul */
            case 0xFFFFFFFF:
                istyle = IBUS_ATTR_PREEDIT_SELECTION;
                break;
            default: /* Custom */
                istyle = IBUS_ATTR_PREEDIT_NONE;
            }
            break;
        case IBUS_ATTR_TYPE_BACKGROUND:
            switch (attr->value) {
            case 0xC8C8F0: /* Japanese */
            case 0xFFC8C8F0:
                istyle = IBUS_ATTR_PREEDIT_SELECTION;
                break;
            default:; /* Custom */
            }
            break;
        default:
            istyle = IBUS_ATTR_PREEDIT_NONE;
        }
        switch (istyle) {
        case IBUS_ATTR_PREEDIT_NONE:
            wstyle = ZWP_TEXT_INPUT_V1_PREEDIT_STYLE_NONE;
            break;
        case IBUS_ATTR_PREEDIT_WHOLE:
            wstyle = ZWP_TEXT_INPUT_V1_PREEDIT_STYLE_UNDERLINE;
            break;
        case IBUS_ATTR_PREEDIT_SELECTION:
            wstyle = ZWP_TEXT_INPUT_V1_PREEDIT_STYLE_SELECTION;
            break;
        case IBUS_ATTR_PREEDIT_PREDICTION:
            wstyle = ZWP_TEXT_INPUT_V1_PREEDIT_STYLE_INACTIVE;
            break;
        case IBUS_ATTR_PREEDIT_PREFIX:
            wstyle = ZWP_TEXT_INPUT_V1_PREEDIT_STYLE_HIGHLIGHT;
            break;
        case IBUS_ATTR_PREEDIT_SUFFIX:
            wstyle = ZWP_TEXT_INPUT_V1_PREEDIT_STYLE_INACTIVE;
            break;
        case IBUS_ATTR_PREEDIT_ERROR_SPELLING:
            wstyle = ZWP_TEXT_INPUT_V1_PREEDIT_STYLE_INCORRECT;
            break;
        case IBUS_ATTR_PREEDIT_ERROR_COMPOSE:
            wstyle = ZWP_TEXT_INPUT_V1_PREEDIT_STYLE_INCORRECT;
            break;
        default:;
        }
        if (wstyle == ZWP_TEXT_INPUT_V1_PREEDIT_STYLE_DEFAULT)
            continue;
        str = priv->preedit_text->text;
        start = g_utf8_offset_to_pointer (str, attr->start_index) - str;
        end = g_utf8_offset_to_pointer (str, attr->end_index) - str;
        /* Double styles cannot be applied likes the underline and
         * preedit color. */
        if (start == 0 && strlen (str) == end &&
            (i > 0 || ibus_attr_list_get (attrs, i + 1))) {
            whole_wstyle = wstyle;
            continue;
        }
        if (end < prev_start) {
            if (priv->log) {
                fprintf (priv->log,
                         "Reverse order is not supported in end %d for %s "
                         "against start %d.\n", end, str, prev_start);
                fflush (priv->log);
            }
            continue;
        }
        if (prev_end > end) {
            if (priv->log) {
                fprintf (priv->log,
                         "Nested styles are not supported in end %d for %s "
                         "against end %d.\n", end, str, prev_end);
                fflush (priv->log);
            }
            continue;
        }
        if (prev_end > start && prev_start >= start)
            start = prev_end;
        if (start >= end) {
            if (priv->log) {
                fprintf (priv->log, "Wrong start %d and end %d for %s.\n",
                         start, end, str);
                fflush (priv->log);
            }
            return;
        }
        zwp_input_method_context_v1_preedit_styling (priv->context,
                                                     start,
                                                     end - start,
                                                     wstyle);
        prev_start = start;
        prev_end = end;
    }
    if (whole_wstyle != ZWP_TEXT_INPUT_V1_PREEDIT_STYLE_DEFAULT) {
        uint32_t whole_start = 0;
        uint32_t whole_end = strlen (str);
        uint32_t start, end;
        for (i = 0; ; i++) {
            IBusAttribute *attr = ibus_attr_list_get (attrs, i);
            if (!attr)
                break;
            start = g_utf8_offset_to_pointer (str, attr->start_index) - str;
            end = g_utf8_offset_to_pointer (str, attr->end_index) - str;
            if (start == 0 && strlen (str) == end)
                continue;
            if (start == 0) {
                whole_start = end;
            } else if (strlen (str) == end) {
                whole_end = start;
            } else {
                whole_end = start;
                if (whole_start < whole_end) {
                    zwp_input_method_context_v1_preedit_styling (
                            priv->context,
                            whole_start,
                            whole_end - whole_start,
                            whole_wstyle);
                }
                whole_start = end;
                whole_end = strlen (str);
            }
        }
        if (whole_start < whole_end) {
            zwp_input_method_context_v1_preedit_styling (
                priv->context,
                whole_start,
                whole_end - whole_start,
                whole_wstyle);
        }
    }
}

static void
_context_show_preedit_text_cb (IBusInputContext *context,
                               IBusWaylandIM    *wlim)
{
    IBusWaylandIMPrivate *priv;
    uint32_t cursor;
    const char *commit = "";
    g_return_if_fail (IBUS_IS_WAYLAND_IM (wlim));
    priv = ibus_wayland_im_get_instance_private (wlim);
    /* CURSOR is byte offset.  */
    cursor =
        g_utf8_offset_to_pointer (priv->preedit_text->text,
                                  priv->preedit_cursor_pos) -
        priv->preedit_text->text;

    if (priv->preedit_mode == IBUS_ENGINE_PREEDIT_COMMIT)
        commit = priv->preedit_text->text;
    switch (priv->version) {
    case INPUT_METHOD_V1:
        zwp_input_method_context_v1_preedit_cursor (priv->context,
                                                    cursor);
        ibus_wayland_im_update_preedit_style (wlim);
        zwp_input_method_context_v1_preedit_string (priv->context,
                                                    priv->serial,
                                                    priv->preedit_text->text,
                                                    commit);
        break;
    case INPUT_METHOD_V2:
        zwp_input_method_v2_set_preedit_string  (
                priv->seat->input_method_v2,
                priv->preedit_text->text,
                cursor,
                strlen (priv->preedit_text->text));
        zwp_input_method_v2_commit (priv->seat->input_method_v2, priv->serial);
        break;
    default:
        g_assert_not_reached ();
    }
}


static void
_context_hide_preedit_text_cb (IBusInputContext *context,
                               IBusWaylandIM    *wlim)
{
    IBusWaylandIMPrivate *priv;
    g_return_if_fail (IBUS_IS_WAYLAND_IM (wlim));
    priv = ibus_wayland_im_get_instance_private (wlim);
    switch (priv->version) {
    case INPUT_METHOD_V1:
        zwp_input_method_context_v1_preedit_string (priv->context,
                                                    priv->serial,
                                                    "",
                                                    "");
        break;
    case INPUT_METHOD_V2:
        zwp_input_method_v2_set_preedit_string  (priv->seat->input_method_v2,
                                                 "", 0, 0);
        zwp_input_method_v2_commit (priv->seat->input_method_v2, priv->serial);
        break;
    default:
        g_assert_not_reached ();
    }
}


static void
_context_update_preedit_text_cb (IBusInputContext *context,
                                 IBusText         *text,
                                 gint              cursor_pos,
                                 gboolean          visible,
                                 guint             mode,
                                 IBusWaylandIM    *wlim)
{
    IBusWaylandIMPrivate *priv;
    g_return_if_fail (IBUS_IS_WAYLAND_IM (wlim));
    priv = ibus_wayland_im_get_instance_private (wlim);
    if (priv->preedit_text)
        g_object_unref (priv->preedit_text);
    priv->preedit_text = g_object_ref_sink (text);
    priv->preedit_cursor_pos = cursor_pos;
    priv->preedit_mode = mode;

    if (visible)
        _context_show_preedit_text_cb (context, wlim);
    else
        _context_hide_preedit_text_cb (context, wlim);
}


static void
handle_surrounding_text (void                                  *data,
                         struct zwp_input_method_context_union *context,
                         const char                            *text,
                         uint32_t                               cursor,
                         uint32_t                               anchor)
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
context_surrounding_text_v1 (void                               *data,
                             struct zwp_input_method_context_v1 *context_v1,
                             const char                         *text,
                             uint32_t                            cursor,
                             uint32_t                            anchor)
{
    struct zwp_input_method_context_union context;
    context.u.context_v1 = context_v1;
    handle_surrounding_text (data, &context, text, cursor, anchor);
}


static void
context_reset_v1 (void                               *data,
                  struct zwp_input_method_context_v1 *context_v1)
{
}


static void
context_content_type_v1 (void                               *data,
                         struct zwp_input_method_context_v1 *context_v1,
                         uint32_t                            hint,
                         uint32_t                            purpose)
{
}


static void
context_invoke_action_v1 (void                               *data,
                          struct zwp_input_method_context_v1 *context_v1,
                          uint32_t                            button,
                          uint32_t                            index)
{
}


static void
context_commit_state_v1 (void                               *data,
                         struct zwp_input_method_context_v1 *context_v1,
                         uint32_t                            serial)
{
    IBusWaylandIM *wlim = data;
    IBusWaylandIMPrivate *priv;
    g_return_if_fail (IBUS_IS_WAYLAND_IM (wlim));
    priv = ibus_wayland_im_get_instance_private (wlim);
    priv->serial = serial;
}


static void
context_preferred_language_v1 (void                               *data,
                               struct zwp_input_method_context_v1 *context_v1,
                               const char                         *language)
{
}


static const struct zwp_input_method_context_v1_listener context_listener_v1 = {
    .surrounding_text = context_surrounding_text_v1,
    .reset = context_reset_v1,
    .content_type = context_content_type_v1,
    .invoke_action= context_invoke_action_v1,
    .commit_state = context_commit_state_v1,
    .preferred_language = context_preferred_language_v1
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
    names.options = g_getenv ("XKB_DEFAULT_OPTIONS");
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
input_method_keyboard_keymap (void                      *data,
                              struct zwp_keyboard_union *keyboard,
                              uint32_t                   format,
                              int32_t                    fd,
                              uint32_t                   size)
{
    IBusWaylandIM *wlim = data;
    IBusWaylandIMPrivate *priv;
    struct xkb_keymap *keymap;

    g_return_if_fail (IBUS_IS_WAYLAND_IM (wlim));
    priv = ibus_wayland_im_get_instance_private (wlim);

    if (priv->version != INPUT_METHOD_V1) {
        zwp_virtual_keyboard_v1_keymap (priv->seat->virtual_keyboard,
                                        format, fd, size);
    }
    if (priv->keymap && priv->state && priv->state_system)
        return;
    keymap = create_system_xkb_keymap (priv->xkb_context, format, fd, size);
    if (keymap && !ibus_wayland_im_update_xkb_state (wlim, keymap))
        xkb_keymap_unref (keymap);
    if (priv->state)
        priv->state_system = xkb_state_ref (priv->state);
}


static void
ibus_wayland_seat_destroy (gpointer data)
{
    IBusWaylandSeat *seat = (IBusWaylandSeat *)data;
    g_return_if_fail (seat);
    g_free (seat->name);
    zwp_input_popup_surface_v2_destroy (seat->input_popup_surface);
    zwp_virtual_keyboard_v1_destroy (seat->virtual_keyboard);
    zwp_input_method_keyboard_grab_v2_destroy (seat->keyboard_v2);
    zwp_input_method_v2_destroy (seat->input_method_v2);
    g_slice_free (IBusWaylandSeat, seat);
}


static IBusWaylandSeat *
_get_seat_with_name (GPtrArray *seats,
                     uint32_t   wl_name,
                     guint     *index)
{
    guint i;
    g_return_val_if_fail (seats, NULL);
    for (i = 0; i < seats->len; ++i) {
        IBusWaylandSeat *seat = g_ptr_array_index (seats, i);
        if (seat->wl_name == wl_name) {
            if (index)
                *index = i;
            return seat;
        }
    }
    return NULL;
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

    /* ibus_wayland_im_commit_text() does not work without the activation. */
    switch (priv->version) {
    case INPUT_METHOD_V1:
        if (!priv->context)
            return FALSE;
        break;
    case INPUT_METHOD_V2:
        if (!priv->seat->active)
            return FALSE;
        break;
    default:
        g_assert_not_reached ();
    }
    if (!priv->state)
        return FALSE;
    if (!filtered && (state != WL_KEYBOARD_KEY_STATE_RELEASED)) {
        ch = xkb_state_key_get_utf32 (priv->state, code);
        if (!(modifiers & IBUS_MODIFIER_MASK & ~IBUS_SHIFT_MASK) &&
            ch && ch != '\n' && ch != '\b' && ch != '\r' && ch != '\t' &&
            ch != '\033' && ch != '\x7f') {
            gchar buff[8] = { 0, };
            buff[g_unichar_to_utf8 (ch, buff)] = '\0';
            ibus_wayland_im_commit_text (wlim, buff);
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
    IBusWaylandKeyEvent *event = (IBusWaylandKeyEvent *)user_data;
    GError *error = NULL;
    gboolean retval = ibus_input_context_process_key_event_async_finish (
            context,
            res,
            &error);
    IBusWaylandIMPrivate *priv = NULL;

    if (error != NULL) {
        if (event && event->wlim && IBUS_IS_WAYLAND_IM (event->wlim)) {
            priv = ibus_wayland_im_get_instance_private (event->wlim);
        }
        if (priv && priv->log) {
            fprintf (priv->log, "Process Key Event failed: %s\n",
                     error->message);
            fflush (priv->log);
        } else {
            g_warning ("Process Key Event failed: %s", error->message);
        }
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
        ibus_wayland_im_key (event->wlim,
                             event->serial,
                             event->time,
                             event->key,
                             event->state);
    }

    g_slice_free (IBusWaylandKeyEvent, event);
}


static void
_process_key_event_reply_done (GObject      *object,
                               GAsyncResult *res,
                               gpointer      user_data)
{
    IBusInputContext *context = (IBusInputContext *)object;
    IBusWaylandKeyEvent *event = (IBusWaylandKeyEvent *)user_data;
    GError *error = NULL;
    gboolean retval = ibus_input_context_process_key_event_async_finish (
            context,
            res,
            &error);
    if (error != NULL) {
        IBusWaylandIMPrivate *priv = NULL;
        if (event && event->wlim && IBUS_IS_WAYLAND_IM (event->wlim)) {
            priv = ibus_wayland_im_get_instance_private (event->wlim);
        }
        if (priv && priv->log) {
            fprintf (priv->log, "Process Key Event failed: %s\n",
                     error->message);
            fflush (priv->log);
        } else {
            g_warning ("Process Key Event failed: %s", error->message);
        }
        g_error_free (error);
    }
    g_return_if_fail (event);
    event->retval = retval;
    event->count = 0;
    g_source_remove (event->count_cb_id);
}


static gboolean
_process_key_event_count_cb (gpointer user_data)
{
    IBusWaylandKeyEvent *event = (IBusWaylandKeyEvent *)user_data;
    g_return_val_if_fail (event, G_SOURCE_REMOVE);
    if (!event->count)
        return G_SOURCE_REMOVE;
    /* Wait for about 10 secs. */
    if (event->count++ == 10000) {
        event->count = 0;
        return G_SOURCE_REMOVE;
    }
    return G_SOURCE_CONTINUE;
}


static void
_process_key_event_sync (IBusWaylandIM       *wlim,
                         IBusWaylandKeyEvent *event)
{
    IBusWaylandIMPrivate *priv;
    gboolean retval;

    g_return_if_fail (IBUS_IS_WAYLAND_IM (wlim));
    g_assert (event);
    priv = ibus_wayland_im_get_instance_private (wlim);
    if (!priv->ibuscontext)
        return;
    retval = ibus_input_context_process_key_event (priv->ibuscontext,
                                                   event->sym,
                                                   event->key,
                                                   event->modifiers);
    ibus_input_context_post_process_key_event (priv->ibuscontext);
    retval = ibus_wayland_im_post_key (wlim,
                                       event->key,
                                       event->modifiers,
                                       event->state,
                                       retval);
    if (!retval) {
        ibus_wayland_im_key (wlim,
                             event->serial,
                             event->time,
                             event->key,
                             event->state);
    }
}


static void
_process_key_event_async (IBusWaylandIM       *wlim,
                          IBusWaylandKeyEvent *event)
{
    IBusWaylandIMPrivate *priv;
    IBusWaylandKeyEvent *async_event;

    g_return_if_fail (IBUS_IS_WAYLAND_IM (wlim));
    g_assert (event);
    priv = ibus_wayland_im_get_instance_private (wlim);
    async_event = g_slice_new0 (IBusWaylandKeyEvent);
    if (!async_event) {
        if (priv->log) {
            fprintf (priv->log, "Cannot allocate async data\n");
            fflush (priv->log);
        } else {
            g_warning ("Cannot allocate async data");
        }
        _process_key_event_sync (wlim, event);
        return;
    }
    async_event->context = priv->context;
    async_event->serial = event->serial;
    async_event->time = event->time;
    async_event->key = event->key;
    async_event->modifiers = event->modifiers & ~IBUS_RELEASE_MASK;
    async_event->state = event->state;
    async_event->wlim = wlim;
    ibus_input_context_process_key_event_async (priv->ibuscontext,
                                                event->sym,
                                                event->key,
                                                event->modifiers,
                                                -1,
                                                NULL,
                                                _process_key_event_done,
                                                async_event);
}


static void
_process_key_event_hybrid_async (IBusWaylandIM       *wlim,
                                 IBusWaylandKeyEvent *event)
{
    IBusWaylandIMPrivate *priv;
    GSource *source;
    IBusWaylandKeyEvent *async_event = NULL;

    g_return_if_fail (IBUS_IS_WAYLAND_IM (wlim));
    g_assert (event);
    priv = ibus_wayland_im_get_instance_private (wlim);
    source = g_timeout_source_new (1);
    if (source)
        async_event = g_slice_new0 (IBusWaylandKeyEvent);
    if (!async_event) {
        if (priv->log) {
            fprintf (priv->log, "Cannot wait for the reply of the "
                                "process key event.\n");
            fflush (priv->log);
        } else {
            g_warning ("Cannot wait for the reply of the process key event.");
        }
        _process_key_event_sync (wlim, event);
        if (source)
            g_source_destroy (source);
        return;
    }
    async_event->count = 1;
    async_event->wlim = wlim;
    g_source_attach (source, NULL);
    g_source_unref (source);
    async_event->count_cb_id = g_source_get_id (source);
    ibus_input_context_process_key_event_async (priv->ibuscontext,
                                                event->sym,
                                                event->key,
                                                event->modifiers,
                                                -1,
                                                NULL,
                                                _process_key_event_reply_done,
                                                async_event);
    g_source_set_callback (source, _process_key_event_count_cb,
                           async_event, NULL);
    while (async_event->count)
        g_main_context_iteration (NULL, TRUE);
    /* #2498 Checking source->ref_count might cause Nautilus hang up
     */
    if (priv->ibuscontext) {
        async_event->retval = ibus_wayland_im_post_key (wlim,
                                                        event->key,
                                                        event->modifiers,
                                                        event->state,
                                                        async_event->retval);
    }
    if (priv->ibuscontext && !async_event->retval) {
        ibus_wayland_im_key (wlim,
                             event->serial,
                             event->time,
                             event->key,
                             event->state);
    }
    g_slice_free (IBusWaylandKeyEvent, async_event);
}


static gboolean
_process_key_event_repeat_rate_cb (gpointer user_data)
{
    IBusWaylandKeyEvent *event = (IBusWaylandKeyEvent *)user_data;
    IBusWaylandIM *wlim;
    IBusWaylandIMPrivate *priv;

    g_return_val_if_fail (event, G_SOURCE_REMOVE);

    wlim = event->wlim;
    if (!IBUS_IS_WAYLAND_IM (wlim)) {
        g_warning ("Failed BUS_IS_WAYLAND_IM (wlim)");
        event->repeat_rate_id = 0;
        return G_SOURCE_REMOVE;
    }
    priv = ibus_wayland_im_get_instance_private (wlim);

    if (!priv->ibuscontext) {
        event->repeat_rate_id = 0;
        return G_SOURCE_REMOVE;
    }
    if (g_strcmp0 (event->ibus_object_path,
                   g_dbus_proxy_get_object_path (
                           G_DBUS_PROXY (priv->ibuscontext)))) {
        event->repeat_rate_id = 0;
        return G_SOURCE_REMOVE;
    }
    switch (_use_sync_mode) {
    case 1:
        _process_key_event_sync (wlim, event);
        break;
    case 2:
        _process_key_event_hybrid_async (wlim, event);
        break;
    default:
        _process_key_event_async (wlim, event);
    }
    return G_SOURCE_CONTINUE;
}


static gboolean
_process_key_event_repeat_delay_cb (gpointer user_data)
{
    IBusWaylandKeyEvent *event = (IBusWaylandKeyEvent *)user_data;
    IBusWaylandIM *wlim;
    IBusWaylandIMPrivate *priv;
    GSource *source;

    g_return_val_if_fail (event, G_SOURCE_REMOVE);

    wlim = event->wlim;
    if (!IBUS_IS_WAYLAND_IM (wlim)) {
        g_warning ("Failed BUS_IS_WAYLAND_IM (wlim)");
        event->count_cb_id = 0;
        return G_SOURCE_REMOVE;
    }
    priv = ibus_wayland_im_get_instance_private (wlim);

    /* The key release event was sent to non-Wayland apps likes xterm. */
    if (!priv->ibuscontext) {
        event->count_cb_id = 0;
        /* Stop the infinite Return keys in non-Wayland apps likes xterm in
         * the Sway desktop session.
         * But priv->context in NULL in the Wayland input-method V1 here.
         */
        if (priv->version != INPUT_METHOD_V1) {
            ibus_wayland_im_key(event->wlim,
                                event->serial,
                                event->time + priv->repeat_delay,
                                event->key,
                                WL_KEYBOARD_KEY_STATE_RELEASED);
        }
        return G_SOURCE_REMOVE;
    }
    /* The focus is changed. */
    if (g_strcmp0 (event->ibus_object_path,
                   g_dbus_proxy_get_object_path (
                           G_DBUS_PROXY (priv->ibuscontext)))) {
        event->count_cb_id = 0;
        g_clear_pointer (&event->ibus_object_path, g_free);
        return G_SOURCE_REMOVE;
    }

    if (event->count)
        return G_SOURCE_CONTINUE;

    event->count = 1;
    switch (_use_sync_mode) {
    case 1:
        _process_key_event_sync (event->wlim, event);
        break;
    case 2:
        _process_key_event_hybrid_async (event->wlim, event);
        break;
    default:
        _process_key_event_async (event->wlim, event);
    }
    source = g_timeout_source_new (priv->repeat_rate);
    g_source_attach (source, NULL);
    g_source_unref (source);
    event->repeat_rate_id = g_source_get_id (source);
    g_source_set_callback (source, _process_key_event_repeat_rate_cb,
                           event, NULL);
    return G_SOURCE_CONTINUE;
}


static gboolean
key_event_check_repeat (IBusWaylandIM       *wlim,
                        IBusWaylandKeyEvent *event)
{
    IBusWaylandIMPrivate *priv;
    int i;
    const guint16 *repeat_ignore = IBUS_COMPOSE_IGNORE_KEYLIST;
    int repeat_ignore_length = G_N_ELEMENTS (IBUS_COMPOSE_IGNORE_KEYLIST);
    GSource *source;
    static IBusWaylandKeyEvent repeating_event = { 0, };

    g_return_val_if_fail (IBUS_IS_WAYLAND_IM (wlim), FALSE);
    priv = ibus_wayland_im_get_instance_private (wlim);
    if G_UNLIKELY (!event->sym)
        return FALSE;
    /* FIXME: Should consider if xkb_keymap_key_repeats() is better. */
    for (i = 0; i < repeat_ignore_length; i++) {
        if (event->sym == (uint32_t)repeat_ignore[i])
            return FALSE;
    }

    if (event->state == WL_KEYBOARD_KEY_STATE_PRESSED) {
        if (repeating_event.repeat_rate_id) {
            g_source_remove (repeating_event.repeat_rate_id);
            repeating_event.repeat_rate_id = 0;
        }
        if (repeating_event.count_cb_id) {
            /* Double KeyPress happen likes Ctrl-a-b */
            g_source_remove (repeating_event.count_cb_id);
            repeating_event.count_cb_id = 0;
        }
        g_clear_pointer (&repeating_event.ibus_object_path, g_free);
        if (!priv->ibuscontext)
            return FALSE;
        source = g_timeout_source_new (priv->repeat_delay);
        g_source_attach (source, NULL);
        g_source_unref (source);
        /* Copy the keycode and modifier since Ctrl key has a delay. */
        memcpy (&repeating_event, event, sizeof (IBusWaylandKeyEvent));
        repeating_event.count_cb_id = g_source_get_id (source);
        repeating_event.count = 0;
        repeating_event.ibus_object_path =
                       g_strdup (g_dbus_proxy_get_object_path (
                               G_DBUS_PROXY (priv->ibuscontext)));
        g_source_set_callback (source, _process_key_event_repeat_delay_cb,
                               &repeating_event, NULL);
    } else {
        if (repeating_event.repeat_rate_id) {
            g_source_remove (repeating_event.repeat_rate_id);
            repeating_event.repeat_rate_id = 0;
        }
        if (repeating_event.count_cb_id) {
            g_source_remove (repeating_event.count_cb_id);
            repeating_event.count_cb_id = 0;
        }
        g_clear_pointer (&repeating_event.ibus_object_path, g_free);
    }
    return TRUE;
}


static void
input_method_keyboard_key (void                      *data,
                           struct zwp_keyboard_union *keyboard,
                           uint32_t                   serial,
                           uint32_t                   time,
                           uint32_t                   key,
                           uint32_t                   state)
{
    IBusWaylandIM *wlim = data;
    IBusWaylandIMPrivate *priv;
    IBusWaylandKeyEvent event = { 0, };
    uint32_t code;

    g_return_if_fail (IBUS_IS_WAYLAND_IM (wlim));
    priv = ibus_wayland_im_get_instance_private (wlim);
    if (!priv->state)
        return;

    if (!priv->ibuscontext) {
        gboolean retval = ibus_wayland_im_post_key (wlim,
                                                    key,
                                                    priv->modifiers,
                                                    state,
                                                    FALSE);
        if (!retval)
            ibus_wayland_im_key (wlim, serial, time, key, state);
        return;
    }

    event.serial = serial;
    event.time = time;
    event.key = key;
    event.state = state;
    code = key + 8;
    event.sym = 0;
    /* xkb_key_get_syms() does not return the capital syms with Shift key. */
    if (priv->state_system)
        event.sym = xkb_state_key_get_one_sym (priv->state_system, code);
    if (event.sym != IBUS_KEY_Multi_key)
        event.sym = xkb_state_key_get_one_sym (priv->state, code);
    event.modifiers = priv->modifiers;
    if (state == WL_KEYBOARD_KEY_STATE_RELEASED)
        event.modifiers |= IBUS_RELEASE_MASK;
    event.wlim = wlim;

    key_event_check_repeat (wlim, &event);
    switch (_use_sync_mode) {
    case 1:
        return _process_key_event_sync (wlim, &event);
    case 2:
        return _process_key_event_hybrid_async (wlim, &event);
    default:
        return _process_key_event_async (wlim, &event);
    }
}


static void
input_method_keyboard_modifiers (void                      *data,
                                 struct zwp_keyboard_union *keyboard,
                                 uint32_t                   serial,
                                 uint32_t                   mods_depressed,
                                 uint32_t                   mods_latched,
                                 uint32_t                   mods_locked,
                                 uint32_t                   group)
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

    switch (priv->version) {
    case INPUT_METHOD_V1:
        zwp_input_method_context_v1_modifiers (priv->context, serial,
                                               mods_depressed, mods_latched,
                                               mods_locked, group);
        break;
    case INPUT_METHOD_V2:
        zwp_virtual_keyboard_v1_modifiers (priv->seat->virtual_keyboard,
                                           mods_depressed, mods_latched,
                                           mods_locked, group);
        break;
    default:
        g_assert_not_reached ();
    }
}


static void
input_method_keyboard_repeat_info (void                      *data,
                                   struct zwp_keyboard_union *keyboard,
                                   int32_t                    rate,
                                   int32_t                    delay)
{
    IBusWaylandIM *wlim = data;
    IBusWaylandIMPrivate *priv;

    g_return_if_fail (IBUS_IS_WAYLAND_IM (wlim));
    priv = ibus_wayland_im_get_instance_private (wlim);
    priv->repeat_rate = rate;
    priv->repeat_delay = delay;
    if (priv->verbose) {
        fprintf (priv->log, "keyboard repeat info rate %d delay %d\n",
                rate, delay);
        fflush (priv->log);
    }
}


static void
input_method_keyboard_keymap_v1 (void               *data,
                                 struct wl_keyboard *keyboard_v1,
                                 uint32_t            format,
                                 int32_t             fd,
                                 uint32_t            size)
{
    struct zwp_keyboard_union keyboard;
    keyboard.u.keyboard_v1 = keyboard_v1;
    input_method_keyboard_keymap (data, &keyboard, format, fd, size);
}


static void
input_method_keyboard_key_v1 (void               *data,
                              struct wl_keyboard *keyboard_v1,
                              uint32_t            serial,
                              uint32_t            time,
                              uint32_t            key,
                              uint32_t            state)
{
    struct zwp_keyboard_union keyboard;
    keyboard.u.keyboard_v1 = keyboard_v1;
    input_method_keyboard_key (data, &keyboard, serial, time, key, state);
}


static void
input_method_keyboard_modifiers_v1 (void               *data,
                                    struct wl_keyboard *keyboard_v1,
                                    uint32_t            serial,
                                    uint32_t            mods_depressed,
                                    uint32_t            mods_latched,
                                    uint32_t            mods_locked,
                                    uint32_t            group)
{
    struct zwp_keyboard_union keyboard;
    keyboard.u.keyboard_v1 = keyboard_v1;
    input_method_keyboard_modifiers (data,
                                     &keyboard,
                                     serial,
                                     mods_depressed,
                                     mods_latched,
                                     mods_locked,
                                     group);
}


static void
input_method_keyboard_repeat_info_v1 (void               *data,
                                      struct wl_keyboard *keyboard_v1,
                                      int32_t             rate,
                                      int32_t             delay)
{
    struct zwp_keyboard_union keyboard;
    keyboard.u.keyboard_v1 = keyboard_v1;
    input_method_keyboard_repeat_info (data, &keyboard, rate, delay);
}


static void
input_method_keyboard_keymap_v2 (void    *data,
                                 struct zwp_input_method_keyboard_grab_v2
                                         *keyboard_v2,
                                 uint32_t format,
                                 int32_t  fd,
                                 uint32_t size)
{
    struct zwp_keyboard_union keyboard;
    keyboard.u.keyboard_v2 = keyboard_v2;
    input_method_keyboard_keymap (data, &keyboard, format, fd, size);
}


static void
input_method_keyboard_key_v2 (void    *data,
                              struct zwp_input_method_keyboard_grab_v2
                                      *keyboard_v2,
                              uint32_t serial,
                              uint32_t time,
                              uint32_t key,
                              uint32_t state)
{
    struct zwp_keyboard_union keyboard;
    keyboard.u.keyboard_v2 = keyboard_v2;
    input_method_keyboard_key (data, &keyboard, serial, time, key, state);
}


static void
input_method_keyboard_modifiers_v2 (void    *data,
                                    struct zwp_input_method_keyboard_grab_v2
                                            *keyboard_v2,
                                    uint32_t serial,
                                    uint32_t mods_depressed,
                                    uint32_t mods_latched,
                                    uint32_t mods_locked,
                                    uint32_t group)
{
    struct zwp_keyboard_union keyboard;
    keyboard.u.keyboard_v2 = keyboard_v2;
    input_method_keyboard_modifiers (data,
                                     &keyboard,
                                     serial,
                                     mods_depressed,
                                     mods_latched,
                                     mods_locked,
                                     group);
}


static void
input_method_keyboard_repeat_info_v2 (void   *data,
                                      struct zwp_input_method_keyboard_grab_v2
                                             *keyboard_v2,
                                      int32_t rate,
                                      int32_t delay)
{
    struct zwp_keyboard_union keyboard;
    keyboard.u.keyboard_v2 = keyboard_v2;
    input_method_keyboard_repeat_info (data, &keyboard, rate, delay);
}


static const struct wl_keyboard_listener keyboard_listener_v1 = {
    .keymap = input_method_keyboard_keymap_v1,
    .enter = NULL, /* enter */
    .leave = NULL, /* leave */
    .key = input_method_keyboard_key_v1,
    .modifiers = input_method_keyboard_modifiers_v1,
#ifdef WL_KEYBOARD_REPEAT_INFO_SINCE_VERSION
    .repeat_info = input_method_keyboard_repeat_info_v1
#endif
};


static const struct zwp_input_method_keyboard_grab_v2_listener
        keyboard_listener_v2 = {
    .keymap = input_method_keyboard_keymap_v2,
    .key = input_method_keyboard_key_v2,
    .modifiers = input_method_keyboard_modifiers_v2,
    .repeat_info = input_method_keyboard_repeat_info_v2,
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
        guint32 capabilities = IBUS_CAP_FOCUS | IBUS_CAP_PREEDIT_TEXT;
        priv->ibuscontext = context;

        g_signal_connect (priv->ibuscontext, "commit-text",
                          G_CALLBACK (_context_commit_text_cb),
                          wlim);
        g_signal_connect (priv->ibuscontext, "forward-key-event",
                          G_CALLBACK (_context_forward_key_event_cb),
                          wlim);

        g_signal_connect (priv->ibuscontext, "update-preedit-text-with-mode",
                          G_CALLBACK (_context_update_preedit_text_cb),
                          wlim);
        g_signal_connect (priv->ibuscontext, "show-preedit-text",
                          G_CALLBACK (_context_show_preedit_text_cb),
                          wlim);
        g_signal_connect (priv->ibuscontext, "hide-preedit-text",
                          G_CALLBACK (_context_hide_preedit_text_cb),
                          wlim);
    
#ifdef ENABLE_SURROUNDING
        capabilities |= IBUS_CAP_SURROUNDING_TEXT;
#endif
        if (_use_sync_mode == 1)
            capabilities |= IBUS_CAP_SYNC_PROCESS_KEY_V2;
        ibus_input_context_set_capabilities (priv->ibuscontext,
                                             capabilities);
        ibus_input_context_set_client_commit_preedit (priv->ibuscontext, TRUE);
        if (_use_sync_mode == 1) {
            ibus_input_context_set_post_process_key_event (priv->ibuscontext,
                                                           TRUE);
        }
        ibus_input_context_focus_in (priv->ibuscontext);
        g_signal_emit (wlim,
                       wayland_im_signals[IBUS_FOCUS_IN],
                       0,
                       g_dbus_proxy_get_object_path (
                               G_DBUS_PROXY (priv->ibuscontext)));
    }
}


static void
input_method_activate (void                               *data,
                       struct zwp_input_method_union      *input_method,
                       struct zwp_input_method_context_v1 *context)
{
    IBusWaylandIM *wlim = data;
    IBusWaylandIMPrivate *priv;

    g_return_if_fail (IBUS_IS_WAYLAND_IM (wlim));
    priv = ibus_wayland_im_get_instance_private (wlim);
    if (priv->context)
        input_method_deactivate (data, input_method, context);

    priv->context = context;
    if (context)
        priv->serial = 0;

    switch (priv->version) {
    case INPUT_METHOD_V1:
        zwp_input_method_context_v1_add_listener (context,
                                                  &context_listener_v1,
                                                  wlim);
        priv->keyboard_v1 = zwp_input_method_context_v1_grab_keyboard (context);
        wl_keyboard_add_listener (priv->keyboard_v1,
                                  &keyboard_listener_v1,
                                  wlim);
        break;
    case INPUT_METHOD_V2:
        priv->seat->keyboard_v2 = zwp_input_method_v2_grab_keyboard (
                input_method->u.input_method_v2);
        zwp_input_method_keyboard_grab_v2_add_listener (
                priv->seat->keyboard_v2,
                &keyboard_listener_v2,
                wlim);
        break;
    default:
        g_assert_not_reached ();
    }

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
                         struct zwp_input_method_union      *input_method,
                         struct zwp_input_method_context_v1 *context)
{
    IBusWaylandIM *wlim = data;
    IBusWaylandIMPrivate *priv;

    g_return_if_fail (IBUS_IS_WAYLAND_IM (wlim));
    priv = ibus_wayland_im_get_instance_private (wlim);
    if (priv->cancellable) {
        /* Cancel any ongoing create input context request.  */
        g_cancellable_cancel (priv->cancellable);
        g_clear_object (&priv->cancellable);
    }


    if (priv->ibuscontext) {
        const gchar *object_path = g_dbus_proxy_get_object_path (
                G_DBUS_PROXY (priv->ibuscontext));
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
        g_signal_emit (wlim,
                       wayland_im_signals[IBUS_FOCUS_OUT],
                       0,
                       object_path);
    }

    if (priv->preedit_text)
        g_clear_object (&priv->preedit_text);

    switch (priv->version) {
    case INPUT_METHOD_V1:
        if (priv->context) {
            g_clear_pointer (&priv->context,
                             zwp_input_method_context_v1_destroy);
        }
        break;
    case INPUT_METHOD_V2:
        if (priv->seat->keyboard_v2)
            zwp_input_method_keyboard_grab_v2_release (priv->seat->keyboard_v2);
        break;
    default:
        g_assert_not_reached ();
    }
}


static void
input_method_activate_v1 (void                               *data,
                          struct zwp_input_method_v1         *input_method_v1,
                          struct zwp_input_method_context_v1 *context)
{
    struct zwp_input_method_union input_method;
    input_method.u.input_method_v1 = input_method_v1;
    input_method_activate (data, &input_method, context);
}


static void
input_method_deactivate_v1 (void                               *data,
                            struct zwp_input_method_v1         *input_method_v1,
                            struct zwp_input_method_context_v1 *context)
{
    struct zwp_input_method_union input_method;
    input_method.u.input_method_v1 = input_method_v1;
    input_method_deactivate (data, &input_method, context);
}


static void
input_method_activate_v2 (void                       *data,
                          struct zwp_input_method_v2 *input_method_v2)
{
    IBusWaylandIM *wlim = data;
    IBusWaylandIMPrivate *priv;

    g_return_if_fail (IBUS_IS_WAYLAND_IM (wlim));
    priv = ibus_wayland_im_get_instance_private (wlim);
    priv->seat->pending_activate = TRUE;
}


static void
input_method_deactivate_v2 (void                       *data,
                            struct zwp_input_method_v2 *input_method_v2)
{
    IBusWaylandIM *wlim = data;
    IBusWaylandIMPrivate *priv;

    g_return_if_fail (IBUS_IS_WAYLAND_IM (wlim));
    priv = ibus_wayland_im_get_instance_private (wlim);
    priv->seat->pending_deactivate = TRUE;
}


static void
input_method_surrounding_text_v2 (void                       *data,
                                  struct zwp_input_method_v2 *input_method_v2,
                                  const char                 *text,
                                  uint32_t                    cursor,
                                  uint32_t                    anchor)
{
    struct zwp_input_method_context_union context;
    context.u.input_method_v2 = input_method_v2;
    handle_surrounding_text (data, &context, text, cursor, anchor);
}


static void
input_method_text_change_cause_v2 (void                       *data,
                                   struct zwp_input_method_v2 *input_method_v2,
                                   uint32_t                    cause)
{
}


static void
input_method_content_type_v2 (void                       *data,
                              struct zwp_input_method_v2 *input_method_v2,
                              uint32_t                    hint,
                              uint32_t                    purpose)
{
}


static void
input_method_done_v2 (void                       *data,
                      struct zwp_input_method_v2 *input_method_v2)
{
    IBusWaylandIM *wlim = data;
    IBusWaylandIMPrivate *priv;
    struct zwp_input_method_union input_method;

    g_return_if_fail (IBUS_IS_WAYLAND_IM (wlim));
    priv = ibus_wayland_im_get_instance_private (wlim);
    priv->serial++;
    input_method.u.input_method_v2 = input_method_v2;

    if (priv->seat->pending_activate && !priv->seat->active) {
        priv->seat->active = TRUE;
        input_method_activate (data, &input_method, NULL);
    } else if (priv->seat->pending_deactivate && priv->seat->active) {
        priv->seat->active = FALSE;
        input_method_deactivate (data, &input_method, NULL);
    }
    priv->seat->pending_activate = FALSE;
    priv->seat->pending_deactivate = FALSE;
}


static void
input_method_unavailable_v2 (void                       *data,
                             struct zwp_input_method_v2 *input_method_v2)
{
}


static const struct zwp_input_method_v1_listener input_method_listener_v1 = {
    .activate = input_method_activate_v1,
    .deactivate = input_method_deactivate_v1
};


static const struct zwp_input_method_v2_listener input_method_listener_v2 = {
    .activate = input_method_activate_v2,
    .deactivate = input_method_deactivate_v2,
    .surrounding_text= input_method_surrounding_text_v2,
    .text_change_cause = input_method_text_change_cause_v2,
    .content_type = input_method_content_type_v2,
    .done = input_method_done_v2,
    .unavailable = input_method_unavailable_v2
};


static void
seat_handle_capabilities(void *data, struct wl_seat *wl_seat,
                         enum wl_seat_capability caps)
{
}

static void
seat_handle_name(void *data, struct wl_seat *wl_seat, const char *name)
{
    IBusWaylandSeat *seat = data;
    g_assert (seat);
    g_assert (name);

    g_free (seat->name);
    seat->name = g_strdup(name);
}


static const struct wl_seat_listener seat_listener = {
    .capabilities = seat_handle_capabilities,
    .name = seat_handle_name,
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
        fprintf (priv->log,
                 "wl_registry gets interface: %s name: %u version: %u\n",
                 interface, name, version);
        fflush (priv->log);
    }
    priv->serial = 0;
    if (!g_strcmp0 (interface, zwp_input_method_manager_v2_interface.name)) {
        priv->version = INPUT_METHOD_V2;
        priv->input_method_manager_v2 =
                wl_registry_bind (registry, name,
                                  &zwp_input_method_manager_v2_interface, 1);
    } else if (!g_strcmp0 (interface, zwp_input_method_v1_interface.name)) {
        if (version >= 4)
            version = 4;
        priv->version = INPUT_METHOD_V1;
        priv->input_method_v1 =
                wl_registry_bind (registry, name,
                                  &zwp_input_method_v1_interface, version);
        zwp_input_method_v1_add_listener (priv->input_method_v1,
                                          &input_method_listener_v1, wlim);
    } else if (!g_strcmp0 (interface, zwp_input_panel_v1_interface.name)) {
        priv->panel = wl_registry_bind (registry, name,
                                        &zwp_input_panel_v1_interface, 1);
    } else if (!g_strcmp0 (interface, wl_seat_interface.name)) {
        IBusWaylandSeat *seat = g_slice_new0 (IBusWaylandSeat);
        if (version >= 5)
            version = 5;
        seat->seat = wl_registry_bind (registry, name,
                                       &wl_seat_interface, version);
        seat->wl_name = name;
        wl_seat_add_listener (seat->seat, &seat_listener, seat);
        g_ptr_array_add (priv->seats, seat);
        /* Assume the constructor of IBusWaylandIM is done and the second
         * seat is called here and don't have to wait for
         * wl_display_roundtrip().
         */
        if (priv->seat) {
            seat->input_method_v2 =
                    zwp_input_method_manager_v2_get_input_method (
                            priv->input_method_manager_v2,
                            seat->seat);
            zwp_input_method_v2_add_listener (seat->input_method_v2,
                                              &input_method_listener_v2, wlim);
            seat->virtual_keyboard =
                    zwp_virtual_keyboard_manager_v1_create_virtual_keyboard (
                            _virtual_keyboard_manager,
                            seat->seat);
        }
        priv->seat = seat;
    } else if (!g_strcmp0 (interface,
                           zwp_virtual_keyboard_manager_v1_interface.name)) {
        _virtual_keyboard_manager =
                wl_registry_bind (registry, name,
                                  &zwp_virtual_keyboard_manager_v1_interface,
                                  1);
    }
}


static void
registry_handle_global_remove (void               *data,
                               struct wl_registry *registry,
                               uint32_t            name)
{
    IBusWaylandIM *wlim = data;
    IBusWaylandIMPrivate *priv;
    IBusWaylandSeat *seat;
    guint i = 0;

    g_return_if_fail (IBUS_IS_WAYLAND_IM (wlim));
    priv = ibus_wayland_im_get_instance_private (wlim);
    if (priv->verbose) {
        fprintf (priv->log, "wl_registry remove name: %u\n", name);
        fflush (priv->log);
    }
    if (!(seat = _get_seat_with_name (priv->seats, name, &i)))
        return;
    if (priv->verbose) {
        fprintf (priv->log, "Remove %s seat.\n", seat->name);
        fflush (priv->log);
    }
    g_ptr_array_remove_index (priv->seats, i);
    if (priv->seat == seat) {
        if (!priv->seats->len)
            priv->seat = NULL;
        else
            priv->seat = g_ptr_array_index (priv->seats, 0);
    }
}


static const struct wl_registry_listener registry_listener = {
    .global = registry_handle_global,
    .global_remove = registry_handle_global_remove
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

    /* install signals */
    /* this module can call ibus_input_context_focus_in() and the focus-in
     * signal can reach the IBus panel and this also can call
     * ibus_input_context_focus_out() but the focus-out signal does not reach
     * the panel in case X11 client gets the focus because ibus-x11 calls
     * ibus_input_context_focus_in() before this calls
     * ibus_input_context_focus_out() and ibus-dameon ignores the double
     * focus-out signal since the focus-out signal already happened with the
     * focus-in signal. SO IBUS_FOCUS_OUT signal is needed at least to
     * send the signal to the panel certainly.
     */
    wayland_im_signals[IBUS_FOCUS_IN] =
        g_signal_new (I_("ibus-focus-in"),
            G_TYPE_FROM_CLASS (class),
            G_SIGNAL_RUN_LAST,
            0,
            NULL, NULL,
            g_cclosure_marshal_VOID__STRING,
            G_TYPE_NONE,
            1,
            G_TYPE_STRING);
    g_signal_set_va_marshaller (wayland_im_signals[IBUS_FOCUS_IN],
                                G_TYPE_FROM_CLASS (class),
                                g_cclosure_marshal_VOID__STRINGv);

    wayland_im_signals[IBUS_FOCUS_OUT] =
        g_signal_new (I_("ibus-focus-out"),
            G_TYPE_FROM_CLASS (class),
            G_SIGNAL_RUN_LAST,
            0,
            NULL, NULL,
            g_cclosure_marshal_VOID__STRING,
            G_TYPE_NONE,
            1,
            G_TYPE_STRING);
    g_signal_set_va_marshaller (wayland_im_signals[IBUS_FOCUS_OUT],
                                G_TYPE_FROM_CLASS (class),
                                g_cclosure_marshal_VOID__STRINGv);
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
    IBusWaylandSeat *seat = NULL;
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
    priv->seats = g_ptr_array_new_with_free_func (ibus_wayland_seat_destroy);
    priv->repeat_rate = 25;
    priv->repeat_delay = 600;

    _registry = wl_display_get_registry (priv->display);
    wl_registry_add_listener (_registry, &registry_listener, wlim);
    wl_display_roundtrip (priv->display);
    if (priv->input_method_manager_v2 && priv->seat &&
        _virtual_keyboard_manager) {
        seat = priv->seat;
        seat->input_method_v2 = zwp_input_method_manager_v2_get_input_method (
                priv->input_method_manager_v2,
                seat->seat);
        zwp_input_method_v2_add_listener (seat->input_method_v2,
                                          &input_method_listener_v2, wlim);
        seat->virtual_keyboard =
                zwp_virtual_keyboard_manager_v1_create_virtual_keyboard (
                        _virtual_keyboard_manager,
                        seat->seat);
    }
    if ((!seat || !seat->input_method_v2) && !priv->input_method_v1) {
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

    _use_sync_mode = _get_char_env ("IBUS_ENABLE_SYNC_MODE", 1);

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
    if (priv->panel)
        g_clear_pointer (&priv->panel, zwp_input_panel_v1_destroy);
    if (priv->input_method_v1)
        g_clear_pointer (&priv->input_method_v1, zwp_input_method_v1_destroy);
    if (priv->seats) {
        g_ptr_array_free (priv->seats, TRUE);
        priv->seats = NULL;
    }
    if (priv->input_method_manager_v2) {
        g_clear_pointer (&priv->input_method_manager_v2,
                         zwp_input_method_manager_v2_destroy);
    }


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
    priv = ibus_wayland_im_get_instance_private (wlim);

    switch (priv->version) {
    case INPUT_METHOD_V1:
        if (!_surface)
            return TRUE;
        if (!priv->panel) {
            g_warning ("Need zwp_input_panel_v1 before the surface setting.");
            return FALSE;
        }
        priv->panel_surface =
                zwp_input_panel_v1_get_input_panel_surface (priv->panel,
                                                            _surface);
        g_return_val_if_fail (priv->panel_surface, FALSE);
        zwp_input_panel_surface_v1_set_overlay_panel (priv->panel_surface);
        break;
    case INPUT_METHOD_V2:
        if (!priv->seat || !priv->seat->input_method_v2) {
            g_warning ("Need zwp_input_method_v2 before the surface setting.");
            return FALSE;
        }
        if (priv->seat->input_popup_surface) {
            g_clear_pointer (&priv->seat->input_popup_surface,
                             zwp_input_popup_surface_v2_destroy);
        }
        if (!_surface)
            return TRUE;
        priv->seat->input_popup_surface =
                zwp_input_method_v2_get_input_popup_surface (
                        priv->seat->input_method_v2,
                        _surface);
        break;
    default:
        g_assert_not_reached ();
    }
    return TRUE;
}
