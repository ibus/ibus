/* -*- mode: C; c-basic-offset: 4; indent-tabs-mode: nil; -*- */
/* vim:set et sts=4: */
/* ibus - The Input Bus
 * Copyright (C) 2008-2014 Peng Huang <shawn.p.huang@gmail.com>
 * Copyright (C) 2015-2024 Takao Fujiwara <takao.fujiwara1@gmail.com>
 * Copyright (C) 2008-2024 Red Hat, Inc.
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
#include "inputcontext.h"

#include <string.h>

#include "engineproxy.h"
#include "factoryproxy.h"
#include "global.h"
#include "ibusimpl.h"
#include "marshalers.h"
#include "types.h"

#define MAX_SYNC_DATA 30

struct _SetEngineByDescData {
    /* context related to the data */
    BusInputContext *context;
    /* set engine by desc result, cancellable */
    GTask *task;
    /* a object to cancel bus_engine_proxy_new call */
    GCancellable *cancellable;
    /* a object being passed to the bus_input_context_set_engine_by_desc
     * function. if origin_cancellable is cancelled by someone,
     * we cancel the cancellable above as well. */
    GCancellable *origin_cancellable;
    gulong cancelled_handler_id;
};
typedef struct _SetEngineByDescData SetEngineByDescData;

typedef struct _DeleteSurroundingData {
    gint        offset;
    guint       nchars;
} DeleteSurroundingData;

typedef struct _SyncForwardingData {
    char        key;
    IBusText   *text;
} SyncForwardingData;

typedef struct _SyncForwardingPreData {
    char        key;
    IBusText   *text;
    union {
        guint                 uints[3];
        DeleteSurroundingData deleting;
    } u;
} SyncForwardingPreData;

struct _BusInputContext {
    IBusService parent;

    /* instance members */
    BusConnection *connection;
    BusEngineProxy *engine;
    gchar *client;

    gboolean has_focus;

    /* client capabilities */
    guint capabilities;

    /* cursor location */
    gint x;
    gint y;
    gint w;
    gint h;

    /* prev key event that are used for handling hot-keys */
    guint prev_keyval;
    guint prev_modifiers;

    /* preedit text */
    IBusText *preedit_text;
    guint     preedit_cursor_pos;
    gboolean  preedit_visible;
    guint     preedit_mode;
    gboolean  client_commit_preedit;

    /* auxiliary text */
    IBusText *auxiliary_text;
    gboolean  auxiliary_visible;

    /* lookup table */
    IBusLookupTable *lookup_table;
    gboolean lookup_table_visible;

    /* filter release */
    gboolean filter_release;

    /* is fake context */
    gboolean fake;

    /* incompleted set engine by desc request */
    SetEngineByDescData *data;

    /* content-type (primary purpose and hints) */
    guint    purpose;
    guint    hints;

    BusPanelProxy *emoji_extension;
    gboolean is_extension_lookup_table;
    GQueue *queue_during_process_key_event;
    gboolean use_post_process_key_event;
    gboolean processing_key_event;

    /* IBus CandidatePanel has focus if the client application does not support
     * the Wayland input-method protocol likes setting XMODIFIERS or
     * GTK_IM_MODULE enviroment variable in Wayland. So if the focus-out is
     * sent to the activated IBus engine, the engine may clear the preedit
     * and try to hide the CandidatePanel.
     */
    gboolean ignore_focus_out;
};

struct _BusInputContextClass {
    IBusServiceClass parent;

    /* class members */
    IBusEngineDesc *default_engine_desc;
};

enum {
    PROCESS_KEY_EVENT,
    SET_CURSOR_LOCATION,
    SET_CURSOR_LOCATION_RELATIVE,
    FOCUS_IN,
    FOCUS_OUT,
    UPDATE_PREEDIT_TEXT,
    SHOW_PREEDIT_TEXT,
    HIDE_PREEDIT_TEXT,
    UPDATE_AUXILIARY_TEXT,
    SHOW_AUXILIARY_TEXT,
    HIDE_AUXILIARY_TEXT,
    UPDATE_LOOKUP_TABLE,
    SHOW_LOOKUP_TABLE,
    HIDE_LOOKUP_TABLE,
    PAGE_UP_LOOKUP_TABLE,
    PAGE_DOWN_LOOKUP_TABLE,
    CURSOR_UP_LOOKUP_TABLE,
    CURSOR_DOWN_LOOKUP_TABLE,
    REGISTER_PROPERTIES,
    UPDATE_PROPERTY,
    ENGINE_CHANGED,
    REQUEST_ENGINE,
    SET_CONTENT_TYPE,
    PANEL_EXTENSION,
    LAST_SIGNAL,
};

enum {
    PROP_0,
};

typedef struct _BusInputContextPrivate BusInputContextPrivate;

static guint    context_signals[LAST_SIGNAL] = { 0 };

/* functions prototype */
static void     bus_input_context_destroy
                                   (BusInputContext       *context);
static void     bus_input_context_service_method_call
                                   (IBusService           *service,
                                    GDBusConnection       *connection,
                                    const gchar           *sender,
                                    const gchar           *object_path,
                                    const gchar           *interface_name,
                                    const gchar           *method_name,
                                    GVariant              *parameters,
                                    GDBusMethodInvocation *invocation);
static GVariant *
                bus_input_context_service_get_property
                                   (IBusService           *service,
                                    GDBusConnection       *connection,
                                    const gchar           *sender,
                                    const gchar           *object_path,
                                    const gchar           *interface_name,
                                    const gchar           *property_name,
                                    GError               **error);
static gboolean bus_input_context_service_set_property
                                   (IBusService           *service,
                                    GDBusConnection       *connection,
                                    const gchar           *sender,
                                    const gchar           *object_path,
                                    const gchar           *interface_name,
                                    const gchar           *property_name,
                                    GVariant              *value,
                                    GError               **error);
static void     bus_input_context_unset_engine
                                   (BusInputContext       *context);
static void     bus_input_context_show_preedit_text
                                   (BusInputContext       *context,
                                    gboolean               is_extension);
static void     bus_input_context_hide_preedit_text
                                   (BusInputContext       *context,
                                    gboolean               is_extension);
static void     bus_input_context_update_auxiliary_text
                                   (BusInputContext       *context,
                                    IBusText              *text,
                                    gboolean               visible);
static void     bus_input_context_show_auxiliary_text
                                   (BusInputContext       *context);
static void     bus_input_context_hide_auxiliary_text
                                   (BusInputContext       *context);
static void     bus_input_context_show_lookup_table
                                   (BusInputContext       *context);
static void     bus_input_context_hide_lookup_table
                                   (BusInputContext       *context);
static void     bus_input_context_page_up_lookup_table
                                   (BusInputContext       *context);
static void     bus_input_context_page_down_lookup_table
                                   (BusInputContext       *context);
static void     bus_input_context_cursor_up_lookup_table
                                   (BusInputContext       *context);
static void     bus_input_context_cursor_down_lookup_table
                                   (BusInputContext       *context);
static void     bus_input_context_register_properties
                                   (BusInputContext       *context,
                                    IBusPropList          *props);
static void     bus_input_context_update_property
                                   (BusInputContext       *context,
                                    IBusProperty          *prop);
static void     _engine_destroy_cb (BusEngineProxy        *factory,
                                    BusInputContext       *context);

static IBusText *text_empty = NULL;
static IBusLookupTable *lookup_table_empty = NULL;
static IBusPropList    *props_empty = NULL;

/* The interfaces available in this class, which consists of a list of
 * methods this class implements and a list of signals this class may
 * emit. Method calls to the interface that are not defined in this
 * XML will be automatically rejected by the GDBus library (see
 * src/ibusservice.c for details.) */
static const gchar introspection_xml[] =
    "<node>\n"
    "  <interface name='org.freedesktop.IBus.InputContext'>\n"
    /* properties */
    "    <property name='PostProcessKeyEvent' type='(a(yv))' access='read'>\n"
    "      <annotation name='org.gtk.GDBus.Since'\n"
    "          value='1.5.29' />\n"
    "      <annotation name='org.gtk.GDBus.DocString'\n"
    "          value='Stability: Unstable' />\n"
    "    </property>\n"
    "    <property name='ContentType' type='(uu)' access='write' />\n"
    "    <property name='ClientCommitPreedit' type='(b)' access='write' />\n"
    "    <property name='EffectivePostProcessKeyEvent' type='(b)' \n"
    "                                                  access='write'>\n"
    "      <annotation name='org.gtk.GDBus.Since'\n"
    "          value='1.5.29' />\n"
    "      <annotation name='org.gtk.GDBus.DocString'\n"
    "          value='Stability: Unstable' />\n"
    "    </property>\n"
    /* methods */
    "    <method name='ProcessKeyEvent'>\n"
    "      <arg direction='in'  type='u' name='keyval' />\n"
    "      <arg direction='in'  type='u' name='keycode' />\n"
    "      <arg direction='in'  type='u' name='state' />\n"
    "      <arg direction='out' type='b' name='handled' />\n"
    "    </method>\n"
    "    <method name='SetCursorLocation'>\n"
    "      <arg direction='in' type='i' name='x' />\n"
    "      <arg direction='in' type='i' name='y' />\n"
    "      <arg direction='in' type='i' name='w' />\n"
    "      <arg direction='in' type='i' name='h' />\n"
    "    </method>\n"
    "    <method name='SetCursorLocationRelative'>\n"
    "      <arg direction='in' type='i' name='x' />\n"
    "      <arg direction='in' type='i' name='y' />\n"
    "      <arg direction='in' type='i' name='w' />\n"
    "      <arg direction='in' type='i' name='h' />\n"
    "    </method>\n"
    "    <method name='ProcessHandWritingEvent'>\n"
    "      <arg direction='in' type='ad' name='coordinates' />\n"
    "    </method>\n"
    "    <method name='CancelHandWriting'>\n"
    "      <arg direction='in' type='u' name='n_strokes' />\n"
    "    </method>\n"
    "    <method name='FocusIn' />\n"
    "    <method name='FocusOut' />\n"
    "    <method name='Reset' />\n"
    "    <method name='SetCapabilities'>\n"
    "      <arg direction='in' type='u' name='caps' />\n"
    "    </method>\n"
    "    <method name='PropertyActivate'>\n"
    "      <arg direction='in' type='s' name='name' />\n"
    "      <arg direction='in' type='u' name='state' />\n"
    "    </method>\n"
    "    <method name='SetEngine'>\n"
    "      <arg direction='in' type='s' name='name' />\n"
    "    </method>\n"
    "    <method name='GetEngine'>\n"
    "      <arg direction='out' type='v' name='desc' />\n"
    "    </method>\n"
    "    <method name='SetSurroundingText'>\n"
    "      <arg direction='in' type='v' name='text' />\n"
    "      <arg direction='in' type='u' name='cursor_pos' />\n"
    "      <arg direction='in' type='u' name='anchor_pos' />\n"
    "    </method>\n"

    /* signals */
    "    <signal name='CommitText'>\n"
    "      <arg type='v' name='text' />\n"
    "    </signal>\n"
    "    <signal name='ForwardKeyEvent'>\n"
    "      <arg type='u' name='keyval' />\n"
    "      <arg type='u' name='keycode' />\n"
    "      <arg type='u' name='state' />\n"
    "    </signal>\n"
    "    <signal name='UpdatePreeditText'>\n"
    "      <arg type='v' name='text' />\n"
    "      <arg type='u' name='cursor_pos' />\n"
    "      <arg type='b' name='visible' />\n"
    "    </signal>\n"
    "    <signal name='UpdatePreeditTextWithMode'>\n"
    "      <arg type='v' name='text' />\n"
    "      <arg type='u' name='cursor_pos' />\n"
    "      <arg type='b' name='visible' />\n"
    "      <arg type='u' name='mode' />\n"
    "    </signal>\n"
    "    <signal name='ShowPreeditText'/>\n"
    "    <signal name='HidePreeditText'/>\n"
    "    <signal name='UpdateAuxiliaryText'>\n"
    "      <arg type='v' name='text' />\n"
    "      <arg type='b' name='visible' />\n"
    "    </signal>\n"
    "    <signal name='ShowAuxiliaryText'/>\n"
    "    <signal name='HideAuxiliaryText'/>\n"
    "    <signal name='UpdateLookupTable'>\n"
    "      <arg type='v' name='table' />\n"
    "      <arg type='b' name='visible' />\n"
    "    </signal>\n"
    "    <signal name='ShowLookupTable'/>\n"
    "    <signal name='HideLookupTable'/>\n"
    "    <signal name='PageUpLookupTable'/>\n"
    "    <signal name='PageDownLookupTable'/>\n"
    "    <signal name='CursorUpLookupTable'/>\n"
    "    <signal name='CursorDownLookupTable'/>\n"
    "    <signal name='RegisterProperties'>\n"
    "      <arg type='v' name='props' />\n"
    "    </signal>\n"
    "    <signal name='UpdateProperty'>\n"
    "      <arg type='v' name='prop' />\n"
    "    </signal>\n"
    "  </interface>\n"
    "</node>\n";

G_DEFINE_TYPE (BusInputContext, bus_input_context, IBUS_TYPE_SERVICE)

/* %TRUE if we can send preedit text to client. FALSE if the panel has to handle
 * it. Note that we check IBUS_CAP_FOCUS here since
 * when the capability is not set, the client has to handle a preedit text
 * regardless of the embed_preedit_text config. */
#define PREEDIT_CONDITION  \
    ((context->capabilities & IBUS_CAP_PREEDIT_TEXT) && \
     (bus_ibus_impl_is_embed_preedit_text (\
            BUS_DEFAULT_IBUS) || (context->capabilities & IBUS_CAP_FOCUS) == 0))

/* CandidatePanel takes the focus when it runs in the Wayland session without
 * the Wayland panel protocol and if the active engine receives the focus-out
 * events, the most engines try to hide CandidatePanel and the preedit.
 * It means CandidatePanel repeats to be shown and hidden if the client does
 * not support the Wayland protocol.
 * This way ignores the focus-out events in XIM and GTK2 applications and the
 * active engine can keep the focus virtually to continue to update the
 * preedit and lookup table.
 */
#define IGNORE_FOCUS_OUT_CONDITION  \
    ((!g_strcmp0 (context->client, "xim") || \
      !g_ascii_strncasecmp (context->client, "gtk-im", 6)) \
     && visible \
     && !context->is_extension_lookup_table \
     && bus_ibus_impl_is_wayland_session (BUS_DEFAULT_IBUS))

static void
_connection_destroy_cb (BusConnection   *connection,
                        BusInputContext *context)
{
    g_assert (BUS_IS_CONNECTION (connection));
    g_assert (BUS_IS_INPUT_CONTEXT (context));

    ibus_object_destroy (IBUS_OBJECT (context));
}

static void
bus_input_context_class_init (BusInputContextClass *class)
{
    IBusObjectClass *ibus_object_class = IBUS_OBJECT_CLASS (class);

    class->default_engine_desc = ibus_engine_desc_new ("dummy",
                                                       "",
                                                       "",
                                                       "",
                                                       "",
                                                       "",
                                                       "ibus-engine",
                                                       "");
    g_object_ref_sink (class->default_engine_desc);

    ibus_object_class->destroy =
            (IBusObjectDestroyFunc)bus_input_context_destroy;

    /* override the parent class's implementation. */
    IBUS_SERVICE_CLASS (class)->service_method_call =
        bus_input_context_service_method_call;
    IBUS_SERVICE_CLASS (class)->service_get_property =
        bus_input_context_service_get_property;
    IBUS_SERVICE_CLASS (class)->service_set_property =
        bus_input_context_service_set_property;
    /* register the xml so that bus_ibus_impl_service_method_call will be
     * called on a method call defined in the xml (e.g. 'FocusIn'.) */
    ibus_service_class_add_interfaces (IBUS_SERVICE_CLASS (class), introspection_xml);

    /* install glib signals that would be handled by other classes like
     * ibusimpl.c and panelproxy.c.
     */
    context_signals[PROCESS_KEY_EVENT] =
        g_signal_new (I_("process-key-event"),
            G_TYPE_FROM_CLASS (class),
            G_SIGNAL_RUN_LAST,
            0,
            NULL, NULL,
            bus_marshal_BOOLEAN__UINT_UINT_UINT,
            G_TYPE_BOOLEAN,
            3,
            G_TYPE_UINT,
            G_TYPE_UINT,
            G_TYPE_UINT);
    g_signal_set_va_marshaller (context_signals[PROCESS_KEY_EVENT],
                                G_TYPE_FROM_CLASS (class),
                                bus_marshal_BOOLEAN__UINT_UINT_UINTv);

    context_signals[SET_CURSOR_LOCATION] =
        g_signal_new (I_("set-cursor-location"),
            G_TYPE_FROM_CLASS (class),
            G_SIGNAL_RUN_LAST,
            0,
            NULL, NULL,
            bus_marshal_VOID__INT_INT_INT_INT,
            G_TYPE_NONE,
            4,
            G_TYPE_INT,
            G_TYPE_INT,
            G_TYPE_INT,
            G_TYPE_INT);
    g_signal_set_va_marshaller (context_signals[SET_CURSOR_LOCATION],
                                G_TYPE_FROM_CLASS (class),
                                bus_marshal_VOID__INT_INT_INT_INTv);

    context_signals[SET_CURSOR_LOCATION_RELATIVE] =
        g_signal_new (I_("set-cursor-location-relative"),
            G_TYPE_FROM_CLASS (class),
            G_SIGNAL_RUN_LAST,
            0,
            NULL, NULL,
            bus_marshal_VOID__INT_INT_INT_INT,
            G_TYPE_NONE,
            4,
            G_TYPE_INT,
            G_TYPE_INT,
            G_TYPE_INT,
            G_TYPE_INT);
    g_signal_set_va_marshaller (context_signals[SET_CURSOR_LOCATION_RELATIVE],
                                G_TYPE_FROM_CLASS (class),
                                bus_marshal_VOID__INT_INT_INT_INTv);

    context_signals[FOCUS_IN] =
        g_signal_new (I_("focus-in"),
            G_TYPE_FROM_CLASS (class),
            G_SIGNAL_RUN_LAST,
            0,
            NULL, NULL,
            bus_marshal_VOID__VOID,
            G_TYPE_NONE, 0);
    g_signal_set_va_marshaller (context_signals[FOCUS_IN],
                                G_TYPE_FROM_CLASS (class),
                                bus_marshal_VOID__VOIDv);

    context_signals[FOCUS_OUT] =
        g_signal_new (I_("focus-out"),
            G_TYPE_FROM_CLASS (class),
            G_SIGNAL_RUN_LAST,
            0,
            NULL, NULL,
            bus_marshal_VOID__VOID,
            G_TYPE_NONE, 0);
    g_signal_set_va_marshaller (context_signals[FOCUS_OUT],
                                G_TYPE_FROM_CLASS (class),
                                bus_marshal_VOID__VOIDv);

    context_signals[UPDATE_PREEDIT_TEXT] =
        g_signal_new (I_("update-preedit-text"),
            G_TYPE_FROM_CLASS (class),
            G_SIGNAL_RUN_LAST,
            0,
            NULL, NULL,
            bus_marshal_VOID__OBJECT_UINT_BOOLEAN,
            G_TYPE_NONE,
            3,
            IBUS_TYPE_TEXT,
            G_TYPE_UINT,
            G_TYPE_BOOLEAN);
    g_signal_set_va_marshaller (context_signals[UPDATE_PREEDIT_TEXT],
                                G_TYPE_FROM_CLASS (class),
                                bus_marshal_VOID__OBJECT_UINT_BOOLEANv);

    context_signals[SHOW_PREEDIT_TEXT] =
        g_signal_new (I_("show-preedit-text"),
            G_TYPE_FROM_CLASS (class),
            G_SIGNAL_RUN_LAST,
            0,
            NULL, NULL,
            bus_marshal_VOID__VOID,
            G_TYPE_NONE,
            0);
    g_signal_set_va_marshaller (context_signals[SHOW_PREEDIT_TEXT],
                                G_TYPE_FROM_CLASS (class),
                                bus_marshal_VOID__VOIDv);

    context_signals[HIDE_PREEDIT_TEXT] =
        g_signal_new (I_("hide-preedit-text"),
            G_TYPE_FROM_CLASS (class),
            G_SIGNAL_RUN_LAST,
            0,
            NULL, NULL,
            bus_marshal_VOID__VOID,
            G_TYPE_NONE,
            0);
    g_signal_set_va_marshaller (context_signals[HIDE_PREEDIT_TEXT],
                                G_TYPE_FROM_CLASS (class),
                                bus_marshal_VOID__VOIDv);

    context_signals[UPDATE_AUXILIARY_TEXT] =
        g_signal_new (I_("update-auxiliary-text"),
            G_TYPE_FROM_CLASS (class),
            G_SIGNAL_RUN_LAST,
            0,
            NULL, NULL,
            bus_marshal_VOID__OBJECT_BOOLEAN,
            G_TYPE_NONE,
            2,
            IBUS_TYPE_TEXT,
            G_TYPE_BOOLEAN);
    g_signal_set_va_marshaller (context_signals[UPDATE_AUXILIARY_TEXT],
                                G_TYPE_FROM_CLASS (class),
                                bus_marshal_VOID__OBJECT_BOOLEANv);

    context_signals[SHOW_AUXILIARY_TEXT] =
        g_signal_new (I_("show-auxiliary-text"),
            G_TYPE_FROM_CLASS (class),
            G_SIGNAL_RUN_LAST,
            0,
            NULL, NULL,
            bus_marshal_VOID__VOID,
            G_TYPE_NONE,
            0);
    g_signal_set_va_marshaller (context_signals[SHOW_AUXILIARY_TEXT],
                                G_TYPE_FROM_CLASS (class),
                                bus_marshal_VOID__VOIDv);

    context_signals[HIDE_AUXILIARY_TEXT] =
        g_signal_new (I_("hide-auxiliary-text"),
            G_TYPE_FROM_CLASS (class),
            G_SIGNAL_RUN_LAST,
            0,
            NULL, NULL,
            bus_marshal_VOID__VOID,
            G_TYPE_NONE,
            0);
    g_signal_set_va_marshaller (context_signals[HIDE_AUXILIARY_TEXT],
                                G_TYPE_FROM_CLASS (class),
                                bus_marshal_VOID__VOIDv);

    context_signals[UPDATE_LOOKUP_TABLE] =
        g_signal_new (I_("update-lookup-table"),
            G_TYPE_FROM_CLASS (class),
            G_SIGNAL_RUN_LAST,
            0,
            NULL, NULL,
            bus_marshal_VOID__OBJECT_BOOLEAN,
            G_TYPE_NONE,
            2,
            IBUS_TYPE_LOOKUP_TABLE,
            G_TYPE_BOOLEAN);
    g_signal_set_va_marshaller (context_signals[UPDATE_LOOKUP_TABLE],
                                G_TYPE_FROM_CLASS (class),
                                bus_marshal_VOID__OBJECT_BOOLEANv);

    context_signals[SHOW_LOOKUP_TABLE] =
        g_signal_new (I_("show-lookup-table"),
            G_TYPE_FROM_CLASS (class),
            G_SIGNAL_RUN_LAST,
            0,
            NULL, NULL,
            bus_marshal_VOID__VOID,
            G_TYPE_NONE, 0);
    g_signal_set_va_marshaller (context_signals[SHOW_LOOKUP_TABLE],
                                G_TYPE_FROM_CLASS (class),
                                bus_marshal_VOID__VOIDv);

    context_signals[HIDE_LOOKUP_TABLE] =
        g_signal_new (I_("hide-lookup-table"),
            G_TYPE_FROM_CLASS (class),
            G_SIGNAL_RUN_LAST,
            0,
            NULL, NULL,
            bus_marshal_VOID__VOID,
            G_TYPE_NONE, 0);
    g_signal_set_va_marshaller (context_signals[HIDE_LOOKUP_TABLE],
                                G_TYPE_FROM_CLASS (class),
                                bus_marshal_VOID__VOIDv);

    context_signals[PAGE_UP_LOOKUP_TABLE] =
        g_signal_new (I_("page-up-lookup-table"),
            G_TYPE_FROM_CLASS (class),
            G_SIGNAL_RUN_LAST,
            0,
            NULL, NULL,
            bus_marshal_VOID__VOID,
            G_TYPE_NONE, 0);
    g_signal_set_va_marshaller (context_signals[PAGE_UP_LOOKUP_TABLE],
                                G_TYPE_FROM_CLASS (class),
                                bus_marshal_VOID__VOIDv);

    context_signals[PAGE_DOWN_LOOKUP_TABLE] =
        g_signal_new (I_("page-down-lookup-table"),
            G_TYPE_FROM_CLASS (class),
            G_SIGNAL_RUN_LAST,
            0,
            NULL, NULL,
            bus_marshal_VOID__VOID,
            G_TYPE_NONE, 0);
    g_signal_set_va_marshaller (context_signals[PAGE_DOWN_LOOKUP_TABLE],
                                G_TYPE_FROM_CLASS (class),
                                bus_marshal_VOID__VOIDv);

    context_signals[CURSOR_UP_LOOKUP_TABLE] =
        g_signal_new (I_("cursor-up-lookup-table"),
            G_TYPE_FROM_CLASS (class),
            G_SIGNAL_RUN_LAST,
            0,
            NULL, NULL,
            bus_marshal_VOID__VOID,
            G_TYPE_NONE, 0);
    g_signal_set_va_marshaller (context_signals[CURSOR_UP_LOOKUP_TABLE],
                                G_TYPE_FROM_CLASS (class),
                                bus_marshal_VOID__VOIDv);

    context_signals[CURSOR_DOWN_LOOKUP_TABLE] =
        g_signal_new (I_("cursor-down-lookup-table"),
            G_TYPE_FROM_CLASS (class),
            G_SIGNAL_RUN_LAST,
            0,
            NULL, NULL,
            bus_marshal_VOID__VOID,
            G_TYPE_NONE, 0);
    g_signal_set_va_marshaller (context_signals[CURSOR_DOWN_LOOKUP_TABLE],
                                G_TYPE_FROM_CLASS (class),
                                bus_marshal_VOID__VOIDv);

    context_signals[REGISTER_PROPERTIES] =
        g_signal_new (I_("register-properties"),
            G_TYPE_FROM_CLASS (class),
            G_SIGNAL_RUN_LAST,
            0,
            NULL, NULL,
            bus_marshal_VOID__OBJECT,
            G_TYPE_NONE,
            1,
            IBUS_TYPE_PROP_LIST);
    g_signal_set_va_marshaller (context_signals[REGISTER_PROPERTIES],
                                G_TYPE_FROM_CLASS (class),
                                bus_marshal_VOID__OBJECTv);

    context_signals[UPDATE_PROPERTY] =
        g_signal_new (I_("update-property"),
            G_TYPE_FROM_CLASS (class),
            G_SIGNAL_RUN_LAST,
            0,
            NULL, NULL,
            bus_marshal_VOID__OBJECT,
            G_TYPE_NONE,
            1,
            IBUS_TYPE_PROPERTY);
    g_signal_set_va_marshaller (context_signals[UPDATE_PROPERTY],
                                G_TYPE_FROM_CLASS (class),
                                bus_marshal_VOID__OBJECTv);

    context_signals[ENGINE_CHANGED] =
        g_signal_new (I_("engine-changed"),
            G_TYPE_FROM_CLASS (class),
            G_SIGNAL_RUN_LAST,
            0,
            NULL, NULL,
            bus_marshal_VOID__VOID,
            G_TYPE_NONE,
            0);
    g_signal_set_va_marshaller (context_signals[ENGINE_CHANGED],
                                G_TYPE_FROM_CLASS (class),
                                bus_marshal_VOID__VOIDv);

    /* This signal is not for notifying an event on this object, but is for
     * requesting an engine as the name shows.
     * On the signal emission, ibusimpl.c will immediately update the
     * context->engine variable.
     */
    context_signals[REQUEST_ENGINE] =
        g_signal_new (I_("request-engine"),
            G_TYPE_FROM_CLASS (class),
            G_SIGNAL_RUN_LAST,
            0,
            NULL, NULL,
            bus_marshal_OBJECT__STRING,
            IBUS_TYPE_ENGINE_DESC,
            1,
            G_TYPE_STRING | G_SIGNAL_TYPE_STATIC_SCOPE);
    g_signal_set_va_marshaller (context_signals[REQUEST_ENGINE],
                                G_TYPE_FROM_CLASS (class),
                                bus_marshal_OBJECT__STRINGv);

    context_signals[SET_CONTENT_TYPE] =
        g_signal_new (I_("set-content-type"),
            G_TYPE_FROM_CLASS (class),
            G_SIGNAL_RUN_LAST,
            0,
            NULL, NULL,
            bus_marshal_VOID__UINT_UINT,
            G_TYPE_NONE,
            2,
            G_TYPE_UINT,
            G_TYPE_UINT);
    g_signal_set_va_marshaller (context_signals[SET_CONTENT_TYPE],
                                G_TYPE_FROM_CLASS (class),
                                bus_marshal_VOID__UINT_UINTv);

    context_signals[PANEL_EXTENSION] =
        g_signal_new (I_("panel-extension"),
            G_TYPE_FROM_CLASS (class),
            G_SIGNAL_RUN_LAST,
            0,
            NULL, NULL,
            bus_marshal_VOID__OBJECT,
            G_TYPE_NONE,
            1,
            IBUS_TYPE_EXTENSION_EVENT);
    g_signal_set_va_marshaller (context_signals[PANEL_EXTENSION],
                                G_TYPE_FROM_CLASS (class),
                                bus_marshal_VOID__OBJECTv);

    text_empty = ibus_text_new_from_string ("");
    g_object_ref_sink (text_empty);
    lookup_table_empty = ibus_lookup_table_new (9 /* page size */,
                                                0, FALSE, FALSE);
    g_object_ref_sink (lookup_table_empty);
    props_empty = ibus_prop_list_new ();
    g_object_ref_sink (props_empty);
}

static void
bus_input_context_init (BusInputContext *context)
{
    context->prev_keyval = IBUS_KEY_VoidSymbol;
    g_object_ref_sink (text_empty);
    context->preedit_text = text_empty;
    context->preedit_mode = IBUS_ENGINE_PREEDIT_CLEAR;
    g_object_ref_sink (text_empty);
    context->auxiliary_text = text_empty;
    g_object_ref_sink (lookup_table_empty);
    context->lookup_table = lookup_table_empty;
    /* other member variables will automatically be zero-cleared. */
}

static void
bus_input_context_destroy (BusInputContext *context)
{
    if (context->has_focus) {
        bus_input_context_focus_out (context);
        context->has_focus = FALSE;
    }

    if (context->engine) {
        bus_input_context_unset_engine (context);
    }

    if (context->preedit_text) {
        g_object_unref (context->preedit_text);
        context->preedit_text = NULL;
    }

    if (context->auxiliary_text) {
        g_object_unref (context->auxiliary_text);
        context->auxiliary_text = NULL;
    }

    if (context->lookup_table) {
        g_object_unref (context->lookup_table);
        context->lookup_table = NULL;
    }

    if (context->connection) {
        g_signal_handlers_disconnect_by_func (
                context->connection,
                (GCallback) _connection_destroy_cb,
                context);
        g_object_unref (context->connection);
        context->connection = NULL;
    }

    if (context->client) {
        g_free (context->client);
        context->client = NULL;
    }

    IBUS_OBJECT_CLASS (bus_input_context_parent_class)->
            destroy (IBUS_OBJECT (context));
}

static gboolean
bus_input_context_send_signal (BusInputContext *context,
                               const gchar     *interface_name,
                               const gchar     *signal_name,
                               GVariant        *parameters,
                               GError         **error)
{
    if (context->connection == NULL) {
        g_variant_unref (parameters);
        return TRUE;
    }

    GDBusMessage *message = g_dbus_message_new_signal (
            ibus_service_get_object_path ((IBusService *)context),
            interface_name,
            signal_name);
    g_dbus_message_set_sender (message, IBUS_NAME_OWNER_NAME);
    g_dbus_message_set_destination (
            message,
            bus_connection_get_unique_name (context->connection));
    if (parameters != NULL)
        g_dbus_message_set_body (message, parameters);

    gboolean retval =  g_dbus_connection_send_message (
            bus_connection_get_dbus_connection (context->connection),
            message,
            G_DBUS_SEND_MESSAGE_FLAGS_NONE,
            NULL, error);
    g_object_unref (message);
    return retval;
}

/**
 * bus_input_context_emit_signal:
 * @signal_name: The D-Bus signal name to emit which is in the
 * introspection_xml.
 *
 * Emit the D-Bus signal.
 */
static gboolean
bus_input_context_emit_signal (BusInputContext *context,
                               const gchar     *signal_name,
                               GVariant        *parameters,
                               GError         **error)
{
    if (context->connection == NULL) {
        /* fake context has no connections. */
        if (parameters)
            g_variant_unref (parameters);
        return TRUE;
    }

    return bus_input_context_send_signal (context,
                                          "org.freedesktop.IBus.InputContext",
                                          signal_name,
                                          parameters,
                                          error);
}

/**
 * bus_input_context_property_changed:
 * @context: a #BusInputContext
 * @property_name: The D-Bus property name which has changed
 * @value: The new value of the property
 *
 * Emit the D-Bus "PropertiesChanged" signal for a property.
 * Returns: %TRUE on success, %FALSE on failure
 */
static gboolean
bus_input_context_property_changed (BusInputContext *context,
                                    const gchar     *property_name,
                                    GVariant        *value,
                                    GError         **error)
{
    if (context->connection == NULL)
        return TRUE;

    GVariantBuilder builder;
    g_variant_builder_init (&builder, G_VARIANT_TYPE_ARRAY);
    g_variant_builder_add (&builder, "{sv}", property_name, value);
    return bus_input_context_send_signal (context,
                                          "org.freedesktop.DBus.Properties",
                                          "PropertiesChanged",
                                          g_variant_new ("(sa{sv}as)",
                                                         "org.freedesktop.IBus",
                                                         &builder,
                                                         NULL),
                                          error);
}


typedef struct _PanelProcessKeyEventData {
    GDBusMethodInvocation *invocation;
    BusInputContext *context;
} PanelProcessKeyEventData;

/**
 * _panel_process_key_event_cb:
 *
 * A GAsyncReadyCallback function to be called when
 * bus_panel_proxy_process_key_event() is finished.
 */
static void
_panel_process_key_event_cb (GObject                  *source,
                             GAsyncResult             *res,
                             PanelProcessKeyEventData *data)
{
    GError *error = NULL;
    GVariant *value = g_dbus_proxy_call_finish ((GDBusProxy *)source,
                                                 res,
                                                 &error);
    GDBusMethodInvocation *invocation;
    BusInputContext *context;

    g_assert (data);
    invocation = data->invocation;
    context = data->context;
    g_slice_free (PanelProcessKeyEventData, data);
    if (value != NULL) {
        g_dbus_method_invocation_return_value (invocation, value);
        g_variant_unref (value);
    }
    else {
        g_dbus_method_invocation_return_gerror (invocation, error);
        g_error_free (error);
    }
    context->processing_key_event = FALSE;
}

typedef struct _ProcessKeyEventData ProcessKeyEventData;
struct _ProcessKeyEventData {
    GDBusMethodInvocation *invocation;
    BusInputContext       *context;
    guint keyval;
    guint keycode;
    guint modifiers;
};

/**
 * _ic_process_key_event_reply_cb:
 *
 * A GAsyncReadyCallback function to be called when
 * bus_engine_proxy_process_key_event() is finished.
 */
static void
_ic_process_key_event_reply_cb (GObject               *source,
                                GAsyncResult          *res,
                                ProcessKeyEventData   *data)
{
    GDBusMethodInvocation *invocation = data->invocation;
    BusInputContext *context = data->context;
    guint keyval = data->keyval;
    guint keycode = data->keycode;
    guint modifiers = data->modifiers;
    GError *error = NULL;
    GVariant *value = g_dbus_proxy_call_finish ((GDBusProxy *)source,
                                                 res,
                                                 &error);

    if (value != NULL) {
        gboolean retval = FALSE;
        g_variant_get (value, "(b)", &retval);
        if (context->emoji_extension && !retval) {
            PanelProcessKeyEventData *pdata =
                    g_slice_new (PanelProcessKeyEventData);
            pdata->invocation = invocation;
            pdata->context = context;
            bus_panel_proxy_process_key_event (context->emoji_extension,
                                               keyval,
                                               keycode,
                                               modifiers,
                                               (GAsyncReadyCallback)
                                                    _panel_process_key_event_cb,
                                               pdata);
        } else {
            g_dbus_method_invocation_return_value (invocation, value);
            context->processing_key_event = FALSE;
        }
        g_variant_unref (value);
    }
    else {
        g_dbus_method_invocation_return_gerror (invocation, error);
        g_error_free (error);
        context->processing_key_event = FALSE;
    }

    g_object_unref (context);
    g_slice_free (ProcessKeyEventData, data);
}

static void
_forward_process_key_event_reply_cb (GObject               *source,
                                     GAsyncResult          *res,
                                     ProcessKeyEventData   *data)
{
    BusInputContext *context = data->context;
    GError *error = NULL;
    GVariant *value = g_dbus_proxy_call_finish ((GDBusProxy *)source,
                                                 res,
                                                 &error);
    if (value != NULL)
        g_variant_unref (value);
    else
        g_error_free (error);
    g_object_unref (context);
    g_slice_free (ProcessKeyEventData, data);
}

/**
 * _ic_process_key_event:
 *
 * Implement the "ProcessKeyEvent" method call of the
 * org.freedesktop.IBus.InputContext interface.
 */
static void
_ic_process_key_event (BusInputContext       *context,
                       GVariant              *parameters,
                       GDBusMethodInvocation *invocation)
{
    guint keyval = IBUS_KEY_VoidSymbol;
    guint keycode = 0;
    guint modifiers = 0;

    if (context->use_post_process_key_event)
        context->processing_key_event = TRUE;
    g_variant_get (parameters, "(uuu)", &keyval, &keycode, &modifiers);
    if (bus_ibus_impl_process_key_event (BUS_DEFAULT_IBUS,
                                         keyval,
                                         keycode,
                                         modifiers)) {
        /* If the shortcut key hits, it should return TRUE.
         * Otherwise a space would be inserted into the active input-context
         * by pressing Super-space.
         */
        g_dbus_method_invocation_return_value (invocation,
                                               g_variant_new ("(b)", TRUE));
        context->processing_key_event = FALSE;
        return;
    }
    if (G_UNLIKELY (!context->has_focus)) {
        /* workaround: set focus if context does not have focus */
        BusInputContext *focused_context =
                bus_ibus_impl_get_focused_input_context (BUS_DEFAULT_IBUS);
        if (focused_context == NULL ||
            focused_context->fake == TRUE ||
            context->fake == FALSE) {
            /* grab focus, if context is a real IC or current focused IC is
             * fake */
            bus_input_context_focus_in (context);
        }
    }

    /* If I move the focus from the URL entry box of google-chrome
     * to the text buffer of gnome-terminal,
     * focus-in/focus-out of google-chrome is caused after
     * focus-in of gonme-terminal and gnome-terminal loses the focus.
     * The following focus events are received in ibusimcontext:
     * 1) (gnome-terminal:445): IBUS-WARNING **: 15:32:36:717  focus_in
     * 2) (google-chrome:495): IBUS-WARNING **: 15:32:36:866  focus_out
     * 3) (google-chrome:495): IBUS-WARNING **: 15:32:36:875  focus_in
     * 4) (google-chrome:495): IBUS-WARNING **: 15:32:36:890  focus_out
     * In 2), Just return because focused_context is not google-chrome.
     * In 3), focused_context is changed from gnome-terminal to google-chrome
     * In 4), focused_context is changed from google-chrome to faked_context.
     *
     * It seems google-chrome has a popup window of the prediction of URL
     * and async focus-in/focus-out.
     */
    if (context->has_focus && context->engine == NULL &&
        context->fake == FALSE) {
        BusInputContext *focused_context =
                bus_ibus_impl_get_focused_input_context (BUS_DEFAULT_IBUS);

        if (focused_context != NULL && context != focused_context &&
            (context->capabilities & IBUS_CAP_FOCUS) != 0) {
            context->has_focus = FALSE;
            bus_input_context_focus_in (context);
        }
    }

    /* ignore key events, if it is a fake input context */
    if (context->has_focus && context->engine && context->fake == FALSE) {
        ProcessKeyEventData *data = g_slice_new0 (ProcessKeyEventData);
        data->invocation = invocation;
        data->context = g_object_ref (context);
        data->keyval = keyval;
        data->keycode = keycode;
        data->modifiers = modifiers;
        bus_engine_proxy_process_key_event (context->engine,
                                            keyval,
                                            keycode,
                                            modifiers,
                                            (GAsyncReadyCallback)
                                                _ic_process_key_event_reply_cb,
                                            data);
    }
    else {
        g_dbus_method_invocation_return_value (invocation,
                                               g_variant_new ("(b)", FALSE));
        context->processing_key_event = FALSE;
    }
}

/**
 * _ic_set_cursor_location:
 *
 * Implement the "SetCursorLocation" method call of the
 * org.freedesktop.IBus.InputContext interface.
 */
static void
_ic_set_cursor_location (BusInputContext       *context,
                         GVariant              *parameters,
                         GDBusMethodInvocation *invocation)
{
    g_dbus_method_invocation_return_value (invocation, NULL);

    g_variant_get (parameters, "(iiii)",
                   &context->x, &context->y, &context->w, &context->h);

    if (context->has_focus && context->engine) {
        bus_engine_proxy_set_cursor_location (context->engine,
                        context->x, context->y, context->w, context->h);
    }

    if (context->capabilities & IBUS_CAP_FOCUS) {
        g_signal_emit (context,
                       context_signals[SET_CURSOR_LOCATION],
                       0,
                       context->x,
                       context->y,
                       context->w,
                       context->h);
        if (context->emoji_extension) {
            bus_panel_proxy_set_cursor_location (context->emoji_extension,
                                                 context->x,
                                                 context->y,
                                                 context->w,
                                                 context->h);
        }
    }
}

/**
 * _ic_set_cursor_location_relative:
 *
 * Implement the "SetCursorLocationRelative" method call of the
 * org.freedesktop.IBus.InputContext interface.
 *
 * Unlike _ic_set_cursor_location, this doesn't deliver the location
 * to the engine proxy, since the relative coordinates are not very
 * useful for engines.
 */
static void
_ic_set_cursor_location_relative (BusInputContext       *context,
                                  GVariant              *parameters,
                                  GDBusMethodInvocation *invocation)
{
    gint x, y, w, h;

    g_dbus_method_invocation_return_value (invocation, NULL);

    g_variant_get (parameters, "(iiii)", &x, &y, &w, &h);

    if (context->capabilities & IBUS_CAP_FOCUS) {
        g_signal_emit (context,
                       context_signals[SET_CURSOR_LOCATION_RELATIVE],
                       0,
                       x,
                       y,
                       w,
                       h);
        if (context->emoji_extension) {
            bus_panel_proxy_set_cursor_location_relative (
                    context->emoji_extension,
                    x,
                    y,
                    w,
                    h);
        }
    }
}

static void
_ic_process_hand_writing_event (BusInputContext       *context,
                                GVariant              *parameters,
                                GDBusMethodInvocation *invocation)
{
    /* do nothing if it is a fake input context */
    if (context->has_focus &&
        context->engine && context->fake == FALSE) {
        bus_engine_proxy_process_hand_writing_event (context->engine,
                                                     parameters);
    }
    g_dbus_method_invocation_return_value (invocation, NULL);
}

static void
_ic_cancel_hand_writing (BusInputContext       *context,
                         GVariant              *parameters,
                         GDBusMethodInvocation *invocation)
{
    guint n_strokes = 0;
    g_variant_get (parameters, "(u)", &n_strokes);

    /* do nothing if it is a fake input context */
    if (context->has_focus &&
        context->engine && context->fake == FALSE) {
        bus_engine_proxy_cancel_hand_writing (context->engine, n_strokes);
    }
    g_dbus_method_invocation_return_value (invocation, NULL);
}

/**
 * _ic_focus_in:
 *
 * Implement the "FocusIn" method call of the org.freedesktop.IBus.InputContext
 * interface.
 */
static void
_ic_focus_in (BusInputContext       *context,
              GVariant              *parameters,
              GDBusMethodInvocation *invocation)
{
    if (context->capabilities & IBUS_CAP_FOCUS) {
        bus_input_context_focus_in (context);
        g_dbus_method_invocation_return_value (invocation, NULL);
    }
    else {
        g_dbus_method_invocation_return_error (
                invocation, G_DBUS_ERROR, G_DBUS_ERROR_FAILED,
                "The input context does not support focus.");
    }
}

/**
 * _ic_focus_out:
 *
 * Implement the "FocusOut" method call of the org.freedesktop.IBus.InputContext
 * interface.
 */
static void
_ic_focus_out (BusInputContext       *context,
               GVariant              *parameters,
               GDBusMethodInvocation *invocation)
{
    if (context->capabilities & IBUS_CAP_FOCUS) {
        if (context->ignore_focus_out)
            return;
        bus_input_context_focus_out (context);
        g_dbus_method_invocation_return_value (invocation, NULL);
    }
    else {
        g_dbus_method_invocation_return_error (
                invocation, G_DBUS_ERROR, G_DBUS_ERROR_FAILED,
                "The input context does not support focus.");
    }
}

/**
 * _ic_reset:
 *
 * Implement the "Reset" method call of the org.freedesktop.IBus.InputContext
 * interface.
 */
static void
_ic_reset (BusInputContext       *context,
           GVariant              *parameters,
           GDBusMethodInvocation *invocation)
{
    if (context->engine) {
        if (context->preedit_mode == IBUS_ENGINE_PREEDIT_COMMIT) {
            if (context->client_commit_preedit)
               bus_input_context_clear_preedit_text (context, FALSE);
            else
               bus_input_context_clear_preedit_text (context, TRUE);
        }
        bus_engine_proxy_reset (context->engine);
    }
    g_dbus_method_invocation_return_value (invocation, NULL);
}

/**
 * _ic_set_capabilities:
 *
 * Implement the "SetCapabilities" method call of the
 * org.freedesktop.IBus.InputContext interface.
 */
static void
_ic_set_capabilities (BusInputContext       *context,
                      GVariant              *parameters,
                      GDBusMethodInvocation *invocation)
{
    guint caps = 0;
    g_variant_get (parameters, "(u)", &caps);

    bus_input_context_set_capabilities (context, caps);

    g_dbus_method_invocation_return_value (invocation, NULL);
}

/**
 * _ic_property_activate:
 *
 * Implement the "PropertyActivate" method call of the
 * org.freedesktop.IBus.InputContext interface.
 */
static void
_ic_property_activate (BusInputContext       *context,
                       GVariant              *parameters,
                       GDBusMethodInvocation *invocation)
{
    gchar *prop_name = NULL;
    guint prop_state = 0;
    g_variant_get (parameters, "(&su)", &prop_name, &prop_state);

    if (context->engine) {
        bus_engine_proxy_property_activate (context->engine,
                                            prop_name,
                                            prop_state);
    }

#ifdef OS_CHROMEOS
    /* Global engine is always enabled in chromeos,
     * so pass PropertyActivate signal to the focused context.
     */
    else if (context->fake) {
        BusInputContext *focused_context =
                bus_ibus_impl_get_focused_input_context (BUS_DEFAULT_IBUS);
        if (focused_context && focused_context->engine)
            bus_engine_proxy_property_activate (focused_context->engine,
                                                prop_name,
                                                prop_state);
    }
#endif

    g_dbus_method_invocation_return_value (invocation, NULL);
}

static void
_ic_set_engine_done (BusInputContext       *context,
                     GAsyncResult          *res,
                     GDBusMethodInvocation *invocation)
{
    gboolean retval = FALSE;
    GError *error = NULL;

    retval = bus_input_context_set_engine_by_desc_finish (context,
                    res, &error);

    if (!retval) {
        g_dbus_method_invocation_return_gerror (invocation, error);
        g_error_free (error);
    }
    else {
        g_dbus_method_invocation_return_value (invocation, NULL);
    }
}

/**
 * _ic_set_engine:
 *
 * Implement the "SetEngine" method call of the
 * org.freedesktop.IBus.InputContext interface.
 */
static void
_ic_set_engine (BusInputContext       *context,
                GVariant              *parameters,
                GDBusMethodInvocation *invocation)
{
    gchar *engine_name = NULL;
    BusIBusImpl *ibus = bus_ibus_impl_get_default ();

    if (bus_ibus_impl_is_use_global_engine (ibus)) {
        g_dbus_method_invocation_return_error (invocation,
                G_DBUS_ERROR, G_DBUS_ERROR_FAILED,
                "Cannot set engines when use-global-engine is enabled.");
        return;
    }

    g_variant_get (parameters, "(&s)", &engine_name);

    if (!bus_input_context_has_focus (context)) {
        g_dbus_method_invocation_return_error (invocation,
                G_DBUS_ERROR, G_DBUS_ERROR_FAILED,
                "Context which does not has focus can not change engine to %s.",
                engine_name);
        return;
    }

    IBusEngineDesc *desc = NULL;
    g_signal_emit (context,
                   context_signals[REQUEST_ENGINE], 0,
                   engine_name,
                   &desc);
    if (desc == NULL) {
        g_dbus_method_invocation_return_error (invocation,
                        G_DBUS_ERROR, G_DBUS_ERROR_FAILED,
                        "Can not find engine %s.", engine_name);
        return;
    }

    bus_input_context_set_engine_by_desc (context,
                            desc,
                            g_gdbus_timeout,
                            NULL,
                            (GAsyncReadyCallback)_ic_set_engine_done,
                            invocation);
}

/**
 * _ic_get_engine:
 *
 * Implement the "GetEngine" method call of
 * the org.freedesktop.IBus.InputContext interface.
 */
static void
_ic_get_engine (BusInputContext       *context,
                GVariant              *parameters,
                GDBusMethodInvocation *invocation)
{
    IBusEngineDesc *desc = context->engine ?
            bus_engine_proxy_get_desc (context->engine) :
            BUS_INPUT_CONTEXT_GET_CLASS (context)->default_engine_desc;


    g_dbus_method_invocation_return_value (invocation,
            g_variant_new ("(v)",
                           ibus_serializable_serialize (
                                   (IBusSerializable *)desc)));
}

static void
_ic_set_surrounding_text (BusInputContext       *context,
                          GVariant              *parameters,
                          GDBusMethodInvocation *invocation)
{
    GVariant *variant = NULL;
    IBusText *text;
    guint cursor_pos = 0;
    guint anchor_pos = 0;

    g_variant_get (parameters,
                   "(vuu)",
                   &variant,
                   &cursor_pos,
                   &anchor_pos);
    text = IBUS_TEXT (ibus_serializable_deserialize (variant));
    g_variant_unref (variant);

    if ((context->capabilities & IBUS_CAP_SURROUNDING_TEXT) &&
         context->has_focus && context->engine) {
        bus_engine_proxy_set_surrounding_text (context->engine,
                                               text,
                                               cursor_pos,
                                               anchor_pos);
    }

    if (g_object_is_floating (text))
        g_object_unref (text);

    g_dbus_method_invocation_return_value (invocation, NULL);
}

/*
 * Since IBusService is inherited by IBusImpl, this method cannot be
 * applied to IBusServiceClass.method_call() directly but can be in
 * each child class.method_call().
 */
static gboolean
bus_input_context_service_authorized_method (IBusService     *service,
                                             GDBusConnection *connection)
{
    if (ibus_service_get_connection (service) == connection)
        return TRUE;
    return FALSE;
}

/**
 * bus_input_context_service_method_call:
 *
 * Handle a D-Bus method call whose destination and interface name are both
 * "org.freedesktop.IBus.InputContext"
 */
static void
bus_input_context_service_method_call (IBusService            *service,
                                       GDBusConnection        *connection,
                                       const gchar            *sender,
                                       const gchar            *object_path,
                                       const gchar            *interface_name,
                                       const gchar            *method_name,
                                       GVariant               *parameters,
                                       GDBusMethodInvocation  *invocation)
{
    if (g_strcmp0 (interface_name, IBUS_INTERFACE_INPUT_CONTEXT) != 0) {
        IBUS_SERVICE_CLASS (bus_input_context_parent_class)->
                service_method_call (service,
                                     connection,
                                     sender,
                                     object_path,
                                     interface_name,
                                     method_name,
                                     parameters,
                                     invocation);
        return;
    }

    static const struct {
        const gchar *method_name;
        void (* method_callback) (BusInputContext *,
                                  GVariant *,
                                  GDBusMethodInvocation *);
    } methods [] =  {
        { "ProcessKeyEvent",   _ic_process_key_event },
        { "SetCursorLocation", _ic_set_cursor_location },
        { "SetCursorLocationRelative", _ic_set_cursor_location_relative },
        { "ProcessHandWritingEvent",
                               _ic_process_hand_writing_event },
        { "CancelHandWriting", _ic_cancel_hand_writing },
        { "FocusIn",           _ic_focus_in },
        { "FocusOut",          _ic_focus_out },
        { "Reset",             _ic_reset },
        { "SetCapabilities",   _ic_set_capabilities },
        { "PropertyActivate",  _ic_property_activate },
        { "SetEngine",         _ic_set_engine },
        { "GetEngine",         _ic_get_engine },
        { "SetSurroundingText", _ic_set_surrounding_text }
    };

    gint i;

    if (!bus_input_context_service_authorized_method (service, connection))
        return;

    for (i = 0; i < G_N_ELEMENTS (methods); i++) {
        if (g_strcmp0 (method_name, methods[i].method_name) == 0) {
            methods[i].method_callback ((BusInputContext *)service,
                                        parameters,
                                        invocation);
            return;
        }
    }

    g_return_if_reached ();
}

/**
 * _ic_get_post_process_key_event:
 *
 * Implement the "PostProcessKeyEvent" get property of the
 * org.freedesktop.IBus.InputContext interface because currently the Gio
 * D-Bus method calls don't support multiple nested tuples likes
 * G_VARIANT_TYPE ("((ba(yv)))")) in "ProcessKeyEvent" D-Bus method
 * So these post events are separated from the return value "b" of
 * the "ProcessKeyEvent" D-Bus method call.
 */
static GVariant *
_ic_get_post_process_key_event (BusInputContext *context,
                                GDBusConnection *connection,
                                GError         **error)
{
    const char *error_message = NULL;
    GVariantBuilder array;
    SyncForwardingData *data;

    do {
        if (!BUS_IS_INPUT_CONTEXT (context)) {
            error_message = "BusInputContext is freed";
            break;
        }
        if (context->processing_key_event) {
            error_message = "Another ProcessKeyEvent is called.";
            break;
        }
        g_variant_builder_init (&array, G_VARIANT_TYPE ("a(yv)"));
        while ((data =
                g_queue_pop_head (context->queue_during_process_key_event))) {
            GVariant *variant = ibus_serializable_serialize_object (
                    IBUS_SERIALIZABLE (data->text));
            g_variant_builder_add (&array, "(yv)", data->key, variant);
            g_object_unref (data->text);
            g_slice_free (SyncForwardingData, data);
        }
    } while (FALSE);
    if (error_message) {
        g_set_error (error,
                     G_DBUS_ERROR,
                     G_DBUS_ERROR_FAILED,
                     "%s", error_message);
        return NULL;
    }
    return g_variant_builder_end (&array);
}

static GVariant *
bus_input_context_service_get_property (IBusService           *service,
                                        GDBusConnection       *connection,
                                        const gchar           *sender,
                                        const gchar           *object_path,
                                        const gchar           *interface_name,
                                        const gchar           *property_name,
                                        GError               **error)
{
    int i;
    static const struct {
        const char *property_name;
        GVariant * (* property_callback) (BusInputContext *,
                                          GDBusConnection *,
                                          GError **);
    } properties [] =  {
        { "PostProcessKeyEvent",   _ic_get_post_process_key_event },
    };

    if (error)
        *error = NULL;
    if (g_strcmp0 (interface_name, IBUS_INTERFACE_INPUT_CONTEXT) != 0) {
        return IBUS_SERVICE_CLASS (bus_input_context_parent_class)->
                service_get_property (
                        service, connection, sender, object_path,
                        interface_name, property_name,
                        error);
    }
    for (i = 0; i < G_N_ELEMENTS (properties); i++) {
        if (g_strcmp0 (properties[i].property_name, property_name) == 0) {
            return properties[i].property_callback ((BusInputContext *)service,
                                                    connection,
                                                    error);
        }
    }

    g_set_error (error,
                 G_DBUS_ERROR,
                 G_DBUS_ERROR_FAILED,
                 "service_get_property received an unknown property: %s",
                 property_name ? property_name : "(null)");
    g_return_val_if_reached (NULL);
}

static gboolean
_ic_set_content_type (BusInputContext *context,
                      GVariant        *value,
                      GError         **error)
{
    guint purpose = 0;
    guint hints = 0;
    gboolean retval = TRUE;

    g_variant_get (value, "(uu)", &purpose, &hints);
    if (purpose != context->purpose || hints != context->hints) {

        context->purpose = purpose;
        context->hints = hints;

        if (context->has_focus && context->engine)
            bus_engine_proxy_set_content_type (context->engine,
                                               purpose,
                                               hints);

        if (context->has_focus) {
            g_signal_emit (context,
                           context_signals[SET_CONTENT_TYPE],
                           0,
                           context->purpose,
                           context->hints);
        }

        retval = bus_input_context_property_changed (context,
                                                     "ContentType",
                                                     value,
                                                     error);
    }
    return retval;
}

static gboolean
_ic_set_client_commit_preedit (BusInputContext *context,
                               GVariant        *value,
                               GError         **error)
{
    g_variant_get (value, "(b)", &context->client_commit_preedit);
    return TRUE;
}

static gboolean
_ic_set_use_post_process_key_event (BusInputContext *context,
                                    GVariant        *value,
                                    GError         **error)
{
    g_variant_get (value, "(b)", &context->use_post_process_key_event);
    return TRUE;
}

static gboolean
bus_input_context_service_set_property (IBusService     *service,
                                        GDBusConnection *connection,
                                        const gchar     *sender,
                                        const gchar     *object_path,
                                        const gchar     *interface_name,
                                        const gchar     *property_name,
                                        GVariant        *value,
                                        GError         **error)
{
    int i;
    static const struct {
        const char *property_name;
        gboolean (* property_callback) (BusInputContext *,
                                        GVariant *,
                                        GError **);
    } properties [] =  {
        { "ContentType",                   _ic_set_content_type },
        { "ClientCommitPreedit",           _ic_set_client_commit_preedit },
        { "EffectivePostProcessKeyEvent",  _ic_set_use_post_process_key_event },
    };

    if (error)
        *error = NULL;
    if (g_strcmp0 (interface_name, IBUS_INTERFACE_INPUT_CONTEXT) != 0) {
        return IBUS_SERVICE_CLASS (bus_input_context_parent_class)->
            service_set_property (service,
                                  connection,
                                  sender,
                                  object_path,
                                  interface_name,
                                  property_name,
                                  value,
                                  error);
    }

    if (!BUS_IS_INPUT_CONTEXT (service)) {
        g_set_error (error,
                     G_DBUS_ERROR,
                     G_DBUS_ERROR_FAILED,
                     "%p is not BusInputContext.", service);
        return FALSE;
    }
    if (!bus_input_context_service_authorized_method (service, connection)) {
        /* No error message due to the security issue but GError is required
         * by gdbusconnection.c:invoke_set_property_in_idle_cb() */
        g_set_error (error,
                     G_DBUS_ERROR,
                     G_DBUS_ERROR_FAILED,
                     " ");
        return FALSE;
    }
    for (i = 0; i < G_N_ELEMENTS (properties); i++) {
        if (g_strcmp0 (properties[i].property_name, property_name) == 0) {
            return properties[i].property_callback ((BusInputContext *) service,
                                                    value,
                                                    error);
        }
    }

    g_set_error (error,
                 G_DBUS_ERROR,
                 G_DBUS_ERROR_FAILED,
                 "service_set_property received an unknown property: %s",
                 property_name ? property_name : "(null)");
    g_return_val_if_reached (FALSE);
}


static gboolean
bus_input_context_make_post_process_key_event (BusInputContext       *context,
                                               SyncForwardingPreData *pre_data)
{
    SyncForwardingData *data;
    if (context->processing_key_event && g_queue_get_length (
            context->queue_during_process_key_event) <= MAX_SYNC_DATA) {
        if (g_queue_get_length (context->queue_during_process_key_event)
            == MAX_SYNC_DATA) {
            g_warning ("Exceed max number of post process_key_event data");
        }
        data = g_slice_new (SyncForwardingData);
        data->key = pre_data->key;
        switch (pre_data->key) {
        case 'c':
            data->text = g_object_ref (pre_data->text);
            break;
        case 'd':
            data->text = ibus_text_new_from_printf (
                    "%d,%u",
                    pre_data->u.deleting.offset,
                    pre_data->u.deleting.nchars);
            break;
        case 'f':
            data->text = ibus_text_new_from_printf ("%u,%u,%u",
                                                    pre_data->u.uints[0],
                                                    pre_data->u.uints[1],
                                                    pre_data->u.uints[2]);
            break;
        case 'h':
        case 'r':
        case 's':
            data->text = ibus_text_new_from_static_string ("");
            break;
        case 'u':
        case 'm':
            data->text = g_object_ref (pre_data->text);
            g_queue_push_tail (context->queue_during_process_key_event, data);
            data = g_slice_new (SyncForwardingData);
            data->key = pre_data->key;
            if (pre_data->key == 'u') {
                data->text = ibus_text_new_from_printf (
                        "%u,%u",
                        pre_data->u.uints[0],
                        pre_data->u.uints[1]);
            } else {
                data->text = ibus_text_new_from_printf (
                        "%u,%u,%u",
                        pre_data->u.uints[0],
                        pre_data->u.uints[1],
                        pre_data->u.uints[2]);
            }
            break;
        default:
            g_warning ("Type %c of SyncForwardingData is not supported",
                       pre_data->key);
            g_slice_free (SyncForwardingData, data);
            return FALSE;
        }
        g_queue_push_tail (context->queue_during_process_key_event, data);
        return TRUE;
    }
    return FALSE;
}

gboolean
bus_input_context_has_focus (BusInputContext *context)
{
    g_assert (BUS_IS_INPUT_CONTEXT (context));

    return context->has_focus;
}

void
bus_input_context_focus_in (BusInputContext *context)
{
    g_assert (BUS_IS_INPUT_CONTEXT (context));

    if (context->has_focus)
        return;

    context->has_focus = TRUE;

    /* To make sure that we won't use an old value left before we losing focus
     * last time. */
    context->prev_keyval = IBUS_KEY_VoidSymbol;
    context->prev_modifiers = 0;

    if (context->engine) {
        const gchar *path =
                ibus_service_get_object_path ((IBusService *)context);
        bus_engine_proxy_focus_in (context->engine, path, context->client);
        bus_engine_proxy_enable (context->engine);
        bus_engine_proxy_set_capabilities (context->engine,
                                           context->capabilities);
        bus_engine_proxy_set_cursor_location (context->engine,
                                              context->x,
                                              context->y,
                                              context->w,
                                              context->h);
        bus_engine_proxy_set_content_type (context->engine,
                                           context->purpose,
                                           context->hints);
    }

    if (context->capabilities & IBUS_CAP_FOCUS) {
        g_signal_emit (context, context_signals[FOCUS_IN], 0);
        if (context->engine) {
            /* if necessary, emit glib signals to the context object to update
             * panel status. see the comment for PREEDIT_CONDITION
             * for details. */
            if (context->preedit_visible && !PREEDIT_CONDITION) {
                g_signal_emit (context,
                               context_signals[UPDATE_PREEDIT_TEXT],
                               0,
                               context->preedit_text,
                               context->preedit_cursor_pos,
                               context->preedit_visible);
            }
            if (context->auxiliary_visible &&
                (context->capabilities & IBUS_CAP_AUXILIARY_TEXT) == 0) {
                g_signal_emit (context,
                               context_signals[UPDATE_AUXILIARY_TEXT],
                               0,
                               context->auxiliary_text,
                               context->auxiliary_visible);
            }
            if (context->lookup_table_visible &&
                (context->capabilities & IBUS_CAP_LOOKUP_TABLE) == 0) {
                g_signal_emit (context,
                               context_signals[UPDATE_LOOKUP_TABLE],
                               0,
                               context->lookup_table,
                               context->lookup_table_visible);
            }
        }
    }
}

/**
 * bus_input_context_clear_preedit_text:
 * @context: A #BusInputContext
 * @with_signal: %FALSE if the preedit is already updated in ibus clients
 *               likes ibus-im.so. Otherwise %TRUE.
 *
 * Clear context->preedit_text. If the preedit mode is
 * IBUS_ENGINE_PREEDIT_COMMIT, commit it before clearing.
 */
void
bus_input_context_clear_preedit_text (BusInputContext *context,
                                      gboolean         with_signal)
{
    IBusText *preedit_text;
    guint     preedit_mode;
    gboolean  preedit_visible;

    g_assert (BUS_IS_INPUT_CONTEXT (context));

    if (!with_signal) {
        g_object_unref (context->preedit_text);
        context->preedit_mode = IBUS_ENGINE_PREEDIT_CLEAR;
        context->preedit_text = (IBusText *) g_object_ref_sink (text_empty);
        context->preedit_cursor_pos = 0;
        context->preedit_visible = FALSE;
        return;
    }

    /* always clear preedit text to reset the cursor position in the
     * client application before commit the preeit text. */
    preedit_text = g_object_ref (context->preedit_text);
    preedit_mode = context->preedit_mode;
    preedit_visible = context->preedit_visible;
    bus_input_context_update_preedit_text (context,
        text_empty, 0, FALSE, IBUS_ENGINE_PREEDIT_CLEAR, TRUE);

    if (preedit_visible && preedit_mode == IBUS_ENGINE_PREEDIT_COMMIT) {
        bus_input_context_commit_text (context, preedit_text);
    }
    g_object_unref (preedit_text);
}

void
bus_input_context_focus_out (BusInputContext *context)
{
    g_assert (BUS_IS_INPUT_CONTEXT (context));

    if (!context->has_focus)
        return;

    if (context->client_commit_preedit)
        bus_input_context_clear_preedit_text (context, FALSE);
    else
        bus_input_context_clear_preedit_text (context, TRUE);
    bus_input_context_update_auxiliary_text (context, text_empty, FALSE);
    bus_input_context_update_lookup_table (context,
                                           lookup_table_empty,
                                           FALSE,
                                           FALSE);
    bus_input_context_register_properties (context, props_empty);

    if (context->engine) {
        const gchar *path =
            ibus_service_get_object_path ((IBusService *)context);
        bus_engine_proxy_focus_out (context->engine, path);
    }

    context->has_focus = FALSE;

    if (context->capabilities & IBUS_CAP_FOCUS) {
        g_signal_emit (context, context_signals[FOCUS_OUT], 0);
    }
}

#define DEFINE_FUNC(name)                                                   \
    void                                                                    \
    bus_input_context_##name (BusInputContext *context)                     \
    {                                                                       \
        g_assert (BUS_IS_INPUT_CONTEXT (context));                          \
                                                                            \
        if (context->is_extension_lookup_table &&                           \
            context->emoji_extension) {                                     \
            bus_panel_proxy_##name##_lookup_table (context->emoji_extension); \
            return;                                                         \
        }                                                                   \
        if (context->has_focus && context->engine) {                        \
            bus_engine_proxy_##name (context->engine);                      \
        }                                                                   \
    }

DEFINE_FUNC (page_up)
DEFINE_FUNC (page_down)
DEFINE_FUNC (cursor_up)
DEFINE_FUNC (cursor_down)

#undef DEFINE_FUNC

void
bus_input_context_candidate_clicked (BusInputContext *context,
                                     guint            index,
                                     guint            button,
                                     guint            state)
{
    g_assert (BUS_IS_INPUT_CONTEXT (context));

    if (context->is_extension_lookup_table && context->emoji_extension) {
        bus_panel_proxy_candidate_clicked_lookup_table (
                context->emoji_extension,
                index,
                button,
                state);
            return;
    }
    if (context->engine) {
        bus_engine_proxy_candidate_clicked (context->engine,
                                            index,
                                            button,
                                            state);
    }
}

void
bus_input_context_property_activate (BusInputContext *context,
                                     const gchar     *prop_name,
                                     gint             prop_state)
{
    g_assert (BUS_IS_INPUT_CONTEXT (context));

    if (context->engine) {
        bus_engine_proxy_property_activate (context->engine,
                                            prop_name,
                                            prop_state);
    }
}

/**
 * bus_input_context_show_preedit_text:
 *
 * Show a preedit text. Send D-Bus signal to update status of client or send
 * glib signal to the panel, depending on capabilities of the client.
 */
static void
bus_input_context_show_preedit_text (BusInputContext *context,
                                     gboolean         is_extension)
{
    g_assert (BUS_IS_INPUT_CONTEXT (context));

    if (context->preedit_visible)
        return;
    if (!is_extension && context->emoji_extension)
        return;

    if (!is_extension)
        context->preedit_visible = TRUE;

    if (context->emoji_extension && !is_extension) {
        /* Do not use HIDE_PREEDIT_TEXT signal below but call
         * bus_panel_proxy_hide_preedit_text() directly for the extension only
         * but not for the normal panel.
         */
        bus_panel_proxy_show_preedit_text (context->emoji_extension);
        return;
    }

    if (PREEDIT_CONDITION) {
        SyncForwardingPreData pre_data = { 's', };
        if (bus_input_context_make_post_process_key_event (context, &pre_data))
            return;
        bus_input_context_emit_signal (context,
                                       "ShowPreeditText",
                                       NULL,
                                       NULL);
    }
    else {
        g_signal_emit (context,
                       context_signals[SHOW_PREEDIT_TEXT],
                       0);
    }
}

/**
 * bus_input_context_hide_preedit_text:
 *
 * Hide a preedit text. Send D-Bus signal to update status of client or send
 * glib signal to the panel, depending on capabilities of the client.
 */
static void
bus_input_context_hide_preedit_text (BusInputContext *context,
                                     gboolean         is_extension)
{
    g_assert (BUS_IS_INPUT_CONTEXT (context));

    if (!is_extension && !context->preedit_visible)
        return;

    if (!is_extension)
        context->preedit_visible = FALSE;

    if (context->emoji_extension && !is_extension) {
        /* Do not use HIDE_PREEDIT_TEXT signal below but call
         * bus_panel_proxy_hide_preedit_text() directly for the extension only
         * but not for the normal panel.
         */
        bus_panel_proxy_hide_preedit_text (context->emoji_extension);
        return;
    }

    if (PREEDIT_CONDITION) {
        SyncForwardingPreData pre_data = { 'h', };
        if (bus_input_context_make_post_process_key_event (context, &pre_data))
            return;
        bus_input_context_emit_signal (context,
                                       "HidePreeditText",
                                       NULL,
                                       NULL);
    }
    else {
        g_signal_emit (context,
                       context_signals[HIDE_PREEDIT_TEXT],
                       0);
    }
}

/**
 * bus_input_context_update_auxiliary_text:
 *
 * Update an aux text. Send D-Bus signal to update status of client or send
 * glib signal to the panel, depending on capabilities of the client.
 */
static void
bus_input_context_update_auxiliary_text (BusInputContext *context,
                                         IBusText        *text,
                                         gboolean         visible)
{
    g_assert (BUS_IS_INPUT_CONTEXT (context));

    if (context->auxiliary_text) {
        g_object_unref (context->auxiliary_text);
    }

    context->auxiliary_text = (IBusText *)g_object_ref_sink (
            text ? text : text_empty);
    context->auxiliary_visible = visible;

    if (context->capabilities & IBUS_CAP_AUXILIARY_TEXT) {
        GVariant *variant =
                ibus_serializable_serialize ((IBusSerializable *)text);
        bus_input_context_emit_signal (context,
                                       "UpdateAuxiliaryText",
                                       g_variant_new ("(vb)", variant, visible),
                                       NULL);
    }
    else {
        g_signal_emit (context,
                       context_signals[UPDATE_AUXILIARY_TEXT],
                       0,
                       context->auxiliary_text,
                       context->auxiliary_visible);
    }
}

/**
 * bus_input_context_show_auxiliary_text:
 *
 * Show an aux text. Send D-Bus signal to update status of client or send glib
 * signal to the panel, depending on capabilities of the client.
 */
static void
bus_input_context_show_auxiliary_text (BusInputContext *context)
{
    g_assert (BUS_IS_INPUT_CONTEXT (context));

    if (context->auxiliary_visible) {
        return;
    }

    context->auxiliary_visible = TRUE;

    if ((context->capabilities & IBUS_CAP_AUXILIARY_TEXT)
        == IBUS_CAP_AUXILIARY_TEXT) {
        bus_input_context_emit_signal (context,
                                       "ShowAuxiliaryText",
                                       NULL,
                                       NULL);
    }
    else {
        g_signal_emit (context,
                       context_signals[SHOW_AUXILIARY_TEXT],
                       0);
    }
}

/**
 * bus_input_context_hide_auxiliary_text:
 *
 * Hide an aux text. Send D-Bus signal to update status of client or send glib
 * signal to the panel, depending on capabilities of the client.
 */
static void
bus_input_context_hide_auxiliary_text (BusInputContext *context)
{
    g_assert (BUS_IS_INPUT_CONTEXT (context));

    if (!context->auxiliary_visible) {
        return;
    }

    context->auxiliary_visible = FALSE;

    if ((context->capabilities & IBUS_CAP_AUXILIARY_TEXT)
        == IBUS_CAP_AUXILIARY_TEXT) {
        bus_input_context_emit_signal (context,
                                       "HideAuxiliaryText",
                                       NULL,
                                       NULL);
    }
    else {
        g_signal_emit (context,
                       context_signals[HIDE_AUXILIARY_TEXT],
                       0);
    }
}

/**
 * bus_input_context_update_lookup_table:
 * @context: #BusInputContext
 * @table: #IBusLookupTable
 * @visible: %TRUE if the lookup table is visible, otherwise %FALSE.
 * @is_extension: %TRUE if the lookup table is called by a panel extension.
 *                %FALSE if it's called by an engine.
 * I.e. is_extension_lookup_table means the owner of the lookup table.
 */
void
bus_input_context_update_lookup_table (BusInputContext *context,
                                       IBusLookupTable *table,
                                       gboolean         visible,
                                       gboolean         is_extension)
{
    g_assert (BUS_IS_INPUT_CONTEXT (context));

    context->is_extension_lookup_table = is_extension;
    if (context->lookup_table) {
        g_object_unref (context->lookup_table);
    }

    context->lookup_table = (IBusLookupTable *)g_object_ref_sink (
            table ? table : lookup_table_empty);
    context->lookup_table_visible = visible;
    /* If not PREEDIT_CONDITION, ignore_focus_out flag is already evaluated in
     * bus_input_context_update_preedit_text() because UpdatePreeditText
     * D-Bus method is always sent to the IBus panel in xterm before
     * UpdateLookupTable D-BUs method is sent to the panel.
     */
    if (PREEDIT_CONDITION)
        context->ignore_focus_out = FALSE;

    if (context->capabilities & IBUS_CAP_LOOKUP_TABLE) {
        GVariant *variant =
                ibus_serializable_serialize ((IBusSerializable *)table);
        bus_input_context_emit_signal (context,
                                       "UpdateLookupTable",
                                       g_variant_new ("(vb)", variant, visible),
                                       NULL);
    }
    else {
        if (IGNORE_FOCUS_OUT_CONDITION)
            context->ignore_focus_out = TRUE;
        g_signal_emit (context,
                       context_signals[UPDATE_LOOKUP_TABLE],
                       0,
                       context->lookup_table,
                       context->lookup_table_visible);
    }
}

/**
 * bus_input_context_show_lookup_table:
 *
 * Show the lookup table. Send D-Bus signal to update status of client or send
 * glib signal to the panel, depending on capabilities of the client.
 */
static void
bus_input_context_show_lookup_table (BusInputContext *context)
{
    g_assert (BUS_IS_INPUT_CONTEXT (context));

    if (context->lookup_table_visible) {
        return;
    }

    context->lookup_table_visible = TRUE;

    if ((context->capabilities & IBUS_CAP_LOOKUP_TABLE)
        == IBUS_CAP_LOOKUP_TABLE) {
        bus_input_context_emit_signal (context,
                                       "ShowLookupTable",
                                       NULL,
                                       NULL);
    }
    else {
        g_signal_emit (context,
                       context_signals[SHOW_LOOKUP_TABLE],
                       0);
    }
}

/**
 * bus_input_context_hide_lookup_table:
 *
 * Hide the lookup table. Send D-Bus signal to update status of client or send
 * glib signal to the panel, depending on capabilities of the client.
 */
static void
bus_input_context_hide_lookup_table (BusInputContext *context)
{
    g_assert (BUS_IS_INPUT_CONTEXT (context));

    if (!context->lookup_table_visible) {
        return;
    }

    context->lookup_table_visible = FALSE;

    if ((context->capabilities & IBUS_CAP_LOOKUP_TABLE)
        == IBUS_CAP_LOOKUP_TABLE) {
        bus_input_context_emit_signal (context,
                                       "HideLookupTable",
                                       NULL,
                                       NULL);
    }
    else {
        g_signal_emit (context,
                       context_signals[HIDE_LOOKUP_TABLE],
                       0);
    }
}

/**
 * bus_input_context_page_up_lookup_table:
 *
 * Change cursor position. Send D-Bus signal to update status of client or send
 * glib signal to the panel, depending on capabilities of the client.
 */
static void
bus_input_context_page_up_lookup_table (BusInputContext *context)
{
    g_assert (BUS_IS_INPUT_CONTEXT (context));

    if (!ibus_lookup_table_page_up (context->lookup_table)) {
        return;
    }

    if ((context->capabilities & IBUS_CAP_LOOKUP_TABLE)
        == IBUS_CAP_LOOKUP_TABLE) {
        bus_input_context_emit_signal (context,
                                       "PageUpLookupTable",
                                       NULL,
                                       NULL);
    }
    else {
        g_signal_emit (context,
                       context_signals[PAGE_UP_LOOKUP_TABLE],
                       0);
    }
}

/**
 * bus_input_context_page_down_lookup_table:
 *
 * Change cursor position. Send D-Bus signal to update status of client or send
 * glib signal to the panel, depending on capabilities of the client.
 */
static void
bus_input_context_page_down_lookup_table (BusInputContext *context)
{
    g_assert (BUS_IS_INPUT_CONTEXT (context));

    if (!ibus_lookup_table_page_down (context->lookup_table)) {
        return;
    }

    if ((context->capabilities & IBUS_CAP_LOOKUP_TABLE)
        == IBUS_CAP_LOOKUP_TABLE) {
        bus_input_context_emit_signal (context,
                                       "PageDownLookupTable",
                                       NULL,
                                       NULL);
    }
    else {
        g_signal_emit (context,
                       context_signals[PAGE_DOWN_LOOKUP_TABLE],
                       0);
    }
}

/**
 * bus_input_context_cursor_up_lookup_table:
 *
 * Change cursor position. Send D-Bus signal to update status of client or send
 * glib signal to the panel, depending on capabilities of the client.
 */
static void
bus_input_context_cursor_up_lookup_table (BusInputContext *context)
{
    g_assert (BUS_IS_INPUT_CONTEXT (context));

    if (!ibus_lookup_table_cursor_up (context->lookup_table)) {
        return;
    }

    if ((context->capabilities & IBUS_CAP_LOOKUP_TABLE)
        == IBUS_CAP_LOOKUP_TABLE) {
        bus_input_context_emit_signal (context,
                                       "CursorUpLookupTable",
                                       NULL,
                                       NULL);
    }
    else {
        g_signal_emit (context,
                       context_signals[CURSOR_UP_LOOKUP_TABLE],
                       0);
    }
}

/**
 * bus_input_context_cursor_down_lookup_table:
 *
 * Change cursor position. Send D-Bus signal to update status of client or send
 * glib signal to the panel, depending on capabilities of the client.
 */
static void
bus_input_context_cursor_down_lookup_table (BusInputContext *context)
{
    g_assert (BUS_IS_INPUT_CONTEXT (context));

    if (!ibus_lookup_table_cursor_down (context->lookup_table)) {
        return;
    }

    if ((context->capabilities & IBUS_CAP_LOOKUP_TABLE)
        == IBUS_CAP_LOOKUP_TABLE) {
        bus_input_context_emit_signal (context,
                                       "CursorDownLookupTable",
                                       NULL,
                                       NULL);
    }
    else {
        g_signal_emit (context,
                       context_signals[CURSOR_DOWN_LOOKUP_TABLE],
                       0);
    }
}

/**
 * bus_input_context_register_properties:
 *
 * Register properties. Send D-Bus signal to update status of client or send
 * glib signal to the panel, depending on capabilities of the client.
 */
static void
bus_input_context_register_properties (BusInputContext *context,
                                       IBusPropList    *props)
{
    g_assert (BUS_IS_INPUT_CONTEXT (context));
    g_assert (IBUS_IS_PROP_LIST (props));

    if (context->capabilities & IBUS_CAP_PROPERTY) {
        GVariant *variant =
                ibus_serializable_serialize ((IBusSerializable *)props);
        bus_input_context_emit_signal (context,
                                       "RegisterProperties",
                                       g_variant_new ("(v)", variant),
                                       NULL);
    }
    else {
        g_signal_emit (context,
                       context_signals[REGISTER_PROPERTIES],
                       0,
                       props);
    }
}

/**
 * bus_input_context_update_property:
 *
 * Update property. Send D-Bus signal to update status of client or send glib
 * signal to the panel, depending on capabilities of the client.
 */
static void
bus_input_context_update_property (BusInputContext *context,
                                   IBusProperty    *prop)
{
    g_assert (BUS_IS_INPUT_CONTEXT (context));
    g_assert (IBUS_IS_PROPERTY (prop));

    if (context->capabilities & IBUS_CAP_PROPERTY) {
        GVariant *variant =
                ibus_serializable_serialize ((IBusSerializable *)prop);
        bus_input_context_emit_signal (context,
                                       "UpdateProperty",
                                       g_variant_new ("(v)", variant),
                                       NULL);
    }
    else {
        g_signal_emit (context,
                       context_signals[UPDATE_PROPERTY],
                       0,
                       prop);
    }
}

/**
 * _engine_destroy_cb:
 *
 * A function to be called when "destroy" glib signal is sent to the engine object.
 * Remove the engine from the context.
 */
static void
_engine_destroy_cb (BusEngineProxy  *engine,
                    BusInputContext *context)
{
    g_assert (BUS_IS_ENGINE_PROXY (engine));
    g_assert (BUS_IS_INPUT_CONTEXT (context));

    g_assert (context->engine == engine);

    bus_input_context_set_engine (context, NULL);
}

/**
 * _engine_commit_text_cb:
 *
 * A function to be called when "commit-text" glib signal is sent to the engine
 * object.
 */
static void
_engine_commit_text_cb (BusEngineProxy  *engine,
                        IBusText        *text,
                        BusInputContext *context)
{
    g_assert (BUS_IS_ENGINE_PROXY (engine));
    g_assert (text != NULL);
    g_assert (BUS_IS_INPUT_CONTEXT (context));

    g_assert (context->engine == engine);

    bus_input_context_commit_text (context, text);
}

/**
 * _engine_forward_key_event_cb:
 *
 * A function to be called when "forward-key-event" glib signal is sent to the
 * engine object.
 */
static void
_engine_forward_key_event_cb (BusEngineProxy    *engine,
                              guint              keyval,
                              guint              keycode,
                              guint              state,
                              BusInputContext   *context)
{
    SyncForwardingPreData pre_data = { 'f', };

    g_assert (BUS_IS_ENGINE_PROXY (engine));
    g_assert (BUS_IS_INPUT_CONTEXT (context));
    g_assert (context->engine == engine);
    g_assert (context->queue_during_process_key_event);

    pre_data.u.uints[0] = keyval;
    pre_data.u.uints[1] = keycode;
    pre_data.u.uints[2] = state;
    if (bus_input_context_make_post_process_key_event (context, &pre_data))
        return;
    bus_input_context_emit_signal (context,
                                   "ForwardKeyEvent",
                                   g_variant_new ("(uuu)",
                                                  keyval, keycode, state),
                                   NULL);
}

/**
 * _engine_delete_surrounding_text_cb:
 *
 * A function to be called when "delete-surrounding-text" glib signal is sent
 * to the engine object.
 */
static void
_engine_delete_surrounding_text_cb (BusEngineProxy    *engine,
                                    gint               offset_from_cursor,
                                    guint              nchars,
                                    BusInputContext   *context)
{
    SyncForwardingPreData pre_data = { 'd', };

    g_assert (BUS_IS_ENGINE_PROXY (engine));
    g_assert (BUS_IS_INPUT_CONTEXT (context));
    g_assert (context->engine == engine);

    pre_data.u.deleting.offset = offset_from_cursor;
    pre_data.u.deleting.nchars = nchars;
    if (bus_input_context_make_post_process_key_event (context, &pre_data))
        return;
    bus_input_context_emit_signal (context,
                                   "DeleteSurroundingText",
                                   g_variant_new ("(iu)",
                                                  offset_from_cursor, nchars),
                                   NULL);
}

/**
 * _engine_require_surrounding_text_cb:
 *
 * A function to be called when "require-surrounding-text" glib signal is sent
 * to the engine object.
 */
static void
_engine_require_surrounding_text_cb (BusEngineProxy    *engine,
                                     BusInputContext   *context)
{
    SyncForwardingPreData pre_data = { 'r', };

    g_assert (BUS_IS_ENGINE_PROXY (engine));
    g_assert (BUS_IS_INPUT_CONTEXT (context));
    g_assert (context->engine == engine);

    if (bus_input_context_make_post_process_key_event (context, &pre_data))
        return;
    bus_input_context_emit_signal (context,
                                   "RequireSurroundingText",
                                   NULL,
                                   NULL);
}

/**
 * _engine_update_preedit_text_cb:
 *
 * A function to be called when "update-preedit-text" glib signal is sent to
 * the engine object.
 */
static void
_engine_update_preedit_text_cb (BusEngineProxy  *engine,
                                IBusText        *text,
                                guint            cursor_pos,
                                gboolean         visible,
                                guint            mode,
                                BusInputContext *context)
{
    g_assert (BUS_IS_ENGINE_PROXY (engine));
    g_assert (IBUS_IS_TEXT (text));
    g_assert (BUS_IS_INPUT_CONTEXT (context));

    g_assert (context->engine == engine);

    bus_input_context_update_preedit_text (context, text,
                                           cursor_pos, visible, mode,
                                           TRUE);
}

/**
 * _engine_update_auxiliary_text_cb:
 *
 * A function to be called when "update-auxiliary-text" glib signal is sent to
 * the engine object.
 */
static void
_engine_update_auxiliary_text_cb (BusEngineProxy   *engine,
                                  IBusText         *text,
                                  gboolean          visible,
                                  BusInputContext  *context)
{
    g_assert (BUS_IS_ENGINE_PROXY (engine));
    g_assert (IBUS_IS_TEXT (text));
    g_assert (BUS_IS_INPUT_CONTEXT (context));

    g_assert (context->engine == engine);

    bus_input_context_update_auxiliary_text (context, text, visible);
}

/**
 * _engine_update_lookup_table_cb:
 *
 * A function to be called when "update-lookup-table" glib signal is sent to
 * the engine object.
 */
static void
_engine_update_lookup_table_cb (BusEngineProxy   *engine,
                                IBusLookupTable  *table,
                                gboolean          visible,
                                BusInputContext  *context)
{
    g_assert (BUS_IS_ENGINE_PROXY (engine));
    g_assert (IBUS_IS_LOOKUP_TABLE (table));
    g_assert (BUS_IS_INPUT_CONTEXT (context));

    g_assert (context->engine == engine);

    bus_input_context_update_lookup_table (context, table, visible, FALSE);
}

/**
 * _engine_register_properties_cb:
 *
 * A function to be called when "register-properties" glib signal is sent to
 * the engine object.
 */
static void
_engine_register_properties_cb (BusEngineProxy  *engine,
                                IBusPropList    *props,
                                BusInputContext *context)
{
    g_assert (BUS_IS_ENGINE_PROXY (engine));
    g_assert (IBUS_IS_PROP_LIST (props));
    g_assert (BUS_IS_INPUT_CONTEXT (context));

    g_assert (context->engine == engine);

    bus_input_context_register_properties (context, props);
}

/**
 * _engine_update_property_cb:
 *
 * A function to be called when "update-property" glib signal is sent to the
 * engine object.
 */
static void
_engine_update_property_cb (BusEngineProxy  *engine,
                            IBusProperty    *prop,
                            BusInputContext *context)
{
    g_assert (BUS_IS_ENGINE_PROXY (engine));
    g_assert (IBUS_IS_PROPERTY (prop));
    g_assert (BUS_IS_INPUT_CONTEXT (context));

    g_assert (context->engine == engine);

    bus_input_context_update_property (context, prop);
}

/**
 * _engine_panel_extension_cb:
 *
 * A function to be called when "panel-extension" glib signal is sent
 * from the engine object.
 */
static void
_engine_panel_extension_cb (BusEngineProxy     *engine,
                            IBusExtensionEvent *event,
                            BusInputContext    *context)
{
    g_signal_emit (context, context_signals[PANEL_EXTENSION], 0, event);
}

static void
_engine_show_preedit_text_cb (BusEngineProxy  *engine,
                              BusInputContext *context)
{
    g_assert (BUS_IS_ENGINE_PROXY (engine));
    g_assert (BUS_IS_INPUT_CONTEXT (context));

    g_assert (context->engine == engine);

    bus_input_context_show_preedit_text (context, FALSE);
}

static void
_engine_hide_preedit_text_cb (BusEngineProxy  *engine,
                              BusInputContext *context)
{
    g_assert (BUS_IS_ENGINE_PROXY (engine));
    g_assert (BUS_IS_INPUT_CONTEXT (context));

    g_assert (context->engine == engine);

    bus_input_context_hide_preedit_text (context, FALSE);
}

#define DEFINE_FUNCTION(name)                                   \
    static void                                                 \
    _engine_##name##_cb (BusEngineProxy   *engine,              \
                         BusInputContext *context)              \
    {                                                           \
        g_assert (BUS_IS_ENGINE_PROXY (engine));                \
        g_assert (BUS_IS_INPUT_CONTEXT (context));              \
                                                                \
        g_assert (context->engine == engine);                   \
                                                                \
        bus_input_context_##name (context);                     \
    }

DEFINE_FUNCTION (show_auxiliary_text)
DEFINE_FUNCTION (hide_auxiliary_text)
DEFINE_FUNCTION (show_lookup_table)
DEFINE_FUNCTION (hide_lookup_table)
DEFINE_FUNCTION (page_up_lookup_table)
DEFINE_FUNCTION (page_down_lookup_table)
DEFINE_FUNCTION (cursor_up_lookup_table)
DEFINE_FUNCTION (cursor_down_lookup_table)
#undef DEFINE_FUNCTION

BusInputContext *
bus_input_context_new (BusConnection    *connection,
                       const gchar      *client)
{
    static guint id = 0;

    g_assert (connection == NULL || BUS_IS_CONNECTION (connection));
    g_assert (client != NULL);

    gchar *path = g_strdup_printf (IBUS_PATH_INPUT_CONTEXT, ++id);

    BusInputContext *context = NULL;
    if (connection) {
        context = (BusInputContext *) g_object_new (
                BUS_TYPE_INPUT_CONTEXT,
                "object-path", path,
                "connection", bus_connection_get_dbus_connection (connection),
                NULL);
    }
    else {
        context = (BusInputContext *) g_object_new (BUS_TYPE_INPUT_CONTEXT,
                                                    "object-path", path,
                                                    NULL);
    }
    g_free (path);

    context->client = g_strdup (client);

    /* it is a fake input context, just need process hotkey */
    context->fake = (strncmp (client, "fake", 4) == 0);
    context->queue_during_process_key_event = g_queue_new ();

    if (connection) {
        g_object_ref_sink (connection);
        context->connection = connection;
        g_signal_connect (context->connection,
                          "destroy",
                          (GCallback) _connection_destroy_cb,
                          context);
    }

    return context;
}

void
bus_input_context_enable (BusInputContext *context)
{
    g_assert (BUS_IS_INPUT_CONTEXT (context));

    if (!context->has_focus) {
        return;
    }

    if (context->engine == NULL) {
        IBusEngineDesc *desc = NULL;
        g_signal_emit (context,
                       context_signals[REQUEST_ENGINE], 0,
                       NULL,
                       &desc);
        if (desc != NULL) {
            bus_input_context_set_engine_by_desc (context,
                            desc,
                            g_gdbus_timeout, /* timeout in msec. */
                            NULL, /* we do not cancel the call. */
                            NULL, /* use the default callback function. */
                            NULL);
        }
    }

    if (context->engine == NULL)
        return;

    {
        const gchar *path =
                ibus_service_get_object_path ((IBusService *)context);
        bus_engine_proxy_focus_in (context->engine, path, context->client);
        bus_engine_proxy_enable (context->engine);
        bus_engine_proxy_set_capabilities (context->engine,
                                           context->capabilities);
        bus_engine_proxy_set_cursor_location (context->engine,
                                              context->x, context->y,
                                              context->w, context->h);
        bus_engine_proxy_set_content_type (context->engine,
                                           context->purpose, context->hints);
    }
}

void
bus_input_context_disable (BusInputContext *context)
{
    g_assert (BUS_IS_INPUT_CONTEXT (context));

    bus_input_context_clear_preedit_text (context, TRUE);
    bus_input_context_update_auxiliary_text (context, text_empty, FALSE);
    bus_input_context_update_lookup_table (context,
                                           lookup_table_empty,
                                           FALSE,
                                           FALSE);
    bus_input_context_register_properties (context, props_empty);

    if (context->engine) {
        const gchar *path =
            ibus_service_get_object_path ((IBusService *)context);
        bus_engine_proxy_focus_out (context->engine, path);
        bus_engine_proxy_disable (context->engine);
    }
}

/* A list of signals (and their handler functions) that could be emit by the
 * engine proxy object.
 */
const static struct {
    const gchar *name;
    GCallback    callback;
} engine_signals [] = {
    { "commit-text",              G_CALLBACK (_engine_commit_text_cb) },
    { "forward-key-event",        G_CALLBACK (_engine_forward_key_event_cb) },
    { "delete-surrounding-text",
                              G_CALLBACK (_engine_delete_surrounding_text_cb) },
    { "require-surrounding-text",
                             G_CALLBACK (_engine_require_surrounding_text_cb) },
    { "update-preedit-text",      G_CALLBACK (_engine_update_preedit_text_cb) },
    { "show-preedit-text",        G_CALLBACK (_engine_show_preedit_text_cb) },
    { "hide-preedit-text",        G_CALLBACK (_engine_hide_preedit_text_cb) },
    { "update-auxiliary-text",
                                G_CALLBACK (_engine_update_auxiliary_text_cb) },
    { "show-auxiliary-text",      G_CALLBACK (_engine_show_auxiliary_text_cb) },
    { "hide-auxiliary-text",      G_CALLBACK (_engine_hide_auxiliary_text_cb) },
    { "update-lookup-table",      G_CALLBACK (_engine_update_lookup_table_cb) },
    { "show-lookup-table",        G_CALLBACK (_engine_show_lookup_table_cb) },
    { "hide-lookup-table",        G_CALLBACK (_engine_hide_lookup_table_cb) },
    { "page-up-lookup-table",
                                 G_CALLBACK (_engine_page_up_lookup_table_cb) },
    { "page-down-lookup-table",
                               G_CALLBACK (_engine_page_down_lookup_table_cb) },
    { "cursor-up-lookup-table",
                               G_CALLBACK (_engine_cursor_up_lookup_table_cb) },
    { "cursor-down-lookup-table",
                             G_CALLBACK (_engine_cursor_down_lookup_table_cb) },
    { "register-properties",      G_CALLBACK (_engine_register_properties_cb) },
    { "update-property",          G_CALLBACK (_engine_update_property_cb) },
    { "panel-extension",          G_CALLBACK (_engine_panel_extension_cb) },
    { "destroy",                  G_CALLBACK (_engine_destroy_cb) }
};

static void
bus_input_context_unset_engine (BusInputContext *context)
{
    g_assert (BUS_IS_INPUT_CONTEXT (context));

    bus_input_context_clear_preedit_text (context, TRUE);
    bus_input_context_update_auxiliary_text (context, text_empty, FALSE);
    bus_input_context_update_lookup_table (context,
                                           lookup_table_empty,
                                           FALSE,
                                           FALSE);
    bus_input_context_register_properties (context, props_empty);

    if (context->engine) {
        gint i;
        const gchar *path =
            ibus_service_get_object_path ((IBusService *)context);
        /* uninstall signal handlers for the engine. */
        for (i = 0; i < G_N_ELEMENTS(engine_signals); i++) {
            g_signal_handlers_disconnect_by_func (context->engine,
                    engine_signals[i].callback, context);
        }
        /* focus out engine so that the next call of
           bus_engine_proxy_focus_in() will take effect and trigger
           RegisterProperties. */
        bus_engine_proxy_focus_out (context->engine, path);
        g_object_unref (context->engine);
        context->engine = NULL;
    }
}

void
bus_input_context_set_engine (BusInputContext *context,
                              BusEngineProxy  *engine)
{
    g_assert (BUS_IS_INPUT_CONTEXT (context));

    if (context->engine == engine)
        return;

    if (context->engine != NULL) {
        bus_input_context_unset_engine (context);
    }

    if (engine == NULL) {
        bus_input_context_disable (context);
    }
    else {
        gint i;
        context->engine = engine;
        g_object_ref (context->engine);

        /* handle signals from the engine. */
        for (i = 0; i < G_N_ELEMENTS(engine_signals); i++) {
            g_signal_connect (context->engine,
                              engine_signals[i].name,
                              engine_signals[i].callback,
                              context);
        }
        if (context->has_focus) {
            const gchar *path =
                    ibus_service_get_object_path ((IBusService *)context);
            bus_engine_proxy_focus_in (context->engine, path, context->client);
            bus_engine_proxy_enable (context->engine);
            bus_engine_proxy_set_capabilities (context->engine, 
                                               context->capabilities);
            bus_engine_proxy_set_cursor_location (context->engine,
                                                  context->x,
                                                  context->y,
                                                  context->w,
                                                  context->h);
            bus_engine_proxy_set_content_type (context->engine,
                                               context->purpose,
                                               context->hints);
        }
    }
    g_signal_emit (context,
                   context_signals[ENGINE_CHANGED],
                   0);
}

static void set_engine_by_desc_data_free (SetEngineByDescData *data)
{
    if (data->context != NULL) {
        if (data->context->data == data)
            data->context->data = NULL;
        g_object_unref (data->context);
    }

    if (data->task != NULL) {
        g_object_unref (data->task);
    }

    if (data->cancellable != NULL)
        g_object_unref (data->cancellable);

    if (data->origin_cancellable != NULL) {
        if (data->cancelled_handler_id != 0)
            g_cancellable_disconnect (data->origin_cancellable,
                data->cancelled_handler_id);
        g_object_unref (data->origin_cancellable);
    }

    g_slice_free (SetEngineByDescData, data);
}

/**
 * new_engine_cb:
 *
 * A callback function to be called when bus_engine_proxy_new() is finished.
 */
static void
new_engine_cb (GObject             *obj,
               GAsyncResult        *res,
               SetEngineByDescData *data)
{
    GError *error = NULL;
    BusEngineProxy *engine = bus_engine_proxy_new_finish (res, &error);

    if (engine == NULL) {
        g_task_return_error (data->task, error);
    }
    else {
        if (data->context->data != data) {
            /* Request has been overridden or cancelled */
            g_object_unref (engine);
            g_task_return_new_error (data->task,
                                     G_IO_ERROR,
                                     G_IO_ERROR_CANCELLED,
                                     "Operation was cancelled");
        }
        else {
            /* Let BusEngineProxy call a Disable signal. */
            bus_input_context_disable (data->context);
            bus_input_context_set_engine (data->context, engine);
            g_object_unref (engine);
            bus_input_context_enable (data->context);
            g_task_return_boolean (data->task, TRUE);
        }
    }

    set_engine_by_desc_data_free (data);
}

static void
cancel_set_engine_by_desc (SetEngineByDescData *data)
{
    if (data->context->data == data)
        data->context->data = NULL;

    if (data->origin_cancellable != NULL) {
        if (data->cancelled_handler_id != 0) {
            g_cancellable_disconnect (data->origin_cancellable,
                                      data->cancelled_handler_id);
            data->cancelled_handler_id = 0;
        }

        g_object_unref (data->origin_cancellable);
        data->origin_cancellable = NULL;
    }

    if (data->cancellable != NULL) {
        g_cancellable_cancel (data->cancellable);
        g_object_unref (data->cancellable);
        data->cancellable = NULL;
    }
}

static gboolean
set_engine_by_desc_cancelled_idle_cb (SetEngineByDescData *data)
{
    cancel_set_engine_by_desc (data);
    return FALSE;
}

static void
set_engine_by_desc_cancelled_cb (GCancellable        *cancellable,
                                 SetEngineByDescData *data)
{
    /* Cancel in idle to avoid deadlock */
    g_idle_add ((GSourceFunc) set_engine_by_desc_cancelled_idle_cb, data);
}

/**
 * set_engine_by_desc_ready_cb:
 *
 * A default callback function for bus_input_context_set_engine_by_desc().
 */
static void
set_engine_by_desc_ready_cb (BusInputContext *context,
                             GAsyncResult    *res,
                             gpointer         user_data)
{
    GError *error = NULL;
    if (!bus_input_context_set_engine_by_desc_finish (context, res, &error)) {
        g_warning ("Set context engine failed: %s", error->message);
        g_error_free (error);
    }
}

void
bus_input_context_set_engine_by_desc (BusInputContext    *context,
                                      IBusEngineDesc     *desc,
                                      gint                timeout,
                                      GCancellable       *cancellable,
                                      GAsyncReadyCallback callback,
                                      gpointer            user_data)
{
    GTask *task;
    SetEngineByDescData *data;

    g_assert (BUS_IS_INPUT_CONTEXT (context));
    g_assert (IBUS_IS_ENGINE_DESC (desc));
    g_assert (cancellable == NULL || G_IS_CANCELLABLE (cancellable));

    if (context->data != NULL) {
        /* Cancel previous set_engine_by_desc() request */
        cancel_set_engine_by_desc (context->data);
    }

    /* Previous request must be completed or cancelled */
    g_assert (context->data == NULL);

    if (callback == NULL)
        callback = (GAsyncReadyCallback) set_engine_by_desc_ready_cb;

    task = g_task_new (context, cancellable, callback, user_data);
    g_task_set_source_tag (task, bus_input_context_set_engine_by_desc);

    if (g_cancellable_is_cancelled (cancellable)) {
        g_task_return_new_error (task,
                                 G_IO_ERROR,
                                 G_IO_ERROR_CANCELLED,
                                 "Operation was cancelled");
        g_object_unref (task);
        return;
    }

    data = g_slice_new0 (SetEngineByDescData);
    context->data = data;
    data->context = context;
    g_object_ref (context);
    data->task = task;

    if (cancellable != NULL) {
        data->origin_cancellable = cancellable;
        g_object_ref (cancellable);
        data->cancelled_handler_id =
                g_cancellable_connect (data->origin_cancellable,
                                       (GCallback) set_engine_by_desc_cancelled_cb,
                                       data,
                                       NULL);
    }

    data->cancellable = g_cancellable_new ();
    /* We can cancel the bus_engine_proxy_new() call by data->cancellable;
     * See cancel_set_engine_by_desc() and set_engine_by_desc_cancelled_cb(). */
    bus_engine_proxy_new (desc,
                          timeout,
                          data->cancellable,
                          (GAsyncReadyCallback) new_engine_cb,
                          data);
}

gboolean
bus_input_context_set_engine_by_desc_finish (BusInputContext  *context,
                                             GAsyncResult     *res,
                                             GError          **error)
{
    GTask *task;
    gboolean had_error;

    g_assert (BUS_IS_INPUT_CONTEXT (context));
    g_assert (g_task_is_valid (res, context));
    task = G_TASK (res);
    g_assert (g_task_get_source_tag (task) ==
                bus_input_context_set_engine_by_desc);

    /* g_task_propagate_error() is not a public API and
     * g_task_had_error() needs to be called before
     * g_task_propagate_pointer() clears task->error.
     */
    had_error = g_task_had_error (task);
    g_task_propagate_pointer (task, error);
    if (had_error)
        return FALSE;
    return TRUE;
}

BusEngineProxy *
bus_input_context_get_engine (BusInputContext *context)
{
    g_assert (BUS_IS_INPUT_CONTEXT (context));

    return context->engine;
}

IBusEngineDesc *
bus_input_context_get_engine_desc (BusInputContext *context)
{
    g_assert (BUS_IS_INPUT_CONTEXT (context));
    if (context->engine)
        return bus_engine_proxy_get_desc (context->engine);
    return NULL;
}

guint
bus_input_context_get_capabilities (BusInputContext *context)
{
    g_assert (BUS_IS_INPUT_CONTEXT (context));
    return context->capabilities;
}

void
bus_input_context_set_capabilities (BusInputContext    *context,
                                    guint               capabilities)
{
    g_assert (BUS_IS_INPUT_CONTEXT (context));

    /* If the context does not support IBUS_CAP_FOCUS, then the client
     * application have to handle all information such as
     * preedit and auxiliary text. */
    if ((capabilities & IBUS_CAP_FOCUS) == 0) {
        capabilities |= (IBUS_CAP_PREEDIT_TEXT | IBUS_CAP_AUXILIARY_TEXT | IBUS_CAP_LOOKUP_TABLE | IBUS_CAP_PROPERTY);
    }

    if (context->capabilities != capabilities) {
        context->capabilities = capabilities;

        /* If the context does not support IBUS_CAP_FOCUS, then we always assume
         * it has focus. */
        if ((capabilities & IBUS_CAP_FOCUS) == 0) {
            bus_input_context_focus_in (context);
        }

        if (context->engine) {
            bus_engine_proxy_set_capabilities (context->engine, capabilities);
        }
    }

    context->capabilities = capabilities;
}


const gchar *
bus_input_context_get_client (BusInputContext *context)
{
    g_assert (BUS_IS_INPUT_CONTEXT (context));
    return context->client;
}

void
bus_input_context_get_content_type (BusInputContext *context,
                                    guint           *purpose,
                                    guint           *hints)
{
    g_assert (BUS_IS_INPUT_CONTEXT (context));
    g_return_if_fail (purpose != NULL && hints != NULL);

    *purpose = context->purpose;
    *hints = context->hints;
}

void
bus_input_context_set_content_type (BusInputContext *context,
                                    guint            purpose,
                                    guint            hints)
{
    GVariant *value;
    GError *error = NULL;

    g_assert (BUS_IS_INPUT_CONTEXT (context));

    value = g_variant_ref_sink (g_variant_new ("(uu)", purpose, hints));
    _ic_set_content_type (context, value, &error);
    if (error) {
        g_warning ("Failed to emit PropertiesChanged signal: %s",
                   error->message);
        g_error_free (error);
    }
    g_variant_unref (value);
}

void
bus_input_context_commit_text_use_extension (BusInputContext *context,
                                             IBusText        *text,
                                             gboolean         use_extension)
{
    SyncForwardingPreData pre_data = { 'c', text, };

    g_assert (BUS_IS_INPUT_CONTEXT (context));
    g_assert (context->queue_during_process_key_event);

    if (text == text_empty || text == NULL)
        return;

    if (use_extension && context->emoji_extension) {
        bus_panel_proxy_commit_text_received (context->emoji_extension, text);
    } else if (bus_input_context_make_post_process_key_event (context,
                                                              &pre_data)) {
        return;
    } else {
        GVariant *variant = ibus_serializable_serialize (
                (IBusSerializable *)text);
        bus_input_context_emit_signal (context,
                                       "CommitText",
                                       g_variant_new ("(v)", variant),
                                       NULL);
    }
}

void
bus_input_context_commit_text (BusInputContext *context,
                               IBusText        *text)
{
    bus_input_context_commit_text_use_extension (context, text, TRUE);
}

void
bus_input_context_update_preedit_text (BusInputContext *context,
                                       IBusText        *text,
                                       guint            cursor_pos,
                                       gboolean         visible,
                                       guint            mode,
                                       gboolean         use_extension)
{
    gboolean extension_visible = FALSE;
    g_assert (BUS_IS_INPUT_CONTEXT (context));

    if (context->preedit_text) {
        g_object_unref (context->preedit_text);
    }

    context->preedit_text = (IBusText *) g_object_ref_sink (text ? text :
                                                            text_empty);
    context->preedit_cursor_pos = cursor_pos;
    if (use_extension) {
        context->preedit_visible = visible;
        context->preedit_mode = mode;
    }
    context->ignore_focus_out = FALSE;
    extension_visible = context->preedit_visible ||
                        (context->emoji_extension != NULL);

    if (use_extension && context->emoji_extension) {
        bus_panel_proxy_update_preedit_text (context->emoji_extension,
                                             context->preedit_text,
                                             context->preedit_cursor_pos,
                                             context->preedit_visible);
    } else if (PREEDIT_CONDITION) {
        SyncForwardingPreData pre_data = { 'u', context->preedit_text, };
        GVariant *variant = ibus_serializable_serialize (
                (IBusSerializable *)context->preedit_text);
        pre_data.u.uints[0] = context->preedit_cursor_pos;
        pre_data.u.uints[1] = extension_visible ? 1 : 0;
        pre_data.u.uints[2] = context->preedit_mode;
        if (context->client_commit_preedit)
            pre_data.key = 'm';
        if (bus_input_context_make_post_process_key_event (context,
                                                           &pre_data)) {
            g_variant_unref (variant);
            return;
        } else if (context->client_commit_preedit) {
            bus_input_context_emit_signal (
                    context,
                    "UpdatePreeditTextWithMode",
                    g_variant_new ("(vubu)",
                                   variant,
                                   context->preedit_cursor_pos,
                                   extension_visible,
                                   context->preedit_mode),
                    NULL);
        } else {
            bus_input_context_emit_signal (
                    context,
                    "UpdatePreeditText",
                    g_variant_new ("(vub)",
                                   variant,
                                   context->preedit_cursor_pos,
                                   extension_visible),
                    NULL);
        }
    } else {
        if (IGNORE_FOCUS_OUT_CONDITION)
            context->ignore_focus_out = TRUE;
        g_signal_emit (context,
                       context_signals[UPDATE_PREEDIT_TEXT],
                       0,
                       context->preedit_text,
                       context->preedit_cursor_pos,
                       extension_visible);
    }
}

void
bus_input_context_set_emoji_extension (BusInputContext *context,
                                       BusPanelProxy   *emoji_extension)
{
    g_assert (BUS_IS_INPUT_CONTEXT (context));

    if (context->emoji_extension)
        g_object_unref (context->emoji_extension);
    context->emoji_extension = emoji_extension;
    if (emoji_extension) {
        g_object_ref (context->emoji_extension);
        if (!context->connection)
            return;
        /* Use bus_input_context_update_preedit_text() instead of
         * bus_input_context_show_preedit_text() because the Wayland
         * input-method protocol requires preedit when Escape key
         * on Emojier causes another focus-in event.
         */
        if (!context->preedit_visible) {
            g_object_ref (context->preedit_text);
            bus_input_context_update_preedit_text (context,
                                                   context->preedit_text,
                                                   context->preedit_cursor_pos,
                                                   TRUE,
                                                   context->preedit_mode,
                                                   TRUE);
            g_object_unref (context->preedit_text);
        }
        bus_panel_proxy_set_cursor_location (context->emoji_extension,
                                             context->x,
                                             context->y,
                                             context->w,
                                             context->h);
    } else {
        if (!context->connection)
            return;
        /* https://gitlab.gnome.org/GNOME/gnome-shell/merge_requests/113
         * Cannot use bus_input_context_hide_preedit_text () yet.
         */
        if (!context->preedit_visible) {
            bus_input_context_update_preedit_text (context,
                                                   text_empty,
                                                   0,
                                                   FALSE,
                                                   IBUS_ENGINE_PREEDIT_CLEAR,
                                                   FALSE);
        }
    }
}

void
bus_input_context_panel_extension_received (BusInputContext    *context,
                                            IBusExtensionEvent *event)
{
    g_assert (BUS_IS_INPUT_CONTEXT (context));

    if (!context->engine)
        return;
    bus_engine_proxy_panel_extension_received (context->engine, event);
}

gboolean
bus_input_context_is_extension_lookup_table (BusInputContext *context)
{
    g_assert (BUS_IS_INPUT_CONTEXT (context));
    return context->is_extension_lookup_table;
}

void
bus_input_context_forward_process_key_event (BusInputContext *context,
                                             guint            keyval,
                                             guint            keycode,
                                             guint            modifiers)
{
    ProcessKeyEventData *data = g_slice_new0 (ProcessKeyEventData);
    g_assert (BUS_IS_INPUT_CONTEXT (context));
    if (context->has_focus && context->engine && context->fake == FALSE) {
        data->context = g_object_ref (context);
        data->keyval = keyval;
        data->keycode = keycode;
        data->modifiers = modifiers;
        bus_engine_proxy_process_key_event (
                context->engine,
                keyval,
                keycode,
                modifiers,
                (GAsyncReadyCallback)
                        _forward_process_key_event_reply_cb,
                data);
    }
}
