/* -*- mode: C; c-basic-offset: 4; indent-tabs-mode: nil; -*- */
/* vim:set et sts=4: */
/* ibus - The Input Bus
 * Copyright (C) 2008-2013 Peng Huang <shawn.p.huang@gmail.com>
 * Copyright (C) 2008-2013 Red Hat, Inc.
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

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <string.h>
#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>
#include <ibus.h>
#include "ibusimcontext.h"

#if !GTK_CHECK_VERSION (2, 91, 0)
#  define DEPRECATED_GDK_KEYSYMS 1
#endif

#ifdef DEBUG
#  define IDEBUG g_debug
#else
#  define IDEBUG(a...)
#endif

#define MAX_QUEUED_EVENTS 20

struct _IBusIMContext {
    GtkIMContext parent;

    /* instance members */
    GtkIMContext *slave;
    GdkWindow *client_window;

    IBusInputContext *ibuscontext;

    /* preedit status */
    gchar           *preedit_string;
    PangoAttrList   *preedit_attrs;
    gint             preedit_cursor_pos;
    gboolean         preedit_visible;

    GdkRectangle     cursor_area;
    gboolean         has_focus;

    guint32          time;
    gint             caps;

    /* cancellable */
    GCancellable    *cancellable;
    GQueue          *events_queue;
};

struct _IBusIMContextClass {
GtkIMContextClass parent;
    /* class members */
};

static guint    _signal_commit_id = 0;
static guint    _signal_preedit_changed_id = 0;
static guint    _signal_preedit_start_id = 0;
static guint    _signal_preedit_end_id = 0;
static guint    _signal_delete_surrounding_id = 0;
static guint    _signal_retrieve_surrounding_id = 0;

static const gchar *_no_snooper_apps = NO_SNOOPER_APPS;
static gboolean _use_key_snooper = ENABLE_SNOOPER;
static guint    _key_snooper_id = 0;

static gboolean _use_sync_mode = FALSE;

static GtkIMContext *_focus_im_context = NULL;
static IBusInputContext *_fake_context = NULL;
static GdkWindow *_input_window = NULL;
static GtkWidget *_input_widget = NULL;

static gboolean _commit_preedit_text_before_resetting_im = FALSE;

/* functions prototype */
static void     ibus_im_context_class_init  (IBusIMContextClass    *class);
static void     ibus_im_context_class_fini  (IBusIMContextClass    *class);
static void     ibus_im_context_init        (GObject               *obj);
static void     ibus_im_context_notify      (GObject               *obj,
                                             GParamSpec            *pspec);
static void     ibus_im_context_finalize    (GObject               *obj);
static void     ibus_im_context_reset       (GtkIMContext          *context);
static gboolean ibus_im_context_filter_keypress
                                            (GtkIMContext           *context,
                                             GdkEventKey            *key);
static void     ibus_im_context_focus_in    (GtkIMContext          *context);
static void     ibus_im_context_focus_out   (GtkIMContext          *context);
static void     ibus_im_context_get_preedit_string
                                            (GtkIMContext           *context,
                                             gchar                  **str,
                                             PangoAttrList          **attrs,
                                             gint                   *cursor_pos);
static void     ibus_im_context_set_client_window
                                            (GtkIMContext           *context,
                                             GdkWindow              *client);
static void     ibus_im_context_set_cursor_location
                                            (GtkIMContext           *context,
                                             GdkRectangle           *area);
static void     ibus_im_context_set_use_preedit
                                            (GtkIMContext           *context,
                                             gboolean               use_preedit);
static void     ibus_im_context_set_surrounding
                                            (GtkIMContext  *slave,
                                             const gchar   *text,
                                             gint           len,
                                             gint           cursor_index);


/* static methods*/
static void     _create_input_context       (IBusIMContext      *context);
static gboolean _set_cursor_location_internal
                                            (IBusIMContext      *context);

static void     _bus_connected_cb           (IBusBus            *bus,
                                             IBusIMContext      *context);
static void     _ibus_context_commit_text_cb
                                            (IBusInputContext *ibuscontext,
                                             IBusText         *text,
                                             IBusIMContext    *ibusimcontext);
static void     _ibus_context_update_preedit_text_cb
                                            (IBusInputContext *ibuscontext,
                                             IBusText         *text,
                                             gint              cursor_pos,
                                             gboolean          visible,
                                             IBusIMContext    *ibusimcontext);

/* callback functions for slave context */
static void     _slave_commit_cb            (GtkIMContext       *slave,
                                             gchar              *string,
                                             IBusIMContext       *context);
static void     _slave_preedit_changed_cb   (GtkIMContext       *slave,
                                             IBusIMContext       *context);
static void     _slave_preedit_start_cb     (GtkIMContext       *slave,
                                             IBusIMContext       *context);
static void     _slave_preedit_end_cb       (GtkIMContext       *slave,
                                             IBusIMContext       *context);
static gboolean _slave_retrieve_surrounding_cb
                                            (GtkIMContext       *slave,
                                             IBusIMContext      *context);
static gboolean _slave_delete_surrounding_cb
                                            (GtkIMContext       *slave,
                                             gint                offset_from_cursor,
                                             guint               nchars,
                                             IBusIMContext      *context);
static void     _request_surrounding_text   (IBusIMContext      *context);
static void     _create_fake_input_context  (void);
static void     _set_content_type           (IBusIMContext      *context);



static GType                _ibus_type_im_context = 0;
static GtkIMContextClass    *parent_class = NULL;

static IBusBus              *_bus = NULL;
static guint                _daemon_name_watch_id = 0;
static gboolean             _daemon_is_running = FALSE;

void
ibus_im_context_register_type (GTypeModule *type_module)
{
    IDEBUG ("%s", __FUNCTION__);

    static const GTypeInfo ibus_im_context_info = {
        sizeof (IBusIMContextClass),
        (GBaseInitFunc)      NULL,
        (GBaseFinalizeFunc)  NULL,
        (GClassInitFunc)     ibus_im_context_class_init,
        (GClassFinalizeFunc) ibus_im_context_class_fini,
        NULL,            /* class data */
        sizeof (IBusIMContext),
        0,
        (GInstanceInitFunc)    ibus_im_context_init,
    };

    if (!_ibus_type_im_context) {
        if (type_module) {
            _ibus_type_im_context =
                g_type_module_register_type (type_module,
                    GTK_TYPE_IM_CONTEXT,
                    "IBusIMContext",
                    &ibus_im_context_info,
                    (GTypeFlags)0);
        }
        else {
            _ibus_type_im_context =
                g_type_register_static (GTK_TYPE_IM_CONTEXT,
                    "IBusIMContext",
                    &ibus_im_context_info,
                    (GTypeFlags)0);
        }
    }
}

GType
ibus_im_context_get_type (void)
{
    IDEBUG ("%s", __FUNCTION__);

    if (_ibus_type_im_context == 0) {
        ibus_im_context_register_type (NULL);
    }

    g_assert (_ibus_type_im_context != 0);
    return _ibus_type_im_context;
}

IBusIMContext *
ibus_im_context_new (void)
{
    IDEBUG ("%s", __FUNCTION__);

    GObject *obj = g_object_new (IBUS_TYPE_IM_CONTEXT, NULL);
    return IBUS_IM_CONTEXT (obj);
}

static gboolean
_focus_in_cb (GtkWidget     *widget,
              GdkEventFocus *event,
              gpointer       user_data)
{
    if (_focus_im_context == NULL && _fake_context != NULL) {
        ibus_input_context_focus_in (_fake_context);
    }
    return FALSE;
}

static gboolean
_focus_out_cb (GtkWidget     *widget,
               GdkEventFocus *event,
               gpointer       user_data)
{
    if (_focus_im_context == NULL && _fake_context != NULL) {
        ibus_input_context_focus_out (_fake_context);
    }
    return FALSE;
}

static void
_process_key_event_done (GObject      *object,
                         GAsyncResult *res,
                         gpointer      user_data)
{
    IBusInputContext *context = (IBusInputContext *)object;
    GdkEventKey *event = (GdkEventKey *) user_data;

    GError *error = NULL;
    gboolean retval = ibus_input_context_process_key_event_async_finish (
            context,
            res,
            &error);

    if (error != NULL) {
        g_warning ("Process Key Event failed: %s.", error->message);
        g_error_free (error);
    }

    if (retval == FALSE) {
        event->state |= IBUS_IGNORED_MASK;
        gdk_event_put ((GdkEvent *)event);
    }
    gdk_event_free ((GdkEvent *)event);
}

static gboolean
_process_key_event (IBusInputContext *context,
                    GdkEventKey      *event)
{
    guint state = event->state;
    gboolean retval = FALSE;

    if (event->type == GDK_KEY_RELEASE) {
        state |= IBUS_RELEASE_MASK;
    }

    if (_use_sync_mode) {
        retval = ibus_input_context_process_key_event (context,
            event->keyval,
            event->hardware_keycode - 8,
            state);
    }
    else {
        ibus_input_context_process_key_event_async (context,
            event->keyval,
            event->hardware_keycode - 8,
            state,
            -1,
            NULL,
            _process_key_event_done,
            gdk_event_copy ((GdkEvent *) event));

        retval = TRUE;
    }

    if (retval) {
        event->state |= IBUS_HANDLED_MASK;
    }
    else {
        event->state |= IBUS_IGNORED_MASK;
    }

    return retval;
}


/* emit "retrieve-surrounding" glib signal of GtkIMContext, if
 * context->caps has IBUS_CAP_SURROUNDING_TEXT and the current IBus
 * engine needs surrounding-text.
 */
static void
_request_surrounding_text (IBusIMContext *context)
{
    if (context &&
        (context->caps & IBUS_CAP_SURROUNDING_TEXT) != 0 &&
        context->ibuscontext != NULL &&
        ibus_input_context_needs_surrounding_text (context->ibuscontext)) {
        gboolean return_value;
        IDEBUG ("requesting surrounding text");
        g_signal_emit (context, _signal_retrieve_surrounding_id, 0,
                       &return_value);
        if (!return_value) {
            context->caps &= ~IBUS_CAP_SURROUNDING_TEXT;
            ibus_input_context_set_capabilities (context->ibuscontext,
                                                 context->caps);
        }
    }
}

static void
_set_content_type (IBusIMContext *context)
{
#if GTK_CHECK_VERSION (3, 6, 0)
    if (context->ibuscontext != NULL) {
        GtkInputPurpose purpose;
        GtkInputHints hints;

        g_object_get (G_OBJECT (context),
                      "input-purpose", &purpose,
                      "input-hints", &hints,
                      NULL);

        ibus_input_context_set_content_type (context->ibuscontext,
                                             purpose,
                                             hints);
    }
#endif
}


static gint
_key_snooper_cb (GtkWidget   *widget,
                 GdkEventKey *event,
                 gpointer     user_data)
{
    IDEBUG ("%s", __FUNCTION__);
    gboolean retval = FALSE;

    IBusIMContext *ibusimcontext = NULL;
    IBusInputContext *ibuscontext = NULL;

    if (!_use_key_snooper)
        return FALSE;

    if (_focus_im_context != NULL &&
        ((IBusIMContext *) _focus_im_context)->has_focus == TRUE) {
        ibusimcontext = (IBusIMContext *) _focus_im_context;
        /* has IC with focus */
        ibuscontext = ibusimcontext->ibuscontext;
    }
    else {
        /* If no IC has focus, and fake IC has been created, then pass key events to fake IC. */
        ibuscontext = _fake_context;
    }

    if (ibuscontext == NULL)
        return FALSE;

    if (G_UNLIKELY (event->state & IBUS_HANDLED_MASK))
        return TRUE;

    if (G_UNLIKELY (event->state & IBUS_IGNORED_MASK))
        return FALSE;

    do {
        if (_fake_context != ibuscontext)
            break;

        /* window has input focus is not changed */
        if (_input_window == event->window)
            break;

        if (_input_window != NULL) {
            g_object_remove_weak_pointer ((GObject *) _input_window,
                                          (gpointer *) &_input_window);
        }
        if (event->window != NULL) {
            g_object_add_weak_pointer ((GObject *) event->window,
                                       (gpointer *) &_input_window);
        }
        _input_window = event->window;

        /* Trace widget has input focus, and listen focus events of it.
         * It is workaround for Alt+Shift+Tab shortcut key issue(crosbug.com/8855).
         * gtk_get_event_widget returns the widget that is associated with the
         * GdkWindow of the GdkEvent.
         * */
        GtkWidget *widget = gtk_get_event_widget ((GdkEvent *)event);
        /* g_assert (_input_widget != widget). */
        if (_input_widget == widget)
            break;

        if (_input_widget != NULL) {
            g_signal_handlers_disconnect_by_func (_input_widget,
                                                  (GCallback) _focus_in_cb,
                                                  NULL);
            g_signal_handlers_disconnect_by_func (_input_widget,
                                                  (GCallback) _focus_out_cb,
                                                  NULL);
            g_object_remove_weak_pointer ((GObject *) _input_widget,
                                          (gpointer *) &_input_widget);
        }

        if (widget != NULL) {
            g_signal_connect (widget,
                              "focus-in-event",
                              (GCallback) _focus_in_cb,
                              NULL);
            g_signal_connect (widget,
                              "focus-out-event",
                              (GCallback) _focus_out_cb,
                              NULL);
            g_object_add_weak_pointer ((GObject *) widget,
                                       (gpointer *) &_input_widget);
        }
        _input_widget = widget;

    } while (0);

    if (ibusimcontext != NULL) {
        /* "retrieve-surrounding" signal sometimes calls unref by
         * gtk_im_multicontext_get_slave() because priv->context_id is not
         * the latest than global_context_id in GtkIMMulticontext.
         * Since _focus_im_context is gotten by the focus_in event,
         * it would be good to call ref here.
         */
        g_object_ref (ibusimcontext);
        _request_surrounding_text (ibusimcontext);
        ibusimcontext->time = event->time;
    }

    retval = _process_key_event (ibuscontext, event);

    if (ibusimcontext != NULL) {
        /* unref ibusimcontext could call ibus_im_context_finalize here
         * because "retrieve-surrounding" signal could call unref.
         */
        g_object_unref (ibusimcontext);
    }

    return retval;
}

static gboolean
_get_boolean_env(const gchar *name,
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

static void
daemon_name_appeared (GDBusConnection *connection,
                      const gchar     *name,
                      const gchar     *owner,
                      gpointer         data)
{
    /* If ibus-daemon is running and run ssh -X localhost,
     * daemon_name_appeared() is called but ibus_get_address() == NULL
     * because the hostname and display number are different between
     * ibus-daemon and clients. So IBusBus would not be connected and
     * ibusimcontext->ibuscontext == NULL and ibusimcontext->events_queue
     * could go beyond MAX_QUEUED_EVENTS . */
    _daemon_is_running = (ibus_get_address () != NULL);
}

static void
daemon_name_vanished (GDBusConnection *connection,
                      const gchar     *name,
                      gpointer         data)
{
    _daemon_is_running = FALSE;
}

static void
ibus_im_context_class_init (IBusIMContextClass *class)
{
    IDEBUG ("%s", __FUNCTION__);

    GtkIMContextClass *im_context_class = GTK_IM_CONTEXT_CLASS (class);
    GObjectClass *gobject_class = G_OBJECT_CLASS (class);

    parent_class = (GtkIMContextClass *) g_type_class_peek_parent (class);

    im_context_class->reset = ibus_im_context_reset;
    im_context_class->focus_in = ibus_im_context_focus_in;
    im_context_class->focus_out = ibus_im_context_focus_out;
    im_context_class->filter_keypress = ibus_im_context_filter_keypress;
    im_context_class->get_preedit_string = ibus_im_context_get_preedit_string;
    im_context_class->set_client_window = ibus_im_context_set_client_window;
    im_context_class->set_cursor_location = ibus_im_context_set_cursor_location;
    im_context_class->set_use_preedit = ibus_im_context_set_use_preedit;
    im_context_class->set_surrounding = ibus_im_context_set_surrounding;
    gobject_class->notify = ibus_im_context_notify;
    gobject_class->finalize = ibus_im_context_finalize;

    _signal_commit_id =
        g_signal_lookup ("commit", G_TYPE_FROM_CLASS (class));
    g_assert (_signal_commit_id != 0);

    _signal_preedit_changed_id =
        g_signal_lookup ("preedit-changed", G_TYPE_FROM_CLASS (class));
    g_assert (_signal_preedit_changed_id != 0);

    _signal_preedit_start_id =
        g_signal_lookup ("preedit-start", G_TYPE_FROM_CLASS (class));
    g_assert (_signal_preedit_start_id != 0);

    _signal_preedit_end_id =
        g_signal_lookup ("preedit-end", G_TYPE_FROM_CLASS (class));
    g_assert (_signal_preedit_end_id != 0);

    _signal_delete_surrounding_id =
        g_signal_lookup ("delete-surrounding", G_TYPE_FROM_CLASS (class));
    g_assert (_signal_delete_surrounding_id != 0);

    _signal_retrieve_surrounding_id =
        g_signal_lookup ("retrieve-surrounding", G_TYPE_FROM_CLASS (class));
    g_assert (_signal_retrieve_surrounding_id != 0);

    _use_key_snooper = !_get_boolean_env ("IBUS_DISABLE_SNOOPER",
                                          !(ENABLE_SNOOPER));
    _use_sync_mode = _get_boolean_env ("IBUS_ENABLE_SYNC_MODE", FALSE);

    /* env IBUS_DISABLE_SNOOPER does not exist */
    if (_use_key_snooper) {
        /* disable snooper if app is in _no_snooper_apps */
        const gchar * prgname = g_get_prgname ();
        if (g_getenv ("IBUS_NO_SNOOPER_APPS")) {
            _no_snooper_apps = g_getenv ("IBUS_NO_SNOOPER_APPS");
        }
        gchar **p;
        gchar ** apps = g_strsplit (_no_snooper_apps, ",", 0);
        for (p = apps; *p != NULL; p++) {
            if (g_regex_match_simple (*p, prgname, 0, 0)) {
                _use_key_snooper = FALSE;
                break;
            }
        }
        g_strfreev (apps);
    }

    /* init bus object */
    if (_bus == NULL) {
        ibus_set_display (gdk_display_get_name (gdk_display_get_default ()));
        _bus = ibus_bus_new_async ();

        /* init the global fake context */
        if (ibus_bus_is_connected (_bus)) {
            _create_fake_input_context ();
        }

        g_signal_connect (_bus, "connected", G_CALLBACK (_bus_connected_cb), NULL);
    }


    /* always install snooper */
    if (_key_snooper_id == 0) {
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
        _key_snooper_id = gtk_key_snooper_install (_key_snooper_cb, NULL);
#pragma GCC diagnostic pop
    }

    _daemon_name_watch_id = g_bus_watch_name (G_BUS_TYPE_SESSION,
                                              IBUS_SERVICE_IBUS,
                                              G_BUS_NAME_WATCHER_FLAGS_NONE,
                                              daemon_name_appeared,
                                              daemon_name_vanished,
                                              NULL,
                                              NULL);
}

static void
ibus_im_context_class_fini (IBusIMContextClass *class)
{
    if (_key_snooper_id != 0) {
        IDEBUG ("snooper is terminated.");
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
        gtk_key_snooper_remove (_key_snooper_id);
#pragma GCC diagnostic pop
        _key_snooper_id = 0;
    }

    g_bus_unwatch_name (_daemon_name_watch_id);
}

/* Copied from gtk+2.0-2.20.1/modules/input/imcedilla.c to fix crosbug.com/11421.
 * Overwrite the original Gtk+'s compose table in gtk+-2.x.y/gtk/gtkimcontextsimple.c. */

/* The difference between this and the default input method is the handling
 * of C+acute - this method produces C WITH CEDILLA rather than C WITH ACUTE.
 * For languages that use CCedilla and not acute, this is the preferred mapping,
 * and is particularly important for pt_BR, where the us-intl keyboard is
 * used extensively.
 */
static guint16 cedilla_compose_seqs[] = {
#ifdef DEPRECATED_GDK_KEYSYMS
  GDK_dead_acute,	GDK_C,	0,	0,	0,	0x00C7,	/* LATIN_CAPITAL_LETTER_C_WITH_CEDILLA */
  GDK_dead_acute,	GDK_c,	0,	0,	0,	0x00E7,	/* LATIN_SMALL_LETTER_C_WITH_CEDILLA */
  GDK_Multi_key,	GDK_apostrophe,	GDK_C,  0,      0,      0x00C7, /* LATIN_CAPITAL_LETTER_C_WITH_CEDILLA */
  GDK_Multi_key,	GDK_apostrophe,	GDK_c,  0,      0,      0x00E7, /* LATIN_SMALL_LETTER_C_WITH_CEDILLA */
  GDK_Multi_key,	GDK_C,  GDK_apostrophe,	0,      0,      0x00C7, /* LATIN_CAPITAL_LETTER_C_WITH_CEDILLA */
  GDK_Multi_key,	GDK_c,  GDK_apostrophe,	0,      0,      0x00E7, /* LATIN_SMALL_LETTER_C_WITH_CEDILLA */
#else
  GDK_KEY_dead_acute,	GDK_KEY_C,	0,	0,	0,	0x00C7,	/* LATIN_CAPITAL_LETTER_C_WITH_CEDILLA */
  GDK_KEY_dead_acute,	GDK_KEY_c,	0,	0,	0,	0x00E7,	/* LATIN_SMALL_LETTER_C_WITH_CEDILLA */
  GDK_KEY_Multi_key,	GDK_KEY_apostrophe,	GDK_KEY_C,  0,      0,      0x00C7, /* LATIN_CAPITAL_LETTER_C_WITH_CEDILLA */
  GDK_KEY_Multi_key,	GDK_KEY_apostrophe,	GDK_KEY_c,  0,      0,      0x00E7, /* LATIN_SMALL_LETTER_C_WITH_CEDILLA */
  GDK_KEY_Multi_key,	GDK_KEY_C,  GDK_KEY_apostrophe,	0,      0,      0x00C7, /* LATIN_CAPITAL_LETTER_C_WITH_CEDILLA */
  GDK_KEY_Multi_key,	GDK_KEY_c,  GDK_KEY_apostrophe,	0,      0,      0x00E7, /* LATIN_SMALL_LETTER_C_WITH_CEDILLA */
#endif
};

static void
ibus_im_context_init (GObject *obj)
{
    IDEBUG ("%s", __FUNCTION__);

    IBusIMContext *ibusimcontext = IBUS_IM_CONTEXT (obj);

    ibusimcontext->client_window = NULL;

    // Init preedit status
    ibusimcontext->preedit_string = NULL;
    ibusimcontext->preedit_attrs = NULL;
    ibusimcontext->preedit_cursor_pos = 0;
    ibusimcontext->preedit_visible = FALSE;

    // Init cursor area
    ibusimcontext->cursor_area.x = -1;
    ibusimcontext->cursor_area.y = -1;
    ibusimcontext->cursor_area.width = 0;
    ibusimcontext->cursor_area.height = 0;

    ibusimcontext->ibuscontext = NULL;
    ibusimcontext->has_focus = FALSE;
    ibusimcontext->time = GDK_CURRENT_TIME;
#ifdef ENABLE_SURROUNDING
    ibusimcontext->caps = IBUS_CAP_PREEDIT_TEXT | IBUS_CAP_FOCUS | IBUS_CAP_SURROUNDING_TEXT;
#else
    ibusimcontext->caps = IBUS_CAP_PREEDIT_TEXT | IBUS_CAP_FOCUS;
#endif

    ibusimcontext->events_queue = g_queue_new ();

    // Create slave im context
    ibusimcontext->slave = gtk_im_context_simple_new ();
    gtk_im_context_simple_add_table (GTK_IM_CONTEXT_SIMPLE (ibusimcontext->slave),
                                     cedilla_compose_seqs,
                                     4,
                                     G_N_ELEMENTS (cedilla_compose_seqs) / (4 + 2));

    g_signal_connect (ibusimcontext->slave,
                      "commit",
                      G_CALLBACK (_slave_commit_cb),
                      ibusimcontext);
    g_signal_connect (ibusimcontext->slave,
                      "preedit-start",
                      G_CALLBACK (_slave_preedit_start_cb),
                      ibusimcontext);
    g_signal_connect (ibusimcontext->slave,
                      "preedit-end",
                      G_CALLBACK (_slave_preedit_end_cb),
                      ibusimcontext);
    g_signal_connect (ibusimcontext->slave,
                      "preedit-changed",
                      G_CALLBACK (_slave_preedit_changed_cb),
                      ibusimcontext);
    g_signal_connect (ibusimcontext->slave,
                      "retrieve-surrounding",
                      G_CALLBACK (_slave_retrieve_surrounding_cb),
                      ibusimcontext);
    g_signal_connect (ibusimcontext->slave,
                      "delete-surrounding",
                      G_CALLBACK (_slave_delete_surrounding_cb),
                      ibusimcontext);

    if (ibus_bus_is_connected (_bus)) {
        GVariant *value = NULL;
        value = ibus_bus_get_ibus_property (_bus, "CommitPreeditTextBeforeResettingIM");
        if (value) {
              _commit_preedit_text_before_resetting_im = g_variant_get_boolean (value);
              g_variant_unref (value);
        }

        _create_input_context (ibusimcontext);
    }

    g_signal_connect (_bus, "connected", G_CALLBACK (_bus_connected_cb), obj);
}

static void
ibus_im_context_notify (GObject    *obj,
                        GParamSpec *pspec)
{
    IDEBUG ("%s", __FUNCTION__);

    if (g_strcmp0 (pspec->name, "input-purpose") == 0 ||
        g_strcmp0 (pspec->name, "input-hints") == 0) {
        _set_content_type (IBUS_IM_CONTEXT (obj));
    }
}

static void
ibus_im_context_finalize (GObject *obj)
{
    IDEBUG ("%s", __FUNCTION__);

    IBusIMContext *ibusimcontext = IBUS_IM_CONTEXT (obj);

    g_signal_handlers_disconnect_by_func (_bus, G_CALLBACK (_bus_connected_cb), obj);

    if (ibusimcontext->cancellable != NULL) {
        /* Cancel any ongoing create input context request */
        g_cancellable_cancel (ibusimcontext->cancellable);
        g_object_unref (ibusimcontext->cancellable);
        ibusimcontext->cancellable = NULL;
    }

    if (ibusimcontext->ibuscontext) {
        ibus_proxy_destroy ((IBusProxy *)ibusimcontext->ibuscontext);
    }

    ibus_im_context_set_client_window ((GtkIMContext *)ibusimcontext, NULL);

    if (ibusimcontext->slave) {
        g_object_unref (ibusimcontext->slave);
        ibusimcontext->slave = NULL;
    }

    // release preedit
    if (ibusimcontext->preedit_string) {
        g_free (ibusimcontext->preedit_string);
    }
    if (ibusimcontext->preedit_attrs) {
        pango_attr_list_unref (ibusimcontext->preedit_attrs);
    }

    g_queue_free_full (ibusimcontext->events_queue,
                       (GDestroyNotify)gdk_event_free);

    G_OBJECT_CLASS(parent_class)->finalize (obj);
}

static gboolean
ibus_im_context_filter_keypress (GtkIMContext *context,
                                 GdkEventKey  *event)
{
    IDEBUG ("%s", __FUNCTION__);

    IBusIMContext *ibusimcontext = IBUS_IM_CONTEXT (context);

    if (!_daemon_is_running)
        return gtk_im_context_filter_keypress (ibusimcontext->slave, event);

    /* If context does not have focus, ibus will process key event in
     * sync mode.  It is a workaround for increase search in treeview.
     */
    if (!ibusimcontext->has_focus)
        return gtk_im_context_filter_keypress (ibusimcontext->slave, event);

    if (event->state & IBUS_HANDLED_MASK)
        return TRUE;

    if (event->state & IBUS_IGNORED_MASK)
        return gtk_im_context_filter_keypress (ibusimcontext->slave, event);

    /* XXX it is a workaround for some applications do not set client
     * window. */
    if (ibusimcontext->client_window == NULL && event->window != NULL)
        gtk_im_context_set_client_window ((GtkIMContext *)ibusimcontext,
                                          event->window);

    _request_surrounding_text (ibusimcontext);

    ibusimcontext->time = event->time;

    if (ibusimcontext->ibuscontext) {
        if (_process_key_event (ibusimcontext->ibuscontext, event))
            return TRUE;
        else
            return gtk_im_context_filter_keypress (ibusimcontext->slave,
                                                   event);
    }

    /* At this point we _should_ be waiting for the IBus context to be
     * created or the connection to IBus to be established. If that's
     * the case we queue events to be processed when the IBus context
     * is ready. */
    g_return_val_if_fail (ibusimcontext->cancellable != NULL ||
                          ibus_bus_is_connected (_bus) == FALSE,
                          FALSE);
    g_queue_push_tail (ibusimcontext->events_queue,
                       gdk_event_copy ((GdkEvent *)event));

    if (g_queue_get_length (ibusimcontext->events_queue) > MAX_QUEUED_EVENTS) {
        g_warning ("Events queue growing too big, will start to drop.");
        gdk_event_free ((GdkEvent *)
                        g_queue_pop_head (ibusimcontext->events_queue));
    }

    return TRUE;
}

static void
ibus_im_context_focus_in (GtkIMContext *context)
{
    IDEBUG ("%s", __FUNCTION__);

    IBusIMContext *ibusimcontext = (IBusIMContext *) context;

    if (ibusimcontext->has_focus)
        return;

    /* don't set focus on password entry */
    if (ibusimcontext->client_window != NULL) {
        GtkWidget *widget;

        gdk_window_get_user_data (ibusimcontext->client_window,
                                  (gpointer *)&widget);

        if (GTK_IS_ENTRY (widget) &&
            !gtk_entry_get_visibility (GTK_ENTRY (widget))) {
            return;
        }
    }

    /* Do not call gtk_im_context_focus_out() here.
     * google-chrome's notification popup window (Pushbullet)
     * takes the focus and the popup window disappears.
     * So other applications lose the focus because
     * ibusimcontext->has_focus is FALSE if
     * gtk_im_context_focus_out() is called here when
     * _focus_im_context != context.
     */
    if (_focus_im_context == NULL) {
        /* focus out fake context */
        if (_fake_context != NULL) {
            ibus_input_context_focus_out (_fake_context);
        }
    }

    ibusimcontext->has_focus = TRUE;
    if (ibusimcontext->ibuscontext) {
        _set_content_type (ibusimcontext);
        ibus_input_context_focus_in (ibusimcontext->ibuscontext);
    }

    gtk_im_context_focus_in (ibusimcontext->slave);

    /* set_cursor_location_internal() will get origin from X server,
     * it blocks UI. So delay it to idle callback. */
    gdk_threads_add_idle_full (G_PRIORITY_DEFAULT_IDLE,
                               (GSourceFunc) _set_cursor_location_internal,
                               g_object_ref (ibusimcontext),
                               (GDestroyNotify) g_object_unref);

    /* retrieve the initial surrounding-text (regardless of whether
     * the current IBus engine needs surrounding-text) */
    _request_surrounding_text (ibusimcontext);

    g_object_add_weak_pointer ((GObject *) context,
                               (gpointer *) &_focus_im_context);
    _focus_im_context = context;
}

static void
ibus_im_context_focus_out (GtkIMContext *context)
{
    IDEBUG ("%s", __FUNCTION__);
    IBusIMContext *ibusimcontext = (IBusIMContext *) context;

    if (ibusimcontext->has_focus == FALSE) {
        return;
    }

    g_object_remove_weak_pointer ((GObject *) context,
                                  (gpointer *) &_focus_im_context);
    _focus_im_context = NULL;

    ibusimcontext->has_focus = FALSE;
    if (ibusimcontext->ibuscontext) {
        ibus_input_context_focus_out (ibusimcontext->ibuscontext);
    }

    gtk_im_context_focus_out (ibusimcontext->slave);

    /* focus in the fake ic */
    if (_fake_context != NULL) {
        ibus_input_context_focus_in (_fake_context);
    }
}

static void
ibus_im_context_reset (GtkIMContext *context)
{
    IDEBUG ("%s", __FUNCTION__);

    IBusIMContext *ibusimcontext = IBUS_IM_CONTEXT (context);

    if (ibusimcontext->ibuscontext) {
        if (_commit_preedit_text_before_resetting_im &&
            ibusimcontext->preedit_string &&
            g_strcmp0 (ibusimcontext->preedit_string, ""))
        {
            IBusText *text_to_commit = ibus_text_new_from_string (g_strdup (ibusimcontext->preedit_string));
            /* clear and update preedit text */
            IBusText *empty_text = ibus_text_new_from_static_string ("");
            _ibus_context_update_preedit_text_cb (ibusimcontext->ibuscontext,
                                                  empty_text,
                                                  0,
                                                  ibusimcontext->preedit_visible,
                                                  ibusimcontext);
            g_object_unref (empty_text);
            /* commit text */
            _ibus_context_commit_text_cb (ibusimcontext->ibuscontext,
                                          text_to_commit,
                                          ibusimcontext);
            g_object_unref (text_to_commit);
        }
        ibus_input_context_reset (ibusimcontext->ibuscontext);
    }
    gtk_im_context_reset (ibusimcontext->slave);
}


static void
ibus_im_context_get_preedit_string (GtkIMContext   *context,
                                    gchar         **str,
                                    PangoAttrList **attrs,
                                    gint           *cursor_pos)
{
    IDEBUG ("%s", __FUNCTION__);

    IBusIMContext *ibusimcontext = IBUS_IM_CONTEXT (context);

    if (ibusimcontext->preedit_visible) {
        if (str) {
            *str = g_strdup (ibusimcontext->preedit_string ? ibusimcontext->preedit_string: "");
        }

        if (attrs) {
            *attrs = ibusimcontext->preedit_attrs ?
                        pango_attr_list_ref (ibusimcontext->preedit_attrs):
                        pango_attr_list_new ();
        }

        if (cursor_pos) {
            *cursor_pos = ibusimcontext->preedit_cursor_pos;
        }
    }
    else {
        if (str) {
            *str = g_strdup ("");
        }
        if (attrs) {
            *attrs = pango_attr_list_new ();
        }
        if (cursor_pos) {
            *cursor_pos = 0;
        }
    }
    IDEBUG ("str=%s", *str);
}


static void
ibus_im_context_set_client_window (GtkIMContext *context, GdkWindow *client)
{
    IDEBUG ("%s", __FUNCTION__);

    IBusIMContext *ibusimcontext = IBUS_IM_CONTEXT (context);

    if (ibusimcontext->client_window) {
        g_object_unref (ibusimcontext->client_window);
        ibusimcontext->client_window = NULL;
    }

    if (client != NULL)
        ibusimcontext->client_window = g_object_ref (client);

    if (ibusimcontext->slave)
        gtk_im_context_set_client_window (ibusimcontext->slave, client);
}

static gboolean
_set_cursor_location_internal (IBusIMContext *ibusimcontext)
{
    GdkRectangle area;

    if(ibusimcontext->client_window == NULL ||
       ibusimcontext->ibuscontext == NULL) {
        return FALSE;
    }

    area = ibusimcontext->cursor_area;
    if (area.x == -1 && area.y == -1 && area.width == 0 && area.height == 0) {
#if GTK_CHECK_VERSION (2, 91, 0)
        area.x = 0;
        area.y += gdk_window_get_height (ibusimcontext->client_window);
#else
        gint w, h;
        gdk_drawable_get_size (ibusimcontext->client_window, &w, &h);
        area.y += h;
        area.x = 0;
#endif
    }

    gdk_window_get_root_coords (ibusimcontext->client_window,
                                area.x, area.y,
                                &area.x, &area.y);
    ibus_input_context_set_cursor_location (ibusimcontext->ibuscontext,
                                            area.x,
                                            area.y,
                                            area.width,
                                            area.height);
    return FALSE;
}

static void
ibus_im_context_set_cursor_location (GtkIMContext *context, GdkRectangle *area)
{
    IDEBUG ("%s", __FUNCTION__);

    IBusIMContext *ibusimcontext = IBUS_IM_CONTEXT (context);

    if (ibusimcontext->cursor_area.x == area->x &&
        ibusimcontext->cursor_area.y == area->y &&
        ibusimcontext->cursor_area.width == area->width &&
        ibusimcontext->cursor_area.height == area->height) {
        return;
    }
    ibusimcontext->cursor_area = *area;
    _set_cursor_location_internal (ibusimcontext);
    gtk_im_context_set_cursor_location (ibusimcontext->slave, area);
}

static void
ibus_im_context_set_use_preedit (GtkIMContext *context, gboolean use_preedit)
{
    IDEBUG ("%s", __FUNCTION__);

    IBusIMContext *ibusimcontext = IBUS_IM_CONTEXT (context);

    if (use_preedit) {
        ibusimcontext->caps |= IBUS_CAP_PREEDIT_TEXT;
    }
    else {
        ibusimcontext->caps &= ~IBUS_CAP_PREEDIT_TEXT;
    }
    if(ibusimcontext->ibuscontext) {
        ibus_input_context_set_capabilities (ibusimcontext->ibuscontext,
                                             ibusimcontext->caps);
    }
    gtk_im_context_set_use_preedit (ibusimcontext->slave, use_preedit);
}

static guint
get_selection_anchor_point (IBusIMContext *ibusimcontext,
                            guint cursor_pos,
                            guint surrounding_text_len)
{
    GtkWidget *widget;
    if (ibusimcontext->client_window == NULL) {
        return cursor_pos;
    }
    gdk_window_get_user_data (ibusimcontext->client_window, (gpointer *)&widget);

    if (!GTK_IS_TEXT_VIEW (widget)){
        return cursor_pos;
    }

    GtkTextView *text_view = GTK_TEXT_VIEW (widget);
    GtkTextBuffer *buffer = gtk_text_view_get_buffer (text_view);

    if (!gtk_text_buffer_get_has_selection (buffer)) {
        return cursor_pos;
    }

    GtkTextIter start_iter, end_iter, cursor_iter;
    if (!gtk_text_buffer_get_selection_bounds (buffer, &start_iter, &end_iter)) {
        return cursor_pos;
    }

    gtk_text_buffer_get_iter_at_mark (buffer,
                                      &cursor_iter,
                                      gtk_text_buffer_get_insert (buffer));

    guint start_index = gtk_text_iter_get_offset (&start_iter);
    guint end_index   = gtk_text_iter_get_offset (&end_iter);
    guint cursor_index = gtk_text_iter_get_offset (&cursor_iter);

    guint anchor;

    if (start_index == cursor_index) {
      anchor = end_index;
    } else if (end_index == cursor_index) {
      anchor = start_index;
    } else {
      return cursor_pos;
    }

    // Change absolute index to relative position.
    guint relative_origin = cursor_index - cursor_pos;

    if (anchor < relative_origin) {
      return cursor_pos;
    }
    anchor -= relative_origin;

    if (anchor > surrounding_text_len) {
      return cursor_pos;
    }

    return anchor;
}

static void
ibus_im_context_set_surrounding (GtkIMContext  *context,
                                 const gchar   *text,
                                 gint           len,
                                 gint           cursor_index)
{
    g_return_if_fail (context != NULL);
    g_return_if_fail (IBUS_IS_IM_CONTEXT (context));
    g_return_if_fail (text != NULL);
    g_return_if_fail (strlen (text) >= len);
    g_return_if_fail (0 <= cursor_index && cursor_index <= len);

    IBusIMContext *ibusimcontext = IBUS_IM_CONTEXT (context);

    if (ibusimcontext->ibuscontext) {
        IBusText *ibustext;
        guint cursor_pos;
        guint utf8_len;
        gchar *p;

        p = g_strndup (text, len);
        cursor_pos = g_utf8_strlen (p, cursor_index);
        utf8_len = g_utf8_strlen(p, len);
        ibustext = ibus_text_new_from_string (p);
        g_free (p);

        guint anchor_pos = get_selection_anchor_point (ibusimcontext,
                                                       cursor_pos,
                                                       utf8_len);
        ibus_input_context_set_surrounding_text (ibusimcontext->ibuscontext,
                                                 ibustext,
                                                 cursor_pos,
                                                 anchor_pos);
    }
    gtk_im_context_set_surrounding (ibusimcontext->slave,
                                    text,
                                    len,
                                    cursor_index);
}

static void
_bus_connected_cb (IBusBus          *bus,
                   IBusIMContext    *ibusimcontext)
{
    IDEBUG ("%s", __FUNCTION__);

    if (ibusimcontext) {
        GVariant *value = NULL;
        value = ibus_bus_get_ibus_property (bus, "CommitPreeditTextBeforeResettingIM");
        if (value) {
            _commit_preedit_text_before_resetting_im = g_variant_get_boolean (value);
            g_variant_unref (value);
        }

        _create_input_context (ibusimcontext);
    }
    else
        _create_fake_input_context ();
}

static void
_ibus_context_commit_text_cb (IBusInputContext *ibuscontext,
                              IBusText         *text,
                              IBusIMContext    *ibusimcontext)
{
    IDEBUG ("%s", __FUNCTION__);

    g_signal_emit (ibusimcontext, _signal_commit_id, 0, text->text);

    _request_surrounding_text (ibusimcontext);
}

static gboolean
_key_is_modifier (guint keyval)
{
  /* See gdkkeys-x11.c:_gdk_keymap_key_is_modifier() for how this
   * really should be implemented */

    switch (keyval) {
#ifdef DEPRECATED_GDK_KEYSYMS
    case GDK_Shift_L:
    case GDK_Shift_R:
    case GDK_Control_L:
    case GDK_Control_R:
    case GDK_Caps_Lock:
    case GDK_Shift_Lock:
    case GDK_Meta_L:
    case GDK_Meta_R:
    case GDK_Alt_L:
    case GDK_Alt_R:
    case GDK_Super_L:
    case GDK_Super_R:
    case GDK_Hyper_L:
    case GDK_Hyper_R:
    case GDK_ISO_Lock:
    case GDK_ISO_Level2_Latch:
    case GDK_ISO_Level3_Shift:
    case GDK_ISO_Level3_Latch:
    case GDK_ISO_Level3_Lock:
    case GDK_ISO_Level5_Shift:
    case GDK_ISO_Level5_Latch:
    case GDK_ISO_Level5_Lock:
    case GDK_ISO_Group_Shift:
    case GDK_ISO_Group_Latch:
    case GDK_ISO_Group_Lock:
        return TRUE;
#else
    case GDK_KEY_Shift_L:
    case GDK_KEY_Shift_R:
    case GDK_KEY_Control_L:
    case GDK_KEY_Control_R:
    case GDK_KEY_Caps_Lock:
    case GDK_KEY_Shift_Lock:
    case GDK_KEY_Meta_L:
    case GDK_KEY_Meta_R:
    case GDK_KEY_Alt_L:
    case GDK_KEY_Alt_R:
    case GDK_KEY_Super_L:
    case GDK_KEY_Super_R:
    case GDK_KEY_Hyper_L:
    case GDK_KEY_Hyper_R:
    case GDK_KEY_ISO_Lock:
    case GDK_KEY_ISO_Level2_Latch:
    case GDK_KEY_ISO_Level3_Shift:
    case GDK_KEY_ISO_Level3_Latch:
    case GDK_KEY_ISO_Level3_Lock:
    case GDK_KEY_ISO_Level5_Shift:
    case GDK_KEY_ISO_Level5_Latch:
    case GDK_KEY_ISO_Level5_Lock:
    case GDK_KEY_ISO_Group_Shift:
    case GDK_KEY_ISO_Group_Latch:
    case GDK_KEY_ISO_Group_Lock:
        return TRUE;
#endif
    default:
        return FALSE;
    }
}
/* Copy from gdk */
static GdkEventKey *
_create_gdk_event (IBusIMContext *ibusimcontext,
                   guint          keyval,
                   guint          keycode,
                   guint          state)
{
    gunichar c = 0;
    gchar buf[8];

    GdkEventKey *event = (GdkEventKey *)gdk_event_new ((state & IBUS_RELEASE_MASK) ? GDK_KEY_RELEASE : GDK_KEY_PRESS);

    if (ibusimcontext && ibusimcontext->client_window)
        event->window = g_object_ref (ibusimcontext->client_window);
    else if (_input_window)
        event->window = g_object_ref (_input_window);

    /* The time is copied the latest value from the previous
     * GdkKeyEvent in filter_keypress().
     *
     * We understand the best way would be to pass the all time value
     * to IBus functions process_key_event() and IBus DBus functions
     * ProcessKeyEvent() in IM clients and IM engines so that the
     * _create_gdk_event() could get the correct time values.
     * However it would causes to change many functions and the time value
     * would not provide the useful meanings for each IBus engines but just
     * pass the original value to ForwardKeyEvent().
     * We use the saved value at the moment.
     *
     * Another idea might be to have the time implementation in X servers
     * but some Xorg uses clock_gettime() and others use gettimeofday()
     * and the values would be different in each implementation and 
     * locale/remote X server. So probably that idea would not work. */
    if (ibusimcontext) {
        event->time = ibusimcontext->time;
    } else {
        event->time = GDK_CURRENT_TIME;
    }

    event->send_event = FALSE;
    event->state = state;
    event->keyval = keyval;
    event->string = NULL;
    event->length = 0;
    event->hardware_keycode = (keycode != 0) ? keycode + 8 : 0;
    event->group = 0;
    event->is_modifier = _key_is_modifier (keyval);

#ifdef DEPRECATED_GDK_KEYSYMS
    if (keyval != GDK_VoidSymbol)
#else
    if (keyval != GDK_KEY_VoidSymbol)
#endif
        c = gdk_keyval_to_unicode (keyval);

    if (c) {
        gsize bytes_written;
        gint len;

        /* Apply the control key - Taken from Xlib
         */
        if (event->state & GDK_CONTROL_MASK) {
            if ((c >= '@' && c < '\177') || c == ' ') c &= 0x1F;
            else if (c == '2') {
                event->string = g_memdup ("\0\0", 2);
                event->length = 1;
                buf[0] = '\0';
                goto out;
            }
            else if (c >= '3' && c <= '7') c -= ('3' - '\033');
            else if (c == '8') c = '\177';
            else if (c == '/') c = '_' & 0x1F;
        }

        len = g_unichar_to_utf8 (c, buf);
        buf[len] = '\0';

        event->string = g_locale_from_utf8 (buf, len,
                                            NULL, &bytes_written,
                                            NULL);
        if (event->string)
            event->length = bytes_written;
#ifdef DEPRECATED_GDK_KEYSYMS
    } else if (keyval == GDK_Escape) {
#else
    } else if (keyval == GDK_KEY_Escape) {
#endif
        event->length = 1;
        event->string = g_strdup ("\033");
    }
#ifdef DEPRECATED_GDK_KEYSYMS
    else if (keyval == GDK_Return ||
             keyval == GDK_KP_Enter) {
#else
    else if (keyval == GDK_KEY_Return ||
             keyval == GDK_KEY_KP_Enter) {
#endif
        event->length = 1;
        event->string = g_strdup ("\r");
    }

    if (!event->string) {
        event->length = 0;
        event->string = g_strdup ("");
    }
out:
    return event;
}

static void
_ibus_context_forward_key_event_cb (IBusInputContext  *ibuscontext,
                                    guint              keyval,
                                    guint              keycode,
                                    guint              state,
                                    IBusIMContext     *ibusimcontext)
{
    IDEBUG ("%s", __FUNCTION__);
    GdkEventKey *event = _create_gdk_event (ibusimcontext, keyval, keycode, state);
    gdk_event_put ((GdkEvent *)event);
    gdk_event_free ((GdkEvent *)event);
}

static void
_ibus_context_delete_surrounding_text_cb (IBusInputContext *ibuscontext,
                                          gint              offset_from_cursor,
                                          guint             nchars,
                                          IBusIMContext    *ibusimcontext)
{
    gboolean return_value;
    g_signal_emit (ibusimcontext, _signal_delete_surrounding_id, 0, offset_from_cursor, nchars, &return_value);
}

static void
_ibus_context_update_preedit_text_cb (IBusInputContext  *ibuscontext,
                                      IBusText          *text,
                                      gint               cursor_pos,
                                      gboolean           visible,
                                      IBusIMContext     *ibusimcontext)
{
    IDEBUG ("%s", __FUNCTION__);

    const gchar *str;
    gboolean flag;

    if (ibusimcontext->preedit_string) {
        g_free (ibusimcontext->preedit_string);
    }
    if (ibusimcontext->preedit_attrs) {
        pango_attr_list_unref (ibusimcontext->preedit_attrs);
        ibusimcontext->preedit_attrs = NULL;
    }

    str = text->text;
    ibusimcontext->preedit_string = g_strdup (str);
    if (text->attrs) {
        guint i;
        ibusimcontext->preedit_attrs = pango_attr_list_new ();
        for (i = 0; ; i++) {
            IBusAttribute *attr = ibus_attr_list_get (text->attrs, i);
            if (attr == NULL) {
                break;
            }

            PangoAttribute *pango_attr;
            switch (attr->type) {
            case IBUS_ATTR_TYPE_UNDERLINE:
                pango_attr = pango_attr_underline_new (attr->value);
                break;
            case IBUS_ATTR_TYPE_FOREGROUND:
                pango_attr = pango_attr_foreground_new (
                                        ((attr->value & 0xff0000) >> 8) | 0xff,
                                        ((attr->value & 0x00ff00)) | 0xff,
                                        ((attr->value & 0x0000ff) << 8) | 0xff);
                break;
            case IBUS_ATTR_TYPE_BACKGROUND:
                pango_attr = pango_attr_background_new (
                                        ((attr->value & 0xff0000) >> 8) | 0xff,
                                        ((attr->value & 0x00ff00)) | 0xff,
                                        ((attr->value & 0x0000ff) << 8) | 0xff);
                break;
            default:
                continue;
            }
            pango_attr->start_index = g_utf8_offset_to_pointer (str, attr->start_index) - str;
            pango_attr->end_index = g_utf8_offset_to_pointer (str, attr->end_index) - str;
            pango_attr_list_insert (ibusimcontext->preedit_attrs, pango_attr);
        }
    }

    ibusimcontext->preedit_cursor_pos = cursor_pos;

    flag = ibusimcontext->preedit_visible != visible;
    ibusimcontext->preedit_visible = visible;

    if (ibusimcontext->preedit_visible) {
        if (flag) {
            /* invisible => visible */
            g_signal_emit (ibusimcontext, _signal_preedit_start_id, 0);
        }
        g_signal_emit (ibusimcontext, _signal_preedit_changed_id, 0);
    }
    else {
        if (flag) {
            /* visible => invisible */
            g_signal_emit (ibusimcontext, _signal_preedit_changed_id, 0);
            g_signal_emit (ibusimcontext, _signal_preedit_end_id, 0);
        }
        else {
            /* still invisible */
            /* do nothing */
        }
    }
}

static void
_ibus_context_show_preedit_text_cb (IBusInputContext   *ibuscontext,
                                    IBusIMContext      *ibusimcontext)
{
    IDEBUG ("%s", __FUNCTION__);

    if (ibusimcontext->preedit_visible == TRUE)
        return;

    ibusimcontext->preedit_visible = TRUE;
    g_signal_emit (ibusimcontext, _signal_preedit_start_id, 0);
    g_signal_emit (ibusimcontext, _signal_preedit_changed_id, 0);

    _request_surrounding_text (ibusimcontext);
}

static void
_ibus_context_hide_preedit_text_cb (IBusInputContext *ibuscontext,
                                    IBusIMContext    *ibusimcontext)
{
    IDEBUG ("%s", __FUNCTION__);

    if (ibusimcontext->preedit_visible == FALSE)
        return;

    ibusimcontext->preedit_visible = FALSE;
    g_signal_emit (ibusimcontext, _signal_preedit_changed_id, 0);
    g_signal_emit (ibusimcontext, _signal_preedit_end_id, 0);
}

static void
_ibus_context_destroy_cb (IBusInputContext *ibuscontext,
                          IBusIMContext    *ibusimcontext)
{
    IDEBUG ("%s", __FUNCTION__);
    g_assert (ibusimcontext->ibuscontext == ibuscontext);

    g_object_unref (ibusimcontext->ibuscontext);
    ibusimcontext->ibuscontext = NULL;

    /* clear preedit */
    ibusimcontext->preedit_visible = FALSE;
    ibusimcontext->preedit_cursor_pos = 0;
    g_free (ibusimcontext->preedit_string);
    ibusimcontext->preedit_string = NULL;

    g_signal_emit (ibusimcontext, _signal_preedit_changed_id, 0);
    g_signal_emit (ibusimcontext, _signal_preedit_end_id, 0);
}

static void
_create_input_context_done (IBusBus       *bus,
                            GAsyncResult  *res,
                            IBusIMContext *ibusimcontext)
{
    GError *error = NULL;
    IBusInputContext *context = ibus_bus_create_input_context_async_finish (
            _bus, res, &error);

    if (ibusimcontext->cancellable != NULL) {
        g_object_unref (ibusimcontext->cancellable);
        ibusimcontext->cancellable = NULL;
    }

    if (context == NULL) {
        g_warning ("Create input context failed: %s.", error->message);
        g_error_free (error);
    }
    else {

        ibusimcontext->ibuscontext = context;

        g_signal_connect (ibusimcontext->ibuscontext,
                          "commit-text",
                          G_CALLBACK (_ibus_context_commit_text_cb),
                          ibusimcontext);
        g_signal_connect (ibusimcontext->ibuscontext,
                          "forward-key-event",
                          G_CALLBACK (_ibus_context_forward_key_event_cb),
                          ibusimcontext);
        g_signal_connect (ibusimcontext->ibuscontext,
                          "delete-surrounding-text",
                          G_CALLBACK (_ibus_context_delete_surrounding_text_cb),
                          ibusimcontext);
        g_signal_connect (ibusimcontext->ibuscontext,
                          "update-preedit-text",
                          G_CALLBACK (_ibus_context_update_preedit_text_cb),
                          ibusimcontext);
        g_signal_connect (ibusimcontext->ibuscontext,
                          "show-preedit-text",
                          G_CALLBACK (_ibus_context_show_preedit_text_cb),
                          ibusimcontext);
        g_signal_connect (ibusimcontext->ibuscontext,
                          "hide-preedit-text",
                          G_CALLBACK (_ibus_context_hide_preedit_text_cb),
                          ibusimcontext);
        g_signal_connect (ibusimcontext->ibuscontext, "destroy",
                          G_CALLBACK (_ibus_context_destroy_cb),
                          ibusimcontext);

        ibus_input_context_set_capabilities (ibusimcontext->ibuscontext, ibusimcontext->caps);

        if (ibusimcontext->has_focus) {
            /* The time order is _create_input_context() ->
             * ibus_im_context_notify() -> ibus_im_context_focus_in() ->
             * _create_input_context_done()
             * so _set_content_type() is called at the beginning here
             * because ibusimcontext->ibuscontext == NULL before. */
            _set_content_type (ibusimcontext);

            ibus_input_context_focus_in (ibusimcontext->ibuscontext);
            _set_cursor_location_internal (ibusimcontext);
        }

        if (!g_queue_is_empty (ibusimcontext->events_queue)) {
            GdkEventKey *event;
            while ((event = g_queue_pop_head (ibusimcontext->events_queue))) {
                _process_key_event (context, event);
                gdk_event_free ((GdkEvent *)event);
            }
        }
    }

    g_object_unref (ibusimcontext);
}

static void
_create_input_context (IBusIMContext *ibusimcontext)
{
    IDEBUG ("%s", __FUNCTION__);

    g_assert (ibusimcontext->ibuscontext == NULL);

    g_return_if_fail (ibusimcontext->cancellable == NULL);

    ibusimcontext->cancellable = g_cancellable_new ();

    ibus_bus_create_input_context_async (_bus,
            "gtk-im", -1,
            ibusimcontext->cancellable,
            (GAsyncReadyCallback)_create_input_context_done,
            g_object_ref (ibusimcontext));
}

/* Callback functions for slave context */
static void
_slave_commit_cb (GtkIMContext  *slave,
                  gchar         *string,
                  IBusIMContext *ibusimcontext)
{
    g_signal_emit (ibusimcontext, _signal_commit_id, 0, string);
}

static void
_slave_preedit_changed_cb (GtkIMContext  *slave,
                           IBusIMContext *ibusimcontext)
{
    if (ibusimcontext->ibuscontext) {
        return;
    }

    g_signal_emit (ibusimcontext, _signal_preedit_changed_id, 0);
}

static void
_slave_preedit_start_cb (GtkIMContext  *slave,
                         IBusIMContext *ibusimcontext)
{
    if (ibusimcontext->ibuscontext) {
        return;
    }

    g_signal_emit (ibusimcontext, _signal_preedit_start_id, 0);
}

static void
_slave_preedit_end_cb (GtkIMContext  *slave,
                       IBusIMContext *ibusimcontext)
{
    if (ibusimcontext->ibuscontext) {
        return;
    }
    g_signal_emit (ibusimcontext, _signal_preedit_end_id, 0);
}

static gboolean
_slave_retrieve_surrounding_cb (GtkIMContext  *slave,
                                IBusIMContext *ibusimcontext)
{
    gboolean return_value;

    if (ibusimcontext->ibuscontext) {
        return FALSE;
    }
    g_signal_emit (ibusimcontext, _signal_retrieve_surrounding_id, 0,
                   &return_value);
    return return_value;
}

static gboolean
_slave_delete_surrounding_cb (GtkIMContext  *slave,
                              gint           offset_from_cursor,
                              guint          nchars,
                              IBusIMContext *ibusimcontext)
{
    gboolean return_value;

    if (ibusimcontext->ibuscontext) {
        return FALSE;
    }
    g_signal_emit (ibusimcontext, _signal_delete_surrounding_id, 0, offset_from_cursor, nchars, &return_value);
    return return_value;
}

#ifdef OS_CHROMEOS
static void
_ibus_fake_context_destroy_cb (IBusInputContext *ibuscontext,
                               gpointer          user_data)
{
    /* The fack IC may be destroyed when the connection is lost.
     * Should release it. */
    g_assert (ibuscontext == _fake_context);
    g_object_unref (_fake_context);
    _fake_context = NULL;
}

static GCancellable     *_fake_cancellable = NULL;

static void
_create_fake_input_context_done (IBusBus       *bus,
                                 GAsyncResult  *res,
                                 IBusIMContext *ibusimcontext)
{
    GError *error = NULL;
    IBusInputContext *context = ibus_bus_create_input_context_async_finish (
            _bus, res, &error);

    if (_fake_cancellable != NULL) {
        g_object_unref (_fake_cancellable);
        _fake_cancellable = NULL;
    }

    if (context == NULL) {
        g_warning ("Create fake input context failed: %s.", error->message);
        g_error_free (error);
        return;
    }

    _fake_context = context;

    g_signal_connect (_fake_context, "forward-key-event",
                      G_CALLBACK (_ibus_context_forward_key_event_cb),
                      NULL);
    g_signal_connect (_fake_context, "destroy",
                      G_CALLBACK (_ibus_fake_context_destroy_cb),
                      NULL);

    guint32 caps = IBUS_CAP_PREEDIT_TEXT | IBUS_CAP_FOCUS | IBUS_CAP_SURROUNDING_TEXT;
    ibus_input_context_set_capabilities (_fake_context, caps);

    /* focus in/out the fake context */
    if (_focus_im_context == NULL)
        ibus_input_context_focus_in (_fake_context);
    else
        ibus_input_context_focus_out (_fake_context);
}

static void
_create_fake_input_context (void)
{
    g_return_if_fail (_fake_context == NULL);

     /* Global engine is always enabled in Chrome OS,
      * so create fake IC, and set focus if no other IC has focus.
     */

    if (_fake_cancellable != NULL) {
        g_cancellable_cancel (_fake_cancellable);
        g_object_unref (_fake_cancellable);
        _fake_cancellable = NULL;
    }

    _fake_cancellable = g_cancellable_new ();

    ibus_bus_create_input_context_async (_bus,
            "fake-gtk-im", -1,
            _fake_cancellable,
            (GAsyncReadyCallback)_create_fake_input_context_done,
            NULL);

}
#else
static void
_create_fake_input_context (void)
{
    /* For Linux desktop, do not use fake IC. */
}
#endif
