/* -*- mode: C; c-basic-offset: 4; indent-tabs-mode: nil; -*- */
/* vim:set et sts=4: */
/* ibus - The Input Bus
 * Copyright (C) 2008-2014 Peng Huang <shawn.p.huang@gmail.com>
 * Copyright (C) 2017-2024 Takao Fujiwara <takao.fujiwara1@gmail.com>
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
#include "panelproxy.h"

#include "global.h"
#include "marshalers.h"
#include "types.h"

/* panelproxy.c is a very simple proxy class for the panel component that does only the following:
 *
 * 1. Handle D-Bus signals from the panel process. For the list of the D-Bus signals, you can check the bus_panel_proxy_g_signal function, or
 *    introspection_xml in src/ibuspanelservice.c. The bus_panel_proxy_g_signal function simply emits a corresponding glib signal for each
 *    D-Bus signal.
 * 2. Handle glib signals for a BusPanelProxy object (which is usually emitted by bus_panel_proxy_g_signal.) The list of such glib signals is
 *    in the bus_panel_proxy_class_init function. The signal handler function, e.g. bus_panel_proxy_candidate_clicked, simply calls the
 *    corresponding function in inputcontext.c, e.g. bus_input_context_candidate_clicked, using the current focused context.
 * 3. Provide a way to call D-Bus methods in the panel process. For the list of the D-Bus methods, you can check the header file (panelproxy.h)
 *    or introspection_xml in src/ibuspanelservice.c. Functions that calls g_dbus_proxy_call, e.g. bus_panel_proxy_set_cursor_location, would
 *    fall into this category.
 * 4. Handle glib signals for a BusInputContext object. The list of such glib signals is in the input_context_signals[] array. The signal handler
 *    function, e.g. _context_set_cursor_location_cb, simply invokes a D-Bus method by calling a function like bus_panel_proxy_set_cursor_location.
 */

enum {
    PAGE_UP,
    PAGE_DOWN,
    CURSOR_UP,
    CURSOR_DOWN,
    CANDIDATE_CLICKED,
    PROPERTY_ACTIVATE,
    PROPERTY_SHOW,
    PROPERTY_HIDE,
    COMMIT_TEXT,
    PANEL_EXTENSION,
    PANEL_EXTENSION_REGISTER_KEYS,
    UPDATE_PREEDIT_TEXT_RECEIVED,
    UPDATE_LOOKUP_TABLE_RECEIVED,
    UPDATE_AUXILIARY_TEXT_RECEIVED,
    FORWARD_PROCESS_KEY_EVENT,
    LAST_SIGNAL
};

struct _BusPanelProxy {
    IBusProxy parent;

    /* instance members */
    BusInputContext *focused_context;
    PanelType panel_type;
};

struct _BusPanelProxyClass {
    IBusProxyClass parent;
    /* class members */

    void (* page_up)            (BusPanelProxy   *panel);
    void (* page_down)          (BusPanelProxy   *panel);
    void (* cursor_up)          (BusPanelProxy   *panel);
    void (* cursor_down)        (BusPanelProxy   *panel);
    void (* candidate_clicked)  (BusPanelProxy   *panel,
                                 guint            index,
                                 guint            button,
                                 guint            state);

    void (* property_activate)  (BusPanelProxy   *panel,
                                 const gchar     *prop_name,
                                 gint             prop_state);
    void (* commit_text)        (BusPanelProxy   *panel,
                                 IBusText        *text);
};

static guint    panel_signals[LAST_SIGNAL] = { 0 };

/* functions prototype */
static void     bus_panel_proxy_init            (BusPanelProxy          *panel);
static void     bus_panel_proxy_real_destroy    (IBusProxy              *proxy);
static void     bus_panel_proxy_g_signal        (GDBusProxy             *proxy,
                                                 const gchar            *sender_name,
                                                 const gchar            *signal_name,
                                                 GVariant               *parameters);
static void     bus_panel_proxy_page_up         (BusPanelProxy          *panel);
static void     bus_panel_proxy_page_down       (BusPanelProxy          *panel);
static void     bus_panel_proxy_cursor_up       (BusPanelProxy          *panel);
static void     bus_panel_proxy_cursor_down     (BusPanelProxy          *panel);
static void     bus_panel_proxy_candidate_clicked
                                                (BusPanelProxy          *panel,
                                                 guint                   index,
                                                 guint                   button,
                                                 guint                   state);
static void     bus_panel_proxy_property_activate
                                                (BusPanelProxy          *panel,
                                                 const gchar            *prop_name,
                                                 gint                    prop_state);
static void     bus_panel_proxy_commit_text
                                                (BusPanelProxy          *panel,
                                                 IBusText               *text);

G_DEFINE_TYPE(BusPanelProxy, bus_panel_proxy, IBUS_TYPE_PROXY)

BusPanelProxy *
bus_panel_proxy_new (BusConnection *connection,
                     PanelType      panel_type)
{
    const gchar *path = NULL;
    GObject *obj;
    BusPanelProxy *panel;

    g_assert (BUS_IS_CONNECTION (connection));

    switch (panel_type) {
    case PANEL_TYPE_PANEL:
        path = IBUS_PATH_PANEL;
        break;
    case PANEL_TYPE_EXTENSION_EMOJI:
        path = IBUS_PATH_PANEL_EXTENSION_EMOJI;
        break;
    default:
        g_return_val_if_reached (NULL);
    }

    obj = g_initable_new (BUS_TYPE_PANEL_PROXY,
                          NULL,
                          NULL,
                          "g-object-path",     path,
                          "g-interface-name",  IBUS_INTERFACE_PANEL,
                          "g-connection",      bus_connection_get_dbus_connection (connection),
                          "g-default-timeout", g_gdbus_timeout,
                          "g-flags",           G_DBUS_PROXY_FLAGS_DO_NOT_AUTO_START | G_DBUS_PROXY_FLAGS_DO_NOT_LOAD_PROPERTIES,
                          NULL);

    panel = BUS_PANEL_PROXY (obj);
    panel->panel_type = panel_type;
    return panel;
}

static void
bus_panel_proxy_class_init (BusPanelProxyClass *class)
{
    IBUS_PROXY_CLASS (class)->destroy = bus_panel_proxy_real_destroy;
    G_DBUS_PROXY_CLASS (class)->g_signal = bus_panel_proxy_g_signal;

    class->page_up     = bus_panel_proxy_page_up;
    class->page_down   = bus_panel_proxy_page_down;
    class->cursor_up   = bus_panel_proxy_cursor_up;
    class->cursor_down = bus_panel_proxy_cursor_down;
    class->candidate_clicked = bus_panel_proxy_candidate_clicked;
    class->property_activate = bus_panel_proxy_property_activate;
    class->commit_text = bus_panel_proxy_commit_text;

    /* install signals */
    panel_signals[PAGE_UP] =
        g_signal_new (I_("page-up"),
            G_TYPE_FROM_CLASS (class),
            G_SIGNAL_RUN_LAST,
            G_STRUCT_OFFSET(BusPanelProxyClass, page_up),
            NULL, NULL,
            bus_marshal_VOID__VOID,
            G_TYPE_NONE, 0);
    g_signal_set_va_marshaller (panel_signals[PAGE_UP],
                                G_TYPE_FROM_CLASS (class),
                                bus_marshal_VOID__VOIDv);

    panel_signals[PAGE_DOWN] =
        g_signal_new (I_("page-down"),
            G_TYPE_FROM_CLASS (class),
            G_SIGNAL_RUN_LAST,
            G_STRUCT_OFFSET(BusPanelProxyClass, page_down),
            NULL, NULL,
            bus_marshal_VOID__VOID,
            G_TYPE_NONE, 0);
    g_signal_set_va_marshaller (panel_signals[PAGE_DOWN],
                                G_TYPE_FROM_CLASS (class),
                                bus_marshal_VOID__VOIDv);

    panel_signals[CURSOR_UP] =
        g_signal_new (I_("cursor-up"),
            G_TYPE_FROM_CLASS (class),
            G_SIGNAL_RUN_LAST,
            G_STRUCT_OFFSET(BusPanelProxyClass, cursor_up),
            NULL, NULL,
            bus_marshal_VOID__VOID,
            G_TYPE_NONE, 0);
    g_signal_set_va_marshaller (panel_signals[CURSOR_UP],
                                G_TYPE_FROM_CLASS (class),
                                bus_marshal_VOID__VOIDv);

    panel_signals[CURSOR_DOWN] =
        g_signal_new (I_("cursor-down"),
            G_TYPE_FROM_CLASS (class),
            G_SIGNAL_RUN_LAST,
            G_STRUCT_OFFSET(BusPanelProxyClass, cursor_down),
            NULL, NULL,
            bus_marshal_VOID__VOID,
            G_TYPE_NONE, 0);
    g_signal_set_va_marshaller (panel_signals[CURSOR_DOWN],
                                G_TYPE_FROM_CLASS (class),
                                bus_marshal_VOID__VOIDv);

    panel_signals[CANDIDATE_CLICKED] =
        g_signal_new (I_("candidate-clicked"),
            G_TYPE_FROM_CLASS (class),
            G_SIGNAL_RUN_LAST,
            G_STRUCT_OFFSET(BusPanelProxyClass, candidate_clicked),
            NULL, NULL,
            bus_marshal_VOID__UINT_UINT_UINT,
            G_TYPE_NONE, 3,
            G_TYPE_UINT,
            G_TYPE_UINT,
            G_TYPE_UINT);
    g_signal_set_va_marshaller (panel_signals[CANDIDATE_CLICKED],
                                G_TYPE_FROM_CLASS (class),
                                bus_marshal_VOID__UINT_UINT_UINTv);

    panel_signals[PROPERTY_ACTIVATE] =
        g_signal_new (I_("property-activate"),
            G_TYPE_FROM_CLASS (class),
            G_SIGNAL_RUN_LAST,
            G_STRUCT_OFFSET(BusPanelProxyClass, property_activate),
            NULL, NULL,
            bus_marshal_VOID__STRING_INT,
            G_TYPE_NONE, 2,
            G_TYPE_STRING,
            G_TYPE_INT);
    g_signal_set_va_marshaller (panel_signals[PROPERTY_ACTIVATE],
                                G_TYPE_FROM_CLASS (class),
                                bus_marshal_VOID__STRING_INTv);

    panel_signals[PROPERTY_SHOW] =
        g_signal_new (I_("property-show"),
            G_TYPE_FROM_CLASS (class),
            G_SIGNAL_RUN_LAST,
            0,
            NULL, NULL,
            bus_marshal_VOID__STRING,
            G_TYPE_NONE, 1,
            G_TYPE_STRING);
    g_signal_set_va_marshaller (panel_signals[PROPERTY_SHOW],
                                G_TYPE_FROM_CLASS (class),
                                bus_marshal_VOID__STRINGv);

    panel_signals[PROPERTY_HIDE] =
        g_signal_new (I_("property-hide"),
            G_TYPE_FROM_CLASS (class),
            G_SIGNAL_RUN_LAST,
            0,
            NULL, NULL,
            bus_marshal_VOID__STRING,
            G_TYPE_NONE, 1,
            G_TYPE_STRING);
    g_signal_set_va_marshaller (panel_signals[PROPERTY_HIDE],
                                G_TYPE_FROM_CLASS (class),
                                bus_marshal_VOID__STRINGv);

    panel_signals[COMMIT_TEXT] =
        g_signal_new (I_("commit-text"),
            G_TYPE_FROM_CLASS (class),
            G_SIGNAL_RUN_LAST,
            G_STRUCT_OFFSET(BusPanelProxyClass, commit_text),
            NULL, NULL,
            bus_marshal_VOID__OBJECT,
            G_TYPE_NONE, 1,
            IBUS_TYPE_TEXT);
    g_signal_set_va_marshaller (panel_signals[COMMIT_TEXT],
                                G_TYPE_FROM_CLASS (class),
                                bus_marshal_VOID__OBJECTv);

    panel_signals[PANEL_EXTENSION] =
        g_signal_new (I_("panel-extension"),
            G_TYPE_FROM_CLASS (class),
            G_SIGNAL_RUN_LAST,
            0,
            NULL, NULL,
            bus_marshal_VOID__OBJECT,
            G_TYPE_NONE, 1,
            IBUS_TYPE_EXTENSION_EVENT);
    g_signal_set_va_marshaller (panel_signals[PANEL_EXTENSION],
                                G_TYPE_FROM_CLASS (class),
                                bus_marshal_VOID__OBJECTv);

    panel_signals[PANEL_EXTENSION_REGISTER_KEYS] =
        g_signal_new (I_("panel-extension-register-keys"),
            G_TYPE_FROM_CLASS (class),
            G_SIGNAL_RUN_LAST,
            0,
            NULL, NULL,
            bus_marshal_VOID__VARIANT,
            G_TYPE_NONE, 1,
            G_TYPE_VARIANT);
    g_signal_set_va_marshaller (panel_signals[PANEL_EXTENSION_REGISTER_KEYS],
                                G_TYPE_FROM_CLASS (class),
                                bus_marshal_VOID__VARIANTv);

    panel_signals[UPDATE_PREEDIT_TEXT_RECEIVED] =
        g_signal_new (I_("update-preedit-text-received"),
            G_TYPE_FROM_CLASS (class),
            G_SIGNAL_RUN_LAST,
            0,
            NULL, NULL,
            bus_marshal_VOID__OBJECT_UINT_BOOLEAN,
            G_TYPE_NONE, 3,
            IBUS_TYPE_TEXT,
            G_TYPE_UINT,
            G_TYPE_BOOLEAN);
    g_signal_set_va_marshaller (panel_signals[UPDATE_PREEDIT_TEXT_RECEIVED],
                                G_TYPE_FROM_CLASS (class),
                                bus_marshal_VOID__OBJECT_UINT_BOOLEANv);

    panel_signals[UPDATE_LOOKUP_TABLE_RECEIVED] =
        g_signal_new (I_("update-lookup-table-received"),
            G_TYPE_FROM_CLASS (class),
            G_SIGNAL_RUN_LAST,
            0,
            NULL, NULL,
            bus_marshal_VOID__OBJECT_BOOLEAN,
            G_TYPE_NONE, 2,
            IBUS_TYPE_LOOKUP_TABLE,
            G_TYPE_BOOLEAN);
    g_signal_set_va_marshaller (panel_signals[UPDATE_LOOKUP_TABLE_RECEIVED],
                                G_TYPE_FROM_CLASS (class),
                                bus_marshal_VOID__OBJECT_BOOLEANv);

    panel_signals[UPDATE_AUXILIARY_TEXT_RECEIVED] =
        g_signal_new (I_("update-auxiliary-text-received"),
            G_TYPE_FROM_CLASS (class),
            G_SIGNAL_RUN_LAST,
            0,
            NULL, NULL,
            bus_marshal_VOID__OBJECT_BOOLEAN,
            G_TYPE_NONE, 2,
            IBUS_TYPE_TEXT,
            G_TYPE_BOOLEAN);
    g_signal_set_va_marshaller (panel_signals[UPDATE_AUXILIARY_TEXT_RECEIVED],
                                G_TYPE_FROM_CLASS (class),
                                bus_marshal_VOID__OBJECT_BOOLEANv);

    panel_signals[FORWARD_PROCESS_KEY_EVENT] =
        g_signal_new (I_("forward-process-key-event"),
            G_TYPE_FROM_CLASS (class),
            G_SIGNAL_RUN_LAST,
            0,
            NULL, NULL,
            bus_marshal_VOID__UINT_UINT_UINT,
            G_TYPE_NONE, 3,
            G_TYPE_UINT,
            G_TYPE_UINT,
            G_TYPE_UINT);
    g_signal_set_va_marshaller (panel_signals[FORWARD_PROCESS_KEY_EVENT],
                                G_TYPE_FROM_CLASS (class),
                                bus_marshal_VOID__UINT_UINT_UINTv);
}

static void
_g_object_unref_if_floating (gpointer instance)
{
    if (g_object_is_floating (instance))
        g_object_unref (instance);
}

static void
bus_panel_proxy_init (BusPanelProxy *panel)
{
    /* member variables will automatically be zero-cleared. */
}

static void
bus_panel_proxy_real_destroy (IBusProxy *proxy)
{
    BusPanelProxy *panel = (BusPanelProxy *)proxy;

    if (panel->focused_context) {
        bus_panel_proxy_focus_out (panel, panel->focused_context);
        panel->focused_context = NULL;
    }

    IBUS_PROXY_CLASS(bus_panel_proxy_parent_class)->
            destroy ((IBusProxy *)panel);
}

/**
 * bus_panel_proxy_g_signal:
 *
 * Handle all D-Bus signals from the panel process. This function emits a corresponding glib signal for each D-Bus signal.
 */
static void
bus_panel_proxy_g_signal (GDBusProxy  *proxy,
                          const gchar *sender_name,
                          const gchar *signal_name,
                          GVariant    *parameters)
{
    BusPanelProxy *panel = (BusPanelProxy *)proxy;

    /* The list of nullary D-Bus signals. */
    static const struct {
        const gchar *signal_name;
        const guint  signal_id;
    } signals [] = {
        { "PageUp",         PAGE_UP },
        { "PageDown",       PAGE_DOWN },
        { "CursorUp",       CURSOR_UP },
        { "CursorDown",     CURSOR_DOWN },
    };

    gint i;
    for (i = 0; i < G_N_ELEMENTS (signals); i++) {
        if (g_strcmp0 (signal_name, signals[i].signal_name) == 0) {
            g_signal_emit (panel, panel_signals[signals[i].signal_id], 0);
            return;
        }
    }

    /* Handle D-Bus signals with parameters. Deserialize them and emit a glib signal. */
    if (g_strcmp0 ("CandidateClicked", signal_name) == 0) {
        guint index = 0;
        guint button = 0;
        guint state = 0;
        g_variant_get (parameters, "(uuu)", &index, &button, &state);
        g_signal_emit (panel, panel_signals[CANDIDATE_CLICKED], 0, index, button, state);
        return;
    }

    if (g_strcmp0 ("PropertyActivate", signal_name) == 0) {
        gchar *prop_name = NULL;
        gint prop_state = 0;
        g_variant_get (parameters, "(&su)", &prop_name, &prop_state);
        g_signal_emit (panel, panel_signals[PROPERTY_ACTIVATE], 0, prop_name, prop_state);
        return;
    }

    if (g_strcmp0 ("PropertyShow", signal_name) == 0) {
        gchar *prop_name = NULL;
        g_variant_get (parameters, "(&s)", &prop_name);
        g_signal_emit (panel, panel_signals[PROPERTY_SHOW], 0, prop_name);
        return;
    }

    if (g_strcmp0 ("PropertyHide", signal_name) == 0) {
        gchar *prop_name = NULL;
        g_variant_get (parameters, "(&s)", &prop_name);
        g_signal_emit (panel, panel_signals[PROPERTY_HIDE], 0, prop_name);
        return;
    }

    if (g_strcmp0 ("CommitText", signal_name) == 0) {
        GVariant *arg0 = NULL;

        g_variant_get (parameters, "(v)", &arg0);
        g_return_if_fail (arg0);
        IBusText *text = IBUS_TEXT (ibus_serializable_deserialize (arg0));
        g_variant_unref (arg0);
        g_return_if_fail (text);
        g_signal_emit (panel, panel_signals[COMMIT_TEXT], 0, text);
        _g_object_unref_if_floating (text);
        return;
    }

    if (g_strcmp0 ("PanelExtension", signal_name) == 0) {
        GVariant *arg0 = NULL;

        g_variant_get (parameters, "(v)", &arg0);
        g_return_if_fail (arg0);
        IBusExtensionEvent *event = IBUS_EXTENSION_EVENT (
                ibus_serializable_deserialize (arg0));
        g_variant_unref (arg0);
        g_return_if_fail (event);
        g_signal_emit (panel, panel_signals[PANEL_EXTENSION], 0, event);
        _g_object_unref_if_floating (event);
        return;
    }

    if (g_strcmp0 ("PanelExtensionRegisterKeys", signal_name) == 0) {
        g_signal_emit (panel, panel_signals[PANEL_EXTENSION_REGISTER_KEYS], 0,
                       parameters);
        return;
    }

    if (g_strcmp0 ("UpdatePreeditTextReceived", signal_name) == 0) {
        GVariant *variant = NULL;
        guint cursor_pos = 0;
        gboolean visible = FALSE;
        IBusText *text = NULL;

        g_variant_get (parameters, "(vub)", &variant, &cursor_pos, &visible);
        g_return_if_fail (variant);
        text = (IBusText *) ibus_serializable_deserialize (variant);
        g_variant_unref (variant);
        g_return_if_fail (text);
        g_signal_emit (panel, panel_signals[UPDATE_PREEDIT_TEXT_RECEIVED], 0,
                       text, cursor_pos, visible);
        _g_object_unref_if_floating (text);
        return;
    }

    if (g_strcmp0 ("UpdateLookupTableReceived", signal_name) == 0) {
        GVariant *variant = NULL;
        gboolean visible = FALSE;
        IBusLookupTable *table = NULL;

        g_variant_get (parameters, "(vb)", &variant, &visible);
        g_return_if_fail (variant);
        table = (IBusLookupTable *) ibus_serializable_deserialize (variant);
        g_variant_unref (variant);
        g_return_if_fail (table);
        g_signal_emit (panel, panel_signals[UPDATE_LOOKUP_TABLE_RECEIVED], 0,
                       table, visible);
        _g_object_unref_if_floating (table);
        return;
    }

    if (g_strcmp0 ("UpdateAuxiliaryTextReceived", signal_name) == 0) {
        GVariant *variant = NULL;
        gboolean visible = FALSE;
        IBusText *text = NULL;

        g_variant_get (parameters, "(vb)", &variant, &visible);
        g_return_if_fail (variant);
        text = (IBusText *) ibus_serializable_deserialize (variant);
        g_variant_unref (variant);
        g_return_if_fail (text);
        g_signal_emit (panel, panel_signals[UPDATE_AUXILIARY_TEXT_RECEIVED], 0,
                       text, visible);
        _g_object_unref_if_floating (text);
        return;
    }

    if (g_strcmp0 ("ForwardProcessKeyEvent", signal_name) == 0) {
        uint keyval;
        uint keycode;
        uint modifiers;
        g_variant_get (parameters, "(uuu)", &keyval, &keycode, &modifiers);
        g_signal_emit (panel, panel_signals[FORWARD_PROCESS_KEY_EVENT], 0,
                       keyval, keycode, modifiers);
        return;
    }

    /* shound not be reached */
    g_return_if_reached ();
}


void
bus_panel_proxy_set_cursor_location (BusPanelProxy *panel,
                                     gint           x,
                                     gint           y,
                                     gint           w,
                                     gint           h)
{
    g_assert (BUS_IS_PANEL_PROXY (panel));
    g_dbus_proxy_call ((GDBusProxy *)panel,
                       "SetCursorLocation",
                       g_variant_new ("(iiii)", x, y, w, h),
                       G_DBUS_CALL_FLAGS_NONE,
                       -1, NULL, NULL, NULL);
}

void
bus_panel_proxy_set_cursor_location_relative (BusPanelProxy *panel,
                                              gint           x,
                                              gint           y,
                                              gint           w,
                                              gint           h)
{
    g_assert (BUS_IS_PANEL_PROXY (panel));
    g_dbus_proxy_call ((GDBusProxy *)panel,
                       "SetCursorLocationRelative",
                       g_variant_new ("(iiii)", x, y, w, h),
                       G_DBUS_CALL_FLAGS_NONE,
                       -1, NULL, NULL, NULL);
}

void
bus_panel_proxy_update_preedit_text (BusPanelProxy  *panel,
                                     IBusText       *text,
                                     guint           cursor_pos,
                                     gboolean        visible)
{
    g_assert (BUS_IS_PANEL_PROXY (panel));
    g_assert (IBUS_IS_TEXT (text));

    GVariant *variant = ibus_serializable_serialize ((IBusSerializable* )text);
    g_dbus_proxy_call ((GDBusProxy *)panel,
                       "UpdatePreeditText",
                       g_variant_new ("(vub)", variant, cursor_pos, visible),
                       G_DBUS_CALL_FLAGS_NONE,
                       -1, NULL, NULL, NULL);
}

void
bus_panel_proxy_update_auxiliary_text (BusPanelProxy *panel,
                                       IBusText      *text,
                                       gboolean       visible)
{
    g_assert (BUS_IS_PANEL_PROXY (panel));
    g_assert (IBUS_IS_TEXT (text));

    GVariant *variant = ibus_serializable_serialize ((IBusSerializable* )text);
    g_dbus_proxy_call ((GDBusProxy *)panel,
                       "UpdateAuxiliaryText",
                       g_variant_new ("(vb)", variant, visible),
                       G_DBUS_CALL_FLAGS_NONE,
                       -1, NULL, NULL, NULL);
}

void
bus_panel_proxy_update_lookup_table (BusPanelProxy   *panel,
                                     IBusLookupTable *table,
                                     gboolean         visible)
{
    g_assert (BUS_IS_PANEL_PROXY (panel));
    g_assert (IBUS_IS_LOOKUP_TABLE (table));

    GVariant *variant = ibus_serializable_serialize ((IBusSerializable* )table);
    g_dbus_proxy_call ((GDBusProxy *)panel,
                       "UpdateLookupTable",
                       g_variant_new ("(vb)", variant, visible),
                       G_DBUS_CALL_FLAGS_NONE,
                       -1, NULL, NULL, NULL);
}

void
bus_panel_proxy_register_properties (BusPanelProxy  *panel,
                                     IBusPropList   *prop_list)
{
    g_assert (BUS_IS_PANEL_PROXY (panel));
    g_assert (IBUS_IS_PROP_LIST (prop_list));

    GVariant *variant = ibus_serializable_serialize ((IBusSerializable *)prop_list);
    g_dbus_proxy_call ((GDBusProxy *)panel,
                       "RegisterProperties",
                       g_variant_new ("(v)", variant),
                       G_DBUS_CALL_FLAGS_NONE,
                       -1, NULL, NULL, NULL);
}

void
bus_panel_proxy_update_property (BusPanelProxy  *panel,
                                 IBusProperty   *prop)
{
    g_assert (BUS_IS_PANEL_PROXY (panel));
    g_assert (IBUS_IS_PROPERTY (prop));

    GVariant *variant = ibus_serializable_serialize ((IBusSerializable *)prop);
    g_dbus_proxy_call ((GDBusProxy *)panel,
                       "UpdateProperty",
                       g_variant_new ("(v)", variant),
                       G_DBUS_CALL_FLAGS_NONE,
                       -1, NULL, NULL, NULL);
}

void
bus_panel_proxy_set_content_type (BusPanelProxy  *panel,
                                  guint           purpose,
                                  guint           hints)
{
    g_assert (BUS_IS_PANEL_PROXY (panel));

    g_dbus_proxy_call ((GDBusProxy *)panel,
                       "ContentType",
                       g_variant_new ("(uu)", purpose, hints),
                       G_DBUS_CALL_FLAGS_NONE,
                       -1, NULL, NULL, NULL);
}

#define DEFINE_FUNC(name)                                       \
    static void                                                 \
    bus_panel_proxy_##name (BusPanelProxy *panel)               \
    {                                                           \
        g_assert (BUS_IS_PANEL_PROXY (panel));                  \
                                                                \
        if (panel->focused_context) {                           \
            bus_input_context_##name (panel->focused_context);  \
        }                                                       \
    }

DEFINE_FUNC(page_up)
DEFINE_FUNC(page_down)
DEFINE_FUNC(cursor_up)
DEFINE_FUNC(cursor_down)
#undef DEFINE_FUNC

static void
bus_panel_proxy_candidate_clicked (BusPanelProxy *panel,
                                   guint          index,
                                   guint          button,
                                   guint          state)
{
    g_assert (BUS_IS_PANEL_PROXY (panel));

    if (panel->focused_context) {
        bus_input_context_candidate_clicked (panel->focused_context,
                                             index,
                                             button,
                                             state);
    }
}

static void
bus_panel_proxy_property_activate (BusPanelProxy *panel,
                                   const gchar   *prop_name,
                                   gint          prop_state)
{
    g_assert (BUS_IS_PANEL_PROXY (panel));

    if (panel->focused_context) {
        bus_input_context_property_activate (panel->focused_context, prop_name, prop_state);
    }
}

static void
bus_panel_proxy_commit_text (BusPanelProxy *panel,
                             IBusText      *text)
{
    gboolean use_extension = TRUE;
    g_assert (BUS_IS_PANEL_PROXY (panel));
    g_assert (text != NULL);

    if (!panel->focused_context)
        return;
    if (panel->panel_type != PANEL_TYPE_PANEL)
        use_extension = FALSE;
    bus_input_context_commit_text_use_extension (panel->focused_context,
                                                 text,
                                                 use_extension);
}

#define DEFINE_FUNCTION(Name, name)                     \
    void bus_panel_proxy_##name (BusPanelProxy *panel)  \
    {                                                   \
        g_assert (BUS_IS_PANEL_PROXY (panel));          \
        g_dbus_proxy_call ((GDBusProxy *) panel,        \
                           #Name,                       \
                           NULL,                        \
                           G_DBUS_CALL_FLAGS_NONE,      \
                           -1, NULL, NULL, NULL);       \
    }

DEFINE_FUNCTION (ShowPreeditText, show_preedit_text)
DEFINE_FUNCTION (HidePreeditText, hide_preedit_text)
DEFINE_FUNCTION (ShowAuxiliaryText, show_auxiliary_text)
DEFINE_FUNCTION (HideAuxiliaryText, hide_auxiliary_text)
DEFINE_FUNCTION (ShowLookupTable, show_lookup_table)
DEFINE_FUNCTION (HideLookupTable, hide_lookup_table)
DEFINE_FUNCTION (PageUpLookupTable, page_up_lookup_table)
DEFINE_FUNCTION (PageDownLookupTable, page_down_lookup_table)
DEFINE_FUNCTION (CursorUpLookupTable, cursor_up_lookup_table)
DEFINE_FUNCTION (CursorDownLookupTable, cursor_down_lookup_table)
DEFINE_FUNCTION (StateChanged, state_changed)

#undef DEFINE_FUNCTION

static void
_context_set_cursor_location_cb (BusInputContext *context,
                                 gint             x,
                                 gint             y,
                                 gint             w,
                                 gint             h,
                                 BusPanelProxy   *panel)
{
    g_assert (BUS_IS_INPUT_CONTEXT (context));
    g_assert (BUS_IS_PANEL_PROXY (panel));

    g_return_if_fail (panel->focused_context == context);

    bus_panel_proxy_set_cursor_location (panel, x, y, w, h);
}

static void
_context_set_cursor_location_relative_cb (BusInputContext *context,
                                          gint             x,
                                          gint             y,
                                          gint             w,
                                          gint             h,
                                          BusPanelProxy   *panel)
{
    g_assert (BUS_IS_INPUT_CONTEXT (context));
    g_assert (BUS_IS_PANEL_PROXY (panel));

    g_return_if_fail (panel->focused_context == context);

    bus_panel_proxy_set_cursor_location_relative (panel, x, y, w, h);
}

static void
_context_update_preedit_text_cb (BusInputContext *context,
                                 IBusText        *text,
                                 guint            cursor_pos,
                                 gboolean         visible,
                                 BusPanelProxy   *panel)
{
    g_assert (BUS_IS_INPUT_CONTEXT (context));
    g_assert (text != NULL);
    g_assert (BUS_IS_PANEL_PROXY (panel));

    g_return_if_fail (panel->focused_context == context);

    /* The callback is called with X11 applications but
     * the callback is not called for extensions and panel
     * extensions are always calls by
     * bus_panel_proxy_update_preedit_text() directly
     * because panel extensions forward UpdatePreeditText to
     * UpdatePreeditTextReceived and it can be an infinite
     * loop.
     */
    if (panel->panel_type != PANEL_TYPE_PANEL)
        return;
    bus_panel_proxy_update_preedit_text (panel,
                                         text,
                                         cursor_pos,
                                         visible);
}

static void
_context_update_auxiliary_text_cb (BusInputContext *context,
                                   IBusText        *text,
                                   gboolean         visible,
                                   BusPanelProxy   *panel)
{
    g_assert (BUS_IS_INPUT_CONTEXT (context));
    g_assert (BUS_IS_PANEL_PROXY (panel));

    g_return_if_fail (panel->focused_context == context);

    if (!bus_input_context_is_extension_lookup_table (context) &&
        panel->panel_type == PANEL_TYPE_EXTENSION_EMOJI) {
        return;
    }
    bus_panel_proxy_update_auxiliary_text (panel,
                                           text,
                                           visible);
}

static void
_context_update_lookup_table_cb (BusInputContext *context,
                                 IBusLookupTable *table,
                                 gboolean         visible,
                                 BusPanelProxy   *panel)
{
    g_assert (BUS_IS_INPUT_CONTEXT (context));
    g_assert (BUS_IS_PANEL_PROXY (panel));

    g_return_if_fail (panel->focused_context == context);

    if (!bus_input_context_is_extension_lookup_table (context) &&
        panel->panel_type == PANEL_TYPE_EXTENSION_EMOJI) {
        return;
    }
    bus_panel_proxy_update_lookup_table (panel,
                                         table,
                                         visible);
}

static void
_context_register_properties_cb (BusInputContext *context,
                                 IBusPropList    *prop_list,
                                 BusPanelProxy   *panel)
{
    g_assert (BUS_IS_INPUT_CONTEXT (context));
    g_assert (BUS_IS_PANEL_PROXY (panel));

    g_return_if_fail (panel->focused_context == context);

    bus_panel_proxy_register_properties (panel,
                                         prop_list);
}

static void
_context_update_property_cb (BusInputContext *context,
                             IBusProperty    *prop,
                             BusPanelProxy   *panel)
{
    g_assert (BUS_IS_INPUT_CONTEXT (context));
    g_assert (BUS_IS_PANEL_PROXY (panel));

    g_return_if_fail (panel->focused_context == context);

    bus_panel_proxy_update_property (panel,
                                     prop);
}

static void
_context_destroy_cb (BusInputContext *context,
                     BusPanelProxy   *panel)
{
    g_assert (BUS_IS_INPUT_CONTEXT (context));
    g_assert (BUS_IS_PANEL_PROXY (panel));

    g_assert (context == panel->focused_context);

    bus_panel_proxy_focus_out (panel, context);
}

static void
_context_set_content_type_cb (BusInputContext *context,
                              guint            purpose,
                              guint            hints,
                              BusPanelProxy   *panel)
{
    g_assert (BUS_IS_INPUT_CONTEXT (context));
    g_assert (BUS_IS_PANEL_PROXY (panel));

    g_return_if_fail (panel->focused_context == context);

    bus_panel_proxy_set_content_type (panel, purpose, hints);
}

#define DEFINE_FUNCTION(name)                                   \
    static void _context_##name##_cb (BusInputContext *context, \
                                      BusPanelProxy   *panel)   \
    {                                                           \
        g_assert (BUS_IS_INPUT_CONTEXT (context));              \
        g_assert (BUS_IS_PANEL_PROXY (panel));                  \
                                                                \
        g_return_if_fail (panel->focused_context == context);   \
                                                                \
        bus_panel_proxy_##name (panel);                         \
    }

#define DEFINE_FUNCTION_NO_EXTENSION(name)                      \
    static void _context_##name##_cb (BusInputContext *context, \
                                      BusPanelProxy   *panel)   \
    {                                                           \
        g_assert (BUS_IS_INPUT_CONTEXT (context));              \
        g_assert (BUS_IS_PANEL_PROXY (panel));                  \
                                                                \
        g_return_if_fail (panel->focused_context == context);   \
                                                                \
        /* The callback is called with X11 applications but     \
         * the callback is not called for extensions and panel  \
         * extensions are always calls by                       \
         * bus_panel_proxy_update_preedit_text() directly       \
         * because panel extensions forward UpdatePreeditText to \
         * UpdatePreeditTextReceived and it can be an infinite  \
         * loop.                                                \
         */                                                     \
        if (panel->panel_type != PANEL_TYPE_PANEL)              \
            return;                                             \
        bus_panel_proxy_##name (panel);                         \
    }


DEFINE_FUNCTION_NO_EXTENSION (show_preedit_text)
DEFINE_FUNCTION_NO_EXTENSION (hide_preedit_text)
DEFINE_FUNCTION (show_auxiliary_text)
DEFINE_FUNCTION (hide_auxiliary_text)
DEFINE_FUNCTION (show_lookup_table)
DEFINE_FUNCTION (hide_lookup_table)
DEFINE_FUNCTION (page_up_lookup_table)
DEFINE_FUNCTION (page_down_lookup_table)
DEFINE_FUNCTION (cursor_up_lookup_table)
DEFINE_FUNCTION (cursor_down_lookup_table)
DEFINE_FUNCTION (state_changed)

#undef DEFINE_FUNCTION
#undef DEFINE_FUNCTION_NO_EXTENSION

static const struct {
    gchar *name;
    GCallback callback;
} input_context_signals[] = {
    { "set-cursor-location",        G_CALLBACK (_context_set_cursor_location_cb) },
    { "set-cursor-location-relative", G_CALLBACK (_context_set_cursor_location_relative_cb) },

    { "update-preedit-text",        G_CALLBACK (_context_update_preedit_text_cb) },
    { "show-preedit-text",          G_CALLBACK (_context_show_preedit_text_cb) },
    { "hide-preedit-text",          G_CALLBACK (_context_hide_preedit_text_cb) },

    { "update-auxiliary-text",      G_CALLBACK (_context_update_auxiliary_text_cb) },
    { "show-auxiliary-text",        G_CALLBACK (_context_show_auxiliary_text_cb) },
    { "hide-auxiliary-text",        G_CALLBACK (_context_hide_auxiliary_text_cb) },

    { "update-lookup-table",        G_CALLBACK (_context_update_lookup_table_cb) },
    { "show-lookup-table",          G_CALLBACK (_context_show_lookup_table_cb) },
    { "hide-lookup-table",          G_CALLBACK (_context_hide_lookup_table_cb) },
    { "page-up-lookup-table",       G_CALLBACK (_context_page_up_lookup_table_cb) },
    { "page-down-lookup-table",     G_CALLBACK (_context_page_down_lookup_table_cb) },
    { "cursor-up-lookup-table",     G_CALLBACK (_context_cursor_up_lookup_table_cb) },
    { "cursor-down-lookup-table",   G_CALLBACK (_context_cursor_down_lookup_table_cb) },

    { "register-properties",        G_CALLBACK (_context_register_properties_cb) },
    { "update-property",            G_CALLBACK (_context_update_property_cb) },

    { "engine-changed",             G_CALLBACK (_context_state_changed_cb) },

    { "destroy",                    G_CALLBACK (_context_destroy_cb) },

    { "set-content-type",           G_CALLBACK (_context_set_content_type_cb) },
};

void
bus_panel_proxy_focus_in (BusPanelProxy     *panel,
                          BusInputContext   *context)
{
    const gchar *path;
    guint purpose, hints;
    gint i;

    g_assert (BUS_IS_PANEL_PROXY (panel));
    g_assert (BUS_IS_INPUT_CONTEXT (context));

    if (panel->focused_context == context)
        return;

    if (panel->focused_context != NULL)
        bus_panel_proxy_focus_out (panel, panel->focused_context);

    g_object_ref_sink (context);
    panel->focused_context = context;

    path = ibus_service_get_object_path ((IBusService *)context);

    g_dbus_proxy_call ((GDBusProxy *)panel,
                       "FocusIn",
                       g_variant_new ("(o)", path),
                       G_DBUS_CALL_FLAGS_NONE,
                       -1, NULL, NULL, NULL);

    /* install signal handlers */
    for (i = 0; i < G_N_ELEMENTS (input_context_signals); i++) {
        g_signal_connect (context,
                          input_context_signals[i].name,
                          input_context_signals[i].callback,
                          panel);
    }

    bus_input_context_get_content_type (context, &purpose, &hints);
    bus_panel_proxy_set_content_type (panel, purpose, hints);
}

void
bus_panel_proxy_focus_out (BusPanelProxy    *panel,
                           BusInputContext  *context)
{
    g_assert (BUS_IS_PANEL_PROXY (panel));
    g_assert (BUS_IS_INPUT_CONTEXT (context));

    g_assert (panel->focused_context == context);

    /* uninstall signal handlers */
    gint i;
    for (i = 0; i < G_N_ELEMENTS (input_context_signals); i++) {
        g_signal_handlers_disconnect_by_func (context,
                                              input_context_signals[i].callback,
                                              panel);
    }

    const gchar *path = ibus_service_get_object_path ((IBusService *)context);

    g_dbus_proxy_call ((GDBusProxy *)panel,
                       "FocusOut",
                       g_variant_new ("(o)", path),
                       G_DBUS_CALL_FLAGS_NONE,
                       -1, NULL, NULL, NULL);

    g_object_unref (panel->focused_context);
    panel->focused_context = NULL;
}

void
bus_panel_proxy_destroy_context (BusPanelProxy    *panel,
                                 BusInputContext  *context)
{
    const gchar *path;

    g_assert (BUS_IS_PANEL_PROXY (panel));
    g_assert (BUS_IS_INPUT_CONTEXT (context));

    g_object_ref_sink (context);
    path = ibus_service_get_object_path ((IBusService *)context);

    g_dbus_proxy_call ((GDBusProxy *)panel,
                       "DestroyContext",
                       g_variant_new ("(o)", path),
                       G_DBUS_CALL_FLAGS_NONE,
                       -1, NULL, NULL, NULL);

    g_object_unref (context);
}

PanelType
bus_panel_proxy_get_panel_type (BusPanelProxy    *panel)
{
    g_assert (BUS_IS_PANEL_PROXY (panel));
    return panel->panel_type;
}

void
bus_panel_proxy_panel_extension_received (BusPanelProxy      *panel,
                                          IBusExtensionEvent *event)
{
    GVariant *data;

    g_assert (BUS_IS_PANEL_PROXY (panel));
    g_assert (event);

    data = ibus_serializable_serialize (IBUS_SERIALIZABLE (event));
    g_return_if_fail (data);
    g_dbus_proxy_call ((GDBusProxy *)panel,
                       "PanelExtensionReceived",
                       g_variant_new ("(v)", data),
                       G_DBUS_CALL_FLAGS_NONE,
                       -1, NULL, NULL, NULL);
}

void
bus_panel_proxy_process_key_event (BusPanelProxy       *panel,
                                   guint                keyval,
                                   guint                keycode,
                                   guint                state,
                                   GAsyncReadyCallback  callback,
                                   gpointer             user_data)
{
    g_assert (BUS_IS_PANEL_PROXY (panel));

    g_dbus_proxy_call ((GDBusProxy *)panel,
                       "ProcessKeyEvent",
                       g_variant_new ("(uuu)", keyval, keycode, state),
                       G_DBUS_CALL_FLAGS_NONE,
                       -1,
                       NULL,
                       callback,
                       user_data);
}

void
bus_panel_proxy_commit_text_received (BusPanelProxy *panel,
                                      IBusText      *text)
{
    GVariant *variant;

    g_assert (BUS_IS_PANEL_PROXY (panel));
    g_assert (IBUS_IS_TEXT (text));

    variant = ibus_serializable_serialize (IBUS_SERIALIZABLE (text));
    g_dbus_proxy_call ((GDBusProxy *)panel,
                       "CommitTextReceived",
                       g_variant_new ("(v)", variant),
                       G_DBUS_CALL_FLAGS_NONE,
                       -1, NULL, NULL, NULL);
}

void
bus_panel_proxy_candidate_clicked_lookup_table (BusPanelProxy *panel,
                                                guint          index,
                                                guint          button,
                                                guint          state)
{
    g_assert (BUS_IS_PANEL_PROXY (panel));

    g_dbus_proxy_call ((GDBusProxy *)panel,
                       "CandidateClickedLookupTable",
                       g_variant_new ("(uuu)", index, button, state),
                       G_DBUS_CALL_FLAGS_NONE,
                       -1, NULL, NULL, NULL);
}
