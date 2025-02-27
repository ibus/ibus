/* -*- mode: C; c-basic-offset: 4; indent-tabs-mode: nil; -*- */
/* vim:set et sts=4: */
/* ibus
 * Copyright (C) 2007-2015 Peng Huang <shawn.p.huang@gmail.com>
 * Copyright (C) 2015-2025 Takao Fujiwara <takao.fujiwara1@gmail.com>
 * Copyright (C) 2007-2015 Red Hat, Inc.
 *
 * main.c:
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
#define _GNU_SOURCE

#include "config.h"

#include <X11/Xproto.h>
#include <X11/Xlib.h>
#include <X11/keysym.h>
#include <X11/Xutil.h>
#ifdef HAVE_XFIXES
#include <X11/extensions/Xfixes.h>
#endif
#include <XimProto.h>
#include <IMdkit.h>
#include <Xi18n.h>
#include <stdio.h>
#include <gtk/gtk.h>
#include <gdk/gdk.h>
#include <gdk/gdkx.h>
#include <langinfo.h>
#include <locale.h>
#include <iconv.h>
#include <signal.h>
#include <stdlib.h>

#include <getopt.h>

#define ESC_SEQUENCE_ISO10646_1 "\033%G"
/* Wait for about 120 secs to return a key from async process-key-event. */
#define MAX_WAIT_KEY_TIME       120000

#define LOG(level, fmt_args...) \
    if (g_debug_level >= (level)) { \
        g_debug (fmt_args); \
    }

#include <ibus.h>
#include "gdk-private.h"
#include "locales.h"

struct _X11ICONN {
    GList        *clients;
};
typedef struct _X11ICONN    X11ICONN;

typedef struct _X11IC    X11IC;
struct _X11IC {
    IBusInputContext *context;
    Window           client_window;
    Window           focus_window;
    gint32           input_style;
    X11ICONN        *conn;
    gint             icid;
    gint             connect_id;
    gchar           *lang;
    gboolean         has_preedit_area;
    GdkRectangle     preedit_area;

    gchar           *preedit_string;
    IBusAttrList    *preedit_attrs;
    gint             preedit_cursor;
    gboolean         preedit_visible;
    gboolean         preedit_started;
    gint             onspot_preedit_length;
};

static void     _xim_set_cursor_location    (X11IC              *x11ic);
static void     _context_commit_text_cb     (IBusInputContext   *context,
                                             IBusText           *text,
                                             X11IC              *x11ic);
static void     _context_forward_key_event_cb
                                            (IBusInputContext   *context,
                                             guint               keyval,
                                             guint               keycode,
                                             guint               state,
                                             X11IC              *x11ic);

static void     _context_update_preedit_text_cb
                                            (IBusInputContext   *context,
                                             IBusText           *text,
                                             gint                cursor_pos,
                                             gboolean            visible,
                                             X11IC              *x11ic);
static void     _context_show_preedit_text_cb
                                            (IBusInputContext   *context,
                                             X11IC              *x11ic);
static void     _context_hide_preedit_text_cb
                                            (IBusInputContext   *context,
                                             X11IC              *x11ic);
static void     _context_enabled_cb         (IBusInputContext   *context,
                                             X11IC              *x11ic);
static void     _context_disabled_cb        (IBusInputContext   *context,
                                             X11IC              *x11ic);

static GHashTable     *_x11_ic_table = NULL;
static GHashTable     *_connections = NULL;
static XIMS _xims = NULL;
static gchar *_server_name = NULL;
static gchar *_locale = NULL;

static gboolean _kill_daemon = FALSE;
static gint     g_debug_level = 0;

static IBusBus *_bus = NULL;

static char _use_sync_mode = 1;

static void
_xim_preedit_start (XIMS xims, const X11IC *x11ic)
{
    IMPreeditStateStruct ips;
    ips.major_code = 0;
    ips.minor_code = 0;
    ips.icid = x11ic->icid;
    ips.connect_id = x11ic->connect_id;
    IMPreeditStart (xims, (XPointer)&ips);
}

static void
_xim_preedit_end (XIMS xims, const X11IC *x11ic)
{
    IMPreeditStateStruct ips;
    ips.major_code = 0;
    ips.minor_code = 0;
    ips.icid = x11ic->icid;
    ips.connect_id = x11ic->connect_id;
    IMPreeditEnd (xims, (XPointer)&ips);
}


static void
_xim_preedit_callback_start (XIMS xims, const X11IC *x11ic)
{
    IMPreeditCBStruct pcb;

    pcb.major_code        = XIM_PREEDIT_START;
    pcb.minor_code        = 0;
    pcb.connect_id        = x11ic->connect_id;
    pcb.icid              = x11ic->icid;
    pcb.todo.return_value = 0;
    IMCallCallback (xims, (XPointer) & pcb);
}


static void
_xim_preedit_callback_done (XIMS xims, const X11IC *x11ic)
{
    IMPreeditCBStruct pcb;

    pcb.major_code        = XIM_PREEDIT_DONE;
    pcb.minor_code        = 0;
    pcb.connect_id        = x11ic->connect_id;
    pcb.icid              = x11ic->icid;
    pcb.todo.return_value = 0;
    IMCallCallback (xims, (XPointer) & pcb);
}


static void
_xim_preedit_callback_draw (XIMS xims, X11IC *x11ic, const gchar *preedit_string, IBusAttrList *attr_list)
{
    IMPreeditCBStruct pcb;
    XIMText text;
    XTextProperty tp;

    static XIMFeedback *feedback;
    static gint feedback_len = 0;
    guint j, i, len;

    if (preedit_string == NULL)
        return;

    len = g_utf8_strlen (preedit_string, -1);

    if (len + 1 > feedback_len) {
        feedback_len = (len + 1 + 63) & ~63;
        if (feedback) {
            feedback = g_renew (XIMFeedback, feedback, feedback_len);
        }
        else {
            feedback = g_new (XIMFeedback, feedback_len);
        }
    }

    for (i = 0; feedback && i < len; i++) {
        feedback[i] = 0;
    }

    if (attr_list != NULL) {
        for (i = 0;; i++) {
            XIMFeedback attr = 0;
            IBusAttribute *ibus_attr = ibus_attr_list_get (attr_list, i);
            if (ibus_attr == NULL) {
                break;
            }
            switch (ibus_attr->type) {
            case IBUS_ATTR_TYPE_UNDERLINE:
                if (ibus_attr->value == IBUS_ATTR_UNDERLINE_SINGLE) {
                    attr = XIMUnderline;
                }
                break;
            case IBUS_ATTR_TYPE_BACKGROUND:
                {
                    if (ibus_attr->value != 0xffffff) {
                        attr = XIMReverse;
                    }
                    break;
                }
            default:
                continue;
            }
            for (j = ibus_attr->start_index; j < ibus_attr->end_index; j++) {
                feedback[j] |= attr;
            }
        }
    }

    for (i = 0; feedback && i < len; i++) {
        if (feedback[i] == 0) {
            feedback[i] = XIMUnderline;
        }
    }
    if (feedback)
        feedback[len] = 0;

    pcb.major_code = XIM_PREEDIT_DRAW;
    pcb.connect_id = x11ic->connect_id;
    pcb.icid = x11ic->icid;

    pcb.todo.draw.caret = len;
    pcb.todo.draw.chg_first = 0;
    pcb.todo.draw.chg_length = x11ic->onspot_preedit_length;
    pcb.todo.draw.text = &text;

    text.feedback = feedback;

    if (len > 0) {
        int ret = Xutf8TextListToTextProperty (
                GDK_DISPLAY_XDISPLAY (gdk_display_get_default ()),
                (char **)&preedit_string,
                1, XCompoundTextStyle, &tp);
        if (ret == EXIT_FAILURE) {
            XFree (tp.value);
            tp.value = (unsigned char *)g_strdup_printf (
                    "%s%s",
                    ESC_SEQUENCE_ISO10646_1,
                    preedit_string);
        }
        text.encoding_is_wchar = 0;
        text.length = strlen ((char*)tp.value);
        text.string.multi_byte = (char*)tp.value;
        IMCallCallback (xims, (XPointer) & pcb);
        XFree (tp.value);
    } else {
        text.encoding_is_wchar = 0;
        text.length = 0;
        text.string.multi_byte = "";
        IMCallCallback (xims, (XPointer) & pcb);
        len = 0;
    }
    x11ic->onspot_preedit_length = len;
}

static int
_xim_store_ic_values (X11IC *x11ic, IMChangeICStruct *call_data)
{
    XICAttribute *ic_attr = call_data->ic_attr;
    XICAttribute *pre_attr = call_data->preedit_attr;
    XICAttribute *sts_attr = call_data->status_attr;

    gint i;

    g_return_val_if_fail (x11ic != NULL, 0);
    for (i = 0; i < (int)call_data->ic_attr_num; ++i, ++ic_attr) {
        if (g_strcmp0 (XNInputStyle, ic_attr->name) == 0) {
            x11ic->input_style = *(gint32 *) ic_attr->value;
        }
        else if (g_strcmp0 (XNClientWindow, ic_attr->name) == 0) {
            x11ic->client_window =  (Window)(*(CARD32 *) call_data->ic_attr[i].value);
        }
        else if (g_strcmp0 (XNFocusWindow, ic_attr->name) == 0) {
            x11ic->focus_window =  (Window)(*(CARD32 *) call_data->ic_attr[i].value);
        }
        else {
            LOG (1, "Unknown ic attribute: %s", ic_attr->name);
        }
    }

    for (i = 0; i < (int)call_data->preedit_attr_num; ++i, ++pre_attr) {
        if (g_strcmp0 (XNSpotLocation, pre_attr->name) == 0) {
            x11ic->has_preedit_area = TRUE;
            x11ic->preedit_area.x = ((XPoint *)pre_attr->value)->x;
            x11ic->preedit_area.y = ((XPoint *)pre_attr->value)->y;
        }
        else {
            LOG (1, "Unknown preedit attribute: %s", pre_attr->name);
        }
    }

    for (i=0; i< (int) call_data->status_attr_num; ++i, ++sts_attr) {
        LOG (1, "Unknown status attribute: %s", sts_attr->name);
    }

    return 1;
}


static int
xim_create_ic (XIMS xims, IMChangeICStruct *call_data)
{
    static int base_icid = 1;
    X11IC *x11ic;
    guint32 capabilities = IBUS_CAP_FOCUS;

    call_data->icid = base_icid ++;

    LOG (1, "XIM_CREATE_IC ic=%d connect_id=%d",
                call_data->icid, call_data->connect_id);

    x11ic = g_slice_new0 (X11IC);
    g_return_val_if_fail (x11ic != NULL, 0);

    x11ic->icid = call_data->icid;
    x11ic->connect_id = call_data->connect_id;
    x11ic->conn = (X11ICONN *)g_hash_table_lookup (_connections,
                                                   GINT_TO_POINTER ((gint) call_data->connect_id));
    if (x11ic->conn == NULL) {
        g_slice_free (X11IC, x11ic);
        g_return_val_if_reached (0);
    }

    _xim_store_ic_values (x11ic, call_data);

    x11ic->context = ibus_bus_create_input_context (_bus, "xim");

    if (x11ic->context == NULL) {
        g_slice_free (X11IC, x11ic);
        g_return_val_if_reached (0);
    }

    g_signal_connect (x11ic->context, "commit-text",
                        G_CALLBACK (_context_commit_text_cb), x11ic);
    g_signal_connect (x11ic->context, "forward-key-event",
                        G_CALLBACK (_context_forward_key_event_cb), x11ic);

    g_signal_connect (x11ic->context, "update-preedit-text",
                        G_CALLBACK (_context_update_preedit_text_cb), x11ic);
    g_signal_connect (x11ic->context, "show-preedit-text",
                        G_CALLBACK (_context_show_preedit_text_cb), x11ic);
    g_signal_connect (x11ic->context, "hide-preedit-text",
                        G_CALLBACK (_context_hide_preedit_text_cb), x11ic);
    g_signal_connect (x11ic->context, "enabled",
                        G_CALLBACK (_context_enabled_cb), x11ic);
    g_signal_connect (x11ic->context, "disabled",
                        G_CALLBACK (_context_disabled_cb), x11ic);


    if (x11ic->input_style & XIMPreeditCallbacks)
        capabilities |= IBUS_CAP_PREEDIT_TEXT;
    ibus_input_context_set_capabilities (x11ic->context, capabilities);
    if (_use_sync_mode == 1)
        ibus_input_context_set_post_process_key_event (x11ic->context, TRUE);

    g_hash_table_insert (_x11_ic_table,
                         GINT_TO_POINTER (x11ic->icid), (gpointer)x11ic);
    x11ic->conn->clients = g_list_append (x11ic->conn->clients,
                         (gpointer)x11ic);
    return 1;
}


static int
xim_destroy_ic (XIMS xims, IMChangeICStruct *call_data)
{
    X11IC *x11ic;

    LOG (1, "XIM_DESTROY_IC ic=%d connect_id=%d",
                call_data->icid, call_data->connect_id);

    x11ic = (X11IC *)g_hash_table_lookup (_x11_ic_table,
                                          GINT_TO_POINTER ((gint) call_data->icid));
    g_return_val_if_fail (x11ic != NULL, 0);

    if (x11ic->context) {
        ibus_proxy_destroy ((IBusProxy *)x11ic->context);
        g_object_unref (x11ic->context);
        x11ic->context = NULL;
    }

    g_hash_table_remove (_x11_ic_table,
                         GINT_TO_POINTER ((gint) call_data->icid));
    x11ic->conn->clients = g_list_remove (x11ic->conn->clients, (gconstpointer)x11ic);

    g_free (x11ic->preedit_string);
    x11ic->preedit_string = NULL;

    if (x11ic->preedit_attrs) {
        g_object_unref (x11ic->preedit_attrs);
        x11ic->preedit_attrs = NULL;
    }

    g_slice_free (X11IC, x11ic);

    return 1;
}


static int
xim_set_ic_focus (XIMS xims, IMChangeFocusStruct *call_data)
{
    X11IC *x11ic;

    LOG (1, "XIM_SET_IC_FOCUS ic=%d connect_id=%d",
                call_data->icid, call_data->connect_id);

    x11ic = (X11IC *) g_hash_table_lookup (_x11_ic_table,
                                           GINT_TO_POINTER ((gint) call_data->icid));
    g_return_val_if_fail (x11ic != NULL, 0);

    ibus_input_context_focus_in (x11ic->context);
    _xim_set_cursor_location (x11ic);

    return 1;
}


static int
xim_unset_ic_focus (XIMS xims, IMChangeFocusStruct *call_data)
{
    X11IC *x11ic;

    LOG (1, "XIM_UNSET_IC_FOCUS ic=%d connect_id=%d",
                call_data->icid, call_data->connect_id);

    x11ic = (X11IC *) g_hash_table_lookup (_x11_ic_table,
                                           GINT_TO_POINTER ((gint) call_data->icid));
    g_return_val_if_fail (x11ic != NULL, 0);

    ibus_input_context_focus_out (x11ic->context);

    return 1;

}


static void
_xim_forward_key_event_done (X11IC   *x11ic,
                             XEvent  *event,
                             gboolean processed)
{
    IMForwardEventStruct fe;
    if (processed) {
        if (!x11ic->has_preedit_area) {
            _xim_set_cursor_location (x11ic);
        }
        return;
    }
    g_assert (x11ic);
    g_assert (event);

    memset (&fe, 0, sizeof (fe));
    fe.major_code = XIM_FORWARD_EVENT;
    fe.icid = x11ic->icid;
    fe.connect_id = x11ic->connect_id;
    fe.sync_bit = 0;
    fe.serial_number = 0L;
    fe.event = *event;
    IMForwardEvent (_xims, (XPointer) &fe);
}


typedef struct {
    int                   count;
    guint                 count_cb_id;
    gboolean              retval;
    X11IC                *x11ic;
    CARD16                connect_id;
    XEvent                event;
} ProcessKeyEventReplyData;


static void
_process_key_event_done (GObject      *object,
                         GAsyncResult *res,
                         gpointer      user_data)
{
    IBusInputContext *context = (IBusInputContext *)object;
    ProcessKeyEventReplyData *data = (ProcessKeyEventReplyData *)user_data;

    GError *error = NULL;
    gboolean retval = ibus_input_context_process_key_event_async_finish (
            context,
            res,
            &error);

    if (error != NULL) {
        g_warning ("Process Key Event failed: %s.", error->message);
        g_error_free (error);
    }

    if (g_hash_table_lookup (_connections,
                             GINT_TO_POINTER ((gint)data->connect_id))
        == NULL) {
        g_slice_free (ProcessKeyEventReplyData, data);
        return;
    }

    if (retval == FALSE)
        _xim_forward_key_event_done (data->x11ic, &data->event, retval);
    g_slice_free (ProcessKeyEventReplyData, data);
}


static void
_process_key_event_reply_done (GObject      *object,
                               GAsyncResult *res,
                               gpointer      user_data)
{
    IBusInputContext *context = (IBusInputContext *)object;
    ProcessKeyEventReplyData *data = (ProcessKeyEventReplyData *)user_data;
    GError *error = NULL;
    gboolean retval = ibus_input_context_process_key_event_async_finish (
            context,
            res,
            &error);
    if (error != NULL) {
        g_warning ("Process Key Event failed: %s.", error->message);
        g_error_free (error);
    }
    g_return_if_fail (data);
    data->retval = retval;
    if (g_hash_table_lookup (_connections,
                             GINT_TO_POINTER ((gint)data->connect_id))
        == NULL) {
        return;
    }
    /* _xim_forward_key_event_done() should be called in
     * _process_key_event_reply_done() because g_main_context_iteration()
     * can call another xim_forward_event() and xim_forward_event() can be
     * nested and the first _process_key_event_reply_done() is returned
     * at last with g_main_context_iteration() so
     * if _xim_forward_key_event_done() is called out of
     * _process_key_event_reply_done(), the key events order
     * can be swapped.
     */
    _xim_forward_key_event_done (data->x11ic, &data->event, retval);
    data->count = 0;
    g_source_remove (data->count_cb_id);
}


static gboolean
_process_key_event_count_cb (gpointer user_data)
{
    ProcessKeyEventReplyData *data = (ProcessKeyEventReplyData *)user_data;
    g_return_val_if_fail (data, G_SOURCE_REMOVE);
    if (!data->count)
        return G_SOURCE_REMOVE;
    if (data->count++ == MAX_WAIT_KEY_TIME) {
        g_warning ("Key event is not returned for %usecs.", MAX_WAIT_KEY_TIME);
        return G_SOURCE_REMOVE;
    }
    return G_SOURCE_CONTINUE;
}


static int
_process_key_event_sync (X11IC                *x11ic,
                         IMForwardEventStruct *call_data,
                         GdkEventKey          *event)
{
    gboolean retval;

    g_assert (x11ic);
    g_assert (call_data);
    g_assert (event);
    retval = ibus_input_context_process_key_event (
            x11ic->context,
            event->keyval,
            event->hardware_keycode - 8,
            event->state);
    ibus_input_context_post_process_key_event (x11ic->context);
    _xim_forward_key_event_done (x11ic, &call_data->event, retval);
    return 1;
}


static int
_process_key_event_async (X11IC                *x11ic,
                          IMForwardEventStruct *call_data,
                          GdkEventKey          *event)
{
    ProcessKeyEventReplyData *data;

    g_assert (x11ic);
    g_assert (call_data);
    g_assert (event);
    if (!(data = g_slice_new0 (ProcessKeyEventReplyData))) {
        g_warning ("Cannot allocate async data");
        return _process_key_event_sync (x11ic, call_data, event);
    }
    data->connect_id = call_data->connect_id;
    data->x11ic = x11ic;
    data->event = call_data->event;
    ibus_input_context_process_key_event_async (x11ic->context,
                                                event->keyval,
                                                event->hardware_keycode - 8,
                                                event->state,
                                                -1,
                                                NULL,
                                                _process_key_event_done,
                                                data);
    return 1;
}


static int
_process_key_event_hybrid_async (X11IC                *x11ic,
                                 IMForwardEventStruct *call_data,
                                 GdkEventKey          *event)
{
    GSource *source;
    ProcessKeyEventReplyData *data = NULL;
    gboolean bus_retval;

    g_assert (x11ic);
    g_assert (call_data);
    g_assert (event);
    source = g_timeout_source_new (1);
    if (source)
        data = g_slice_new0 (ProcessKeyEventReplyData);
    if (!data) {
        int xim_retval;
        g_warning ("Cannot wait for the reply of the process key event.");
        xim_retval = _process_key_event_sync (x11ic, call_data, event);
        if (source)
            g_source_destroy (source);
        return xim_retval;
    }
    data->count = 1;
    g_source_attach (source, NULL);
    g_source_unref (source);
    data->count_cb_id = g_source_get_id (source);
    data->connect_id = call_data->connect_id;
    data->x11ic = x11ic;
    data->event = call_data->event;
    ibus_input_context_process_key_event_async (x11ic->context,
                                                event->keyval,
                                                event->hardware_keycode - 8,
                                                event->state,
                                                -1,
                                                NULL,
                                                _process_key_event_reply_done,
                                                data);
    g_source_set_callback (source, _process_key_event_count_cb,
                           data, NULL);
    while (data->count > 0 && data->count < MAX_WAIT_KEY_TIME)
        g_main_context_iteration (NULL, TRUE);
    /* #2498 Checking source->ref_count might cause Nautilus hang up
     */
    bus_retval = data->retval;
    if (data->count == 0) {
        g_slice_free (ProcessKeyEventReplyData, data);
        return 1;
    }

    g_slice_free (ProcessKeyEventReplyData, data);
    if (g_hash_table_lookup (_connections,
                             GINT_TO_POINTER ((gint)call_data->connect_id))
        == NULL) {
        return 1;
    }
    _xim_forward_key_event_done (x11ic, &call_data->event, bus_retval);
    return 1;
}


static int
xim_forward_event (XIMS xims, IMForwardEventStruct *call_data)
{
    X11IC *x11ic;
    XKeyEvent *xevent;
    GdkEventKey event;

    LOG (1, "XIM_FORWARD_EVENT ic=%d connect_id=%d",
         call_data->icid, call_data->connect_id);

    x11ic = (X11IC *) g_hash_table_lookup (
            _x11_ic_table,
            GINT_TO_POINTER ((gint) call_data->icid));
    g_return_val_if_fail (x11ic != NULL, 0);

    xevent = (XKeyEvent*) &(call_data->event);

    translate_key_event (gdk_display_get_default (),
        (GdkEvent *)&event, (XEvent *)xevent);

    event.send_event = xevent->send_event;
    event.window = NULL;

    if (event.type == GDK_KEY_RELEASE) {
        event.state |= IBUS_RELEASE_MASK;
    }

    switch (_use_sync_mode) {
    case 1:
        return _process_key_event_sync (x11ic, call_data, &event);
    case 2:
        return _process_key_event_hybrid_async (x11ic, call_data, &event);
    default:
        return _process_key_event_async (x11ic, call_data, &event);
    }
    g_assert_not_reached ();
    return 0;
}


static int
xim_open (XIMS xims, IMOpenStruct *call_data)
{
    X11ICONN *conn;

    LOG (1, "XIM_OPEN connect_id=%d",
                call_data->connect_id);

    conn = (X11ICONN *) g_hash_table_lookup (_connections,
                                             GINT_TO_POINTER ((gint) call_data->connect_id));
    g_return_val_if_fail (conn == NULL, 0);

    conn = g_slice_new0 (X11ICONN);

    g_hash_table_insert (_connections,
        (gpointer)(unsigned long)call_data->connect_id,
        (gpointer) conn);

    return 1;
}

static void
_free_ic (gpointer data, gpointer user_data)
{
    X11IC *x11ic = (X11IC *) data;

    g_return_if_fail (x11ic != NULL);

    g_free (x11ic->preedit_string);
    x11ic->preedit_string = NULL;

    if (x11ic->preedit_attrs) {
        g_object_unref (x11ic->preedit_attrs);
        x11ic->preedit_attrs = NULL;
    }

    if (x11ic->context) {
        ibus_proxy_destroy ((IBusProxy *)x11ic->context);
        g_object_unref (x11ic->context);
        x11ic->context = NULL;
    }

    /* Remove the IC from g_client dictionary */
    g_hash_table_remove (_x11_ic_table,
                         GINT_TO_POINTER (x11ic->icid));

    g_slice_free (X11IC, x11ic);
}

static int
_free_x11_iconn_from_id (CARD16 connect_id)
{
    X11ICONN *conn;

    conn = (X11ICONN *) g_hash_table_lookup (_connections,
                                             GINT_TO_POINTER ((gint) connect_id));

    if (conn == NULL) {
        return 0;
    }

    g_list_free_full (conn->clients, (GDestroyNotify) _free_ic);

    g_hash_table_remove (_connections,
                         GINT_TO_POINTER ((gint) connect_id));

    g_slice_free (X11ICONN, conn);

    return 1;
}

static int
xim_close (XIMS xims, IMCloseStruct *call_data)
{
    CARD16 connect_id = call_data->connect_id;

    LOG (1, "XIM_CLOSE connect_id=%d", connect_id);

    return _free_x11_iconn_from_id (connect_id);
}

static int
xim_disconnect_ic (XIMS xims, IMDisConnectStruct *call_data)
{
    CARD16 connect_id = call_data->connect_id;

    LOG (1, "XIM_DISCONNECT connect_id=%d", connect_id);

    _free_x11_iconn_from_id (connect_id);

    /* I am not sure if this can return 1 because I have not experienced
     * that xim_disconnect_ic() is called. But I wish connect_id is
     * released from _connections to avoid SEGV. */
    return 0;
}


static void
_xim_set_cursor_location (X11IC *x11ic)
{
    g_return_if_fail (x11ic != NULL);

    GdkRectangle preedit_area = x11ic->preedit_area;

    Window w = x11ic->focus_window ?
        x11ic->focus_window :x11ic->client_window;

    if (w) {
        XWindowAttributes xwa;
        Window child;

        XGetWindowAttributes (GDK_DISPLAY_XDISPLAY (gdk_display_get_default ()), w, &xwa);
        if (preedit_area.x <= 0 && preedit_area.y <= 0) {
             XTranslateCoordinates (GDK_DISPLAY_XDISPLAY (gdk_display_get_default ()), w,
                xwa.root,
                0,
                xwa.height,
                &preedit_area.x,
                &preedit_area.y,
                &child);
        }
        else {
            XTranslateCoordinates (GDK_DISPLAY_XDISPLAY (gdk_display_get_default ()), w,
                xwa.root,
                preedit_area.x,
                preedit_area.y,
                &preedit_area.x,
                &preedit_area.y,
                &child);
        }
    }

    ibus_input_context_set_cursor_location (x11ic->context,
            preedit_area.x,
            preedit_area.y,
            preedit_area.width,
            preedit_area.height);
}


static int
xim_set_ic_values (XIMS xims, IMChangeICStruct *call_data)
{
    X11IC *x11ic;
    gint i;

    LOG (1, "XIM_SET_IC_VALUES ic=%d connect_id=%d",
                call_data->icid, call_data->connect_id);

    x11ic = (X11IC *) g_hash_table_lookup (_x11_ic_table,
                                           GINT_TO_POINTER ((gint) call_data->icid));
    g_return_val_if_fail (x11ic != NULL, 0);

    i = _xim_store_ic_values (x11ic, call_data);

    if (i) {
        _xim_set_cursor_location (x11ic);
    }

    return 1;
}

static int
xim_get_ic_values (XIMS xims, IMChangeICStruct *call_data)
{
    X11IC *x11ic;
    gint i;

    LOG (1, "XIM_GET_IC_VALUES ic=%d connect_id=%d",
                call_data->icid, call_data->connect_id);

    x11ic = (X11IC *) g_hash_table_lookup (_x11_ic_table,
                                           GINT_TO_POINTER ((gint) call_data->icid));
    g_return_val_if_fail (x11ic != NULL, 0);

    XICAttribute *ic_attr = call_data->ic_attr;

    for (i = 0; i < (int) call_data->ic_attr_num; ++i, ++ic_attr) {
        if (g_strcmp0 (XNFilterEvents, ic_attr->name) == 0) {
            /* ic_attr->value will be freed in server side and ignore
             * leak of malloc with -Wanalyzer-malloc-leak flags in gcc 11.0.1
             */
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wanalyzer-malloc-leak"
            ic_attr->value = (void *) malloc (sizeof (CARD32));
            if (ic_attr->value) {
                *(CARD32 *) ic_attr->value = KeyPressMask | KeyReleaseMask;
                ic_attr->value_length = sizeof (CARD32);
            } else {
                g_warning ("Failed to malloc");
                ic_attr->value_length = 0;
            }
#pragma GCC diagnostic pop
        }
    }

    return 1;
}



static int
xim_reset_ic (XIMS xims, IMResetICStruct *call_data)
{
    X11IC *x11ic;

    LOG (1, "XIM_RESET_IC ic=%d connect_id=%d",
                call_data->icid, call_data->connect_id);

    x11ic = (X11IC *) g_hash_table_lookup (_x11_ic_table,
                                           GINT_TO_POINTER ((gint) call_data->icid));
    g_return_val_if_fail (x11ic != NULL, 0);

    ibus_input_context_reset (x11ic->context);

    return 1;
}

int
ims_protocol_handler (XIMS xims, IMProtocol *call_data)
{
    g_return_val_if_fail (xims != NULL, 1);
    g_return_val_if_fail (call_data != NULL, 1);

    switch (call_data->major_code) {
    case XIM_OPEN:
        return xim_open (xims, (IMOpenStruct *)call_data);
    case XIM_CLOSE:
        return xim_close (xims, (IMCloseStruct *)call_data);
    case XIM_DISCONNECT:
        return xim_disconnect_ic (xims, (IMDisConnectStruct *)call_data);
    case XIM_CREATE_IC:
        return xim_create_ic (xims, (IMChangeICStruct *)call_data);
    case XIM_DESTROY_IC:
        return xim_destroy_ic (xims, (IMChangeICStruct *)call_data);
    case XIM_SET_IC_VALUES:
        return xim_set_ic_values (xims, (IMChangeICStruct *)call_data);
    case XIM_GET_IC_VALUES:
        return xim_get_ic_values (xims, (IMChangeICStruct *)call_data);
    case XIM_FORWARD_EVENT:
        return xim_forward_event (xims, (IMForwardEventStruct *)call_data);
    case XIM_SET_IC_FOCUS:
        return xim_set_ic_focus (xims, (IMChangeFocusStruct *)call_data);
    case XIM_UNSET_IC_FOCUS:
        return xim_unset_ic_focus (xims, (IMChangeFocusStruct *)call_data);
    case XIM_RESET_IC:
        return xim_reset_ic (xims, (IMResetICStruct *)call_data);
    case XIM_TRIGGER_NOTIFY:
        LOG (1, "XIM_TRIGGER_NOTIFY");
        return 0;
    case XIM_PREEDIT_START_REPLY:
        LOG (1, "XIM_PREEDIT_START_REPLY");
        return 0;
    case XIM_PREEDIT_CARET_REPLY:
        LOG (1, "XIM_PREEDIT_CARET_REPLY");
        return 0;
    case XIM_SYNC_REPLY:
        LOG (1, "XIM_SYNC_REPLY");
        return 0;
    default:
        LOG (1, "Unknown (%d)", call_data->major_code);
        return 0;
    }
}

static void
_xim_forward_key_event (X11IC   *x11ic,
                        guint    keyval,
                        guint    keycode,
                        guint    state)
{
    XEvent xkp = {0};

    g_return_if_fail (x11ic != NULL);

    xkp.xkey.type = (state & IBUS_RELEASE_MASK) ? KeyRelease : KeyPress;
    xkp.xkey.serial = 0L;
    xkp.xkey.send_event = False;
    xkp.xkey.same_screen = True;
    xkp.xkey.display = GDK_DISPLAY_XDISPLAY (gdk_display_get_default ());
    xkp.xkey.window =
        x11ic->focus_window ? x11ic->focus_window : x11ic->client_window;
    xkp.xkey.subwindow = None;
    xkp.xkey.root = DefaultRootWindow (
            GDK_DISPLAY_XDISPLAY (gdk_display_get_default ()));

    xkp.xkey.time = 0;
    xkp.xkey.state = state;
    xkp.xkey.keycode = (keycode == 0) ? 0 : keycode + 8;

    _xim_forward_key_event_done (x11ic, &xkp, FALSE);
}

static void
_bus_disconnected_cb (IBusBus  *bus,
                      gpointer  user_data)
{
    g_debug ("Connection closed by ibus-daemon\n");
    g_clear_object (&_bus);
    ibus_quit ();
}

static void
_context_commit_text_cb (IBusInputContext *context,
                         IBusText         *text,
                         X11IC            *x11ic)
{
    g_assert (IBUS_IS_INPUT_CONTEXT (context));
    g_assert (IBUS_IS_TEXT (text));
    g_assert (x11ic != NULL);

    XTextProperty tp;
    IMCommitStruct cms = {0};
    int ret;

    ret = Xutf8TextListToTextProperty (
            GDK_DISPLAY_XDISPLAY (gdk_display_get_default ()),
            (gchar **)&(text->text), 1, XCompoundTextStyle, &tp);
    /* XCompoundTextStyle uses the encoding escaped sequence + encoded chars
     * matched to the specified multibyte characters: text->text, and
     * libX11.so sorts the encoding sets by locale.
     * If an encoded string fails to be matched, ibus-x11 specifies the
     * ISO10641-1 encoding and that escaped sequence is "\033%G":
     * https://gitlab.freedesktop.org/xorg/lib/libx11/-/blob/master/src/xlibi18n/lcCT.c
     * , and the encoding is UTF-8 with utf8_wctomb():
     * https://gitlab.freedesktop.org/xorg/lib/libx11/-/blob/master/src/xlibi18n/lcUniConv/utf8.h
     */
    if (ret == EXIT_FAILURE) {
        XFree (tp.value);
        tp.value = (unsigned char *)g_strdup_printf ("%s%s",
                                                     ESC_SEQUENCE_ISO10646_1,
                                                     text->text);
    }

    cms.major_code = XIM_COMMIT;
    cms.icid = x11ic->icid;
    cms.connect_id = x11ic->connect_id;
    cms.flag = XimLookupChars;
    cms.commit_string = (gchar *)tp.value;
    IMCommitString (_xims, (XPointer) & cms);

    XFree (tp.value);
}

static void
_context_forward_key_event_cb (IBusInputContext *context,
                               guint             keyval,
                               guint             keycode,
                               guint             state,
                               X11IC            *x11ic)
{
    g_assert (x11ic);

    _xim_forward_key_event (x11ic, keyval, keycode, state);
}

static void
_update_preedit (X11IC *x11ic)
{
    if (x11ic->preedit_visible == FALSE && x11ic->preedit_started == TRUE) {
        _xim_preedit_callback_draw (_xims, x11ic, "", NULL);
        _xim_preedit_callback_done (_xims, x11ic);
        x11ic->preedit_started = FALSE;
    }

    if (x11ic->preedit_visible == TRUE && x11ic->preedit_started == FALSE) {
        _xim_preedit_callback_start (_xims, x11ic);
        x11ic->preedit_started = TRUE;
    }
    if (x11ic->preedit_visible == TRUE) {
        _xim_preedit_callback_draw (_xims, x11ic,
            x11ic->preedit_string, x11ic->preedit_attrs);
    }
}

static void
_context_update_preedit_text_cb (IBusInputContext *context,
                                 IBusText         *text,
                                 gint              cursor_pos,
                                 gboolean          visible,
                                 X11IC            *x11ic)
{
    g_assert (IBUS_IS_INPUT_CONTEXT (context));
    g_assert (IBUS_IS_TEXT (text));
    g_assert (x11ic);

    if (x11ic->preedit_string) {
        g_free(x11ic->preedit_string);
    }
    x11ic->preedit_string = g_strdup(text->text);

    if (x11ic->preedit_attrs) {
        g_object_unref (x11ic->preedit_attrs);
    }

    g_object_ref(text->attrs);
    x11ic->preedit_attrs = text->attrs;

    x11ic->preedit_cursor = cursor_pos;
    x11ic->preedit_visible = visible;

    _update_preedit (x11ic);
}

static void
_context_show_preedit_text_cb (IBusInputContext *context,
                               X11IC            *x11ic)
{
    g_assert (IBUS_IS_INPUT_CONTEXT (context));
    g_assert (x11ic);

    x11ic->preedit_visible = TRUE;
    _update_preedit (x11ic);
}

static void
_context_hide_preedit_text_cb (IBusInputContext *context,
                               X11IC            *x11ic)
{
    g_assert (IBUS_IS_INPUT_CONTEXT (context));
    g_assert (x11ic);

    x11ic->preedit_visible = FALSE;
    _update_preedit (x11ic);
}

static void
_context_enabled_cb (IBusInputContext *context,
                    X11IC            *x11ic)
{
    g_assert (IBUS_IS_INPUT_CONTEXT (context));
    g_assert (x11ic);

    _xim_preedit_start (_xims, x11ic);
}

static void
_context_disabled_cb (IBusInputContext *context,
                    X11IC            *x11ic)
{
    g_assert (IBUS_IS_INPUT_CONTEXT (context));
    g_assert (x11ic);

    _xim_preedit_end (_xims, x11ic);
}

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

static void
_init_ibus (void)
{
    if (_bus != NULL)
        return;

    ibus_init ();

    _bus = ibus_bus_new ();

    g_signal_connect (_bus, "disconnected",
                        G_CALLBACK (_bus_disconnected_cb), NULL);

    /* https://github.com/ibus/ibus/issues/1713 */
    _use_sync_mode = _get_char_env ("IBUS_ENABLE_SYNC_MODE", 1);
}

static void
_xim_init_IMdkit ()
{
#if 0
    XIMStyle ims_styles_overspot [] = {
        XIMPreeditPosition  | XIMStatusNothing,
        XIMPreeditNothing   | XIMStatusNothing,
        XIMPreeditPosition  | XIMStatusCallbacks,
        XIMPreeditNothing   | XIMStatusCallbacks,
        0
    };
#endif

    XIMStyle ims_styles_onspot [] = {
        XIMPreeditPosition  | XIMStatusNothing,
        XIMPreeditCallbacks | XIMStatusNothing,
        XIMPreeditNothing   | XIMStatusNothing,
        XIMPreeditPosition  | XIMStatusCallbacks,
        XIMPreeditCallbacks | XIMStatusCallbacks,
        XIMPreeditNothing   | XIMStatusCallbacks,
        0
    };

    XIMEncoding ims_encodings[] = {
        "COMPOUND_TEXT",
        0
    };

    GdkWindowAttr window_attr = {
        .title              = "ibus-xim",
        .event_mask         = GDK_KEY_PRESS_MASK | GDK_KEY_RELEASE_MASK,
        .wclass             = GDK_INPUT_OUTPUT,
        .window_type        = GDK_WINDOW_TOPLEVEL,
        .override_redirect   = 1,
    };

    XIMStyles styles;
    XIMEncodings encodings;

    GdkWindow *win;

    win = gdk_window_new (NULL, &window_attr, GDK_WA_TITLE);

    styles.count_styles =
        sizeof (ims_styles_onspot)/sizeof (XIMStyle) - 1;
    styles.supported_styles = ims_styles_onspot;

    encodings.count_encodings =
        sizeof (ims_encodings)/sizeof (XIMEncoding) - 1;
    encodings.supported_encodings = ims_encodings;

    _xims = IMOpenIM (GDK_DISPLAY_XDISPLAY (gdk_display_get_default ()),
        IMModifiers, "Xi18n",
#if GTK_CHECK_VERSION (3, 0, 0)
        IMServerWindow, GDK_WINDOW_XID (win),
#else
        IMServerWindow, GDK_WINDOW_XWINDOW (win),
#endif
        IMServerName, _server_name != NULL ? _server_name : "ibus",
        IMLocale, _locale != NULL ? _locale : LOCALES_STRING,
        IMServerTransport, "X/",
        IMInputStyles, &styles,
        IMEncodingList, &encodings,
        IMProtocolHandler, ims_protocol_handler,
        IMFilterEventMask, KeyPressMask | KeyReleaseMask,
        NULL);

    _init_ibus ();

    if (!ibus_bus_is_connected (_bus)) {
        g_warning ("Can not connect to ibus daemon");
        exit (EXIT_FAILURE);
    }
}

static void
_atexit_cb ()
{
    if (_bus && ibus_bus_is_connected (_bus)) {
        ibus_bus_exit(_bus, False);
    }
}

static void
_sighandler (int sig)
{
    exit(EXIT_FAILURE);
}

static void
_print_usage (FILE *fp, gchar *name)
{
    fprintf (fp,
        "Usage:\n"
        " %s --help               Show this message\n"
        "    --server-name= -n    Setup xim server name\n"
        "    --locale= -l         Setup support locales\n"
        "    --locale-append= -a  Append locales into the default support locales\n"
        "    --kill-daemon -k     Kill ibus daemon when exit\n"
        "    --debug= -v          Setup debug level\n",
        name);
}

static int
_xerror_handler (Display *dpy, XErrorEvent *e)
{
    g_debug (
        "XError: "
        "serial=%lu error_code=%d request_code=%d minor_code=%d resourceid=%lu",
        e->serial, e->error_code, e->request_code, e->minor_code, e->resourceid);
    return 1;
}

/* When log into GNOME3 desktop immediately after the system is booted,
 * ibus-daemon is sometimes alive but ibus-x11 is dead after log out
 * the session. Because gdk_x_io_error() is called as the callback of
 * XSetIOErrorHandler() in gtk/gdk/x11/gdkmain-x11.c in ibus-x11.
 * Now I assume the callback is called in logout.
 */
static int
_xerror_io_handler (Display *dpy)
{
    if (_kill_daemon)
        _atexit_cb ();
    return 0;
}

int
main (int argc, char **argv)
{
    gint option_index = 0;
    gint c;

    /* GDK_DISPLAY_XDISPLAY() and GDK_WINDOW_XID() does not work
     * with GdkWaylandDisplay.
     */
#if GTK_CHECK_VERSION (3, 10, 0)
    gdk_set_allowed_backends ("x11");
#endif

    gdk_init (&argc, &argv);
    XSetErrorHandler (_xerror_handler);
    XSetIOErrorHandler (_xerror_io_handler);

#ifdef HAVE_XFIXES
    XFixesSetClientDisconnectMode(GDK_DISPLAY_XDISPLAY (gdk_display_get_default ()),
                                  XFixesClientDisconnectFlagTerminate);
#endif

    while (1) {
        static struct option long_options [] = {
            { "debug", 1, 0, 0},
            { "server-name", 1, 0, 0},
            { "locale", 1, 0, 0},
            { "locale-append", 1, 0, 0},
            { "help", 0, 0, 0},
            { "kill-daemon", 0, 0, 0},
            { 0, 0, 0, 0},
        };

        c = getopt_long (argc, argv, "v:n:l:k:a",
            long_options, &option_index);

        if (c == -1) break;

        switch (c) {
        case 0:
            if (g_strcmp0 (long_options[option_index].name, "debug") == 0) {
                g_debug_level = atoi (optarg);
            }
            else if (g_strcmp0 (long_options[option_index].name, "server-name") == 0) {
                g_free (_server_name);
                _server_name = g_strdup (optarg);
            }
            else if (g_strcmp0 (long_options[option_index].name, "locale") == 0) {
                g_free (_locale);
                _locale = g_strdup (optarg);
            }
            else if (g_strcmp0 (long_options[option_index].name, "locale-append") == 0) {
                gchar *tmp = g_strdup_printf ("%s,%s",
                                _locale != NULL ? _locale : LOCALES_STRING, optarg);
                g_free (_locale);
                _locale = tmp;
            }
            else if (g_strcmp0 (long_options[option_index].name, "help") == 0) {
                _print_usage (stdout, argv[0]);
                exit (EXIT_SUCCESS);
            }
            else if (g_strcmp0 (long_options[option_index].name, "kill-daemon") == 0) {
                _kill_daemon = TRUE;
            }
            break;
        case 'v':
            g_debug_level = atoi (optarg);
            break;
        case 'n':
            g_free (_server_name);
            _server_name = g_strdup (optarg);
            break;
        case 'l':
            g_free (_locale);
            _locale = g_strdup (optarg);
            break;
        case 'a': {
                gchar *tmp = g_strdup_printf ("%s,%s",
                                _locale != NULL ? _locale : LOCALES_STRING, optarg);
                g_free (_locale);
                _locale = tmp;
            }
            break;
        case 'k':
            _kill_daemon = TRUE;
            break;
        case '?':
        default:
            _print_usage (stderr, argv[0]);
            exit (EXIT_FAILURE);
        }
    }

    _x11_ic_table = g_hash_table_new (g_direct_hash, g_direct_equal);
    _connections = g_hash_table_new (g_direct_hash, g_direct_equal);

    signal (SIGINT, _sighandler);
    signal (SIGTERM, _sighandler);

    if (_kill_daemon)
        atexit (_atexit_cb);

    _xim_init_IMdkit ();
    ibus_main();

    exit (EXIT_SUCCESS);
}
