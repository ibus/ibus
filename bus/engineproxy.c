/* -*- mode: C; c-basic-offset: 4; indent-tabs-mode: nil; -*- */
/* vim:set et sts=4: */
/* ibus - The Input Bus
 * Copyright (C) 2008-2013 Peng Huang <shawn.p.huang@gmail.com>
 * Copyright (C) 2015-2022 Takao Fujiwara <takao.fujiwara1@gmail.com>
 * Copyright (C) 2008-2016 Red Hat, Inc.
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

#include "engineproxy.h"

#include "global.h"
#include "ibusimpl.h"
#include "marshalers.h"
#include "types.h"

struct _BusEngineProxy {
    IBusProxy parent;

    /* instance members */
    /* TRUE if the engine has a focus (local copy of the engine's status.) */
    gboolean has_focus;
    /* TRUE if the engine is enabled (local copy of the engine's status.) */
    gboolean enabled;
    /* A set of capabilities the current client supports (local copy of the engine's flag.) */
    guint capabilities;
    /* The current cursor location that are sent to the engine. */
    gint x;
    gint y;
    gint w;
    gint h;

    /* an engine desc used to create the proxy. */
    IBusEngineDesc *desc;

    /* a key mapping for the engine that converts keycode into keysym. the mapping is used only when use_sys_layout is FALSE. */
    IBusKeymap     *keymap;
    /* private member */

    /* cached surrounding text (see also IBusEnginePrivate and
       IBusInputContextPrivate) */
    IBusText *surrounding_text;
    guint     surrounding_cursor_pos;
    guint     selection_anchor_pos;

    /* cached properties */
    IBusPropList *prop_list;
    gboolean has_focus_id;
    gboolean has_active_surrounding_text;
};

struct _BusEngineProxyClass {
    IBusProxyClass parent;
    /* class members */
    void (* register_properties) (BusEngineProxy   *engine,
                                  IBusPropList     *prop_list);
    void (* update_property) (BusEngineProxy       *engine,
                              IBusProperty         *prop);
};

enum {
    COMMIT_TEXT,
    FORWARD_KEY_EVENT,
    DELETE_SURROUNDING_TEXT,
    REQUIRE_SURROUNDING_TEXT,
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
    PANEL_EXTENSION,
    LAST_SIGNAL,
};

enum {
    PROP_0 = 0,
    PROP_ENGINE_DESC,
};

static guint    engine_signals[LAST_SIGNAL] = { 0 };

static IBusText *text_empty = NULL;
static IBusPropList *prop_list_empty = NULL;

/* functions prototype */
static void     bus_engine_proxy_set_property   (BusEngineProxy    *engine,
                                                 guint              prop_id,
                                                 const GValue      *value,
                                                 GParamSpec        *pspec);
static void     bus_engine_proxy_get_property   (BusEngineProxy    *engine,
                                                 guint              prop_id,
                                                 GValue            *value,
                                                 GParamSpec        *pspec);
static void     bus_engine_proxy_real_register_properties
                                                (BusEngineProxy    *engine,
                                                 IBusPropList      *prop_list);
static void     bus_engine_proxy_real_update_property
                                                (BusEngineProxy    *engine,
                                                 IBusProperty      *prop);
static void     bus_engine_proxy_real_destroy   (IBusProxy         *proxy);
static void     bus_engine_proxy_g_signal       (GDBusProxy        *proxy,
                                                 const gchar       *sender_name,
                                                 const gchar       *signal_name,
                                                 GVariant          *parameters);
static void     bus_engine_proxy_initable_iface_init
                                                (GInitableIface
                                                               *initable_iface);
static void     bus_engine_proxy_get_has_focus_id
                                                (BusEngineProxy    *engine);
static void     bus_engine_proxy_get_active_surrounding_text
                                                (BusEngineProxy    *engine);

G_DEFINE_TYPE_WITH_CODE (BusEngineProxy, bus_engine_proxy, IBUS_TYPE_PROXY,
                         G_IMPLEMENT_INTERFACE (
                                 G_TYPE_INITABLE,
                                 bus_engine_proxy_initable_iface_init)
                        );

static GInitableIface *parent_initable_iface = NULL;

static void
bus_engine_proxy_class_init (BusEngineProxyClass *class)
{
    GObjectClass *gobject_class = G_OBJECT_CLASS (class);

    gobject_class->set_property =
            (GObjectSetPropertyFunc)bus_engine_proxy_set_property;
    gobject_class->get_property =
            (GObjectGetPropertyFunc)bus_engine_proxy_get_property;

    class->register_properties = bus_engine_proxy_real_register_properties;
    class->update_property = bus_engine_proxy_real_update_property;

    IBUS_PROXY_CLASS (class)->destroy = bus_engine_proxy_real_destroy;
    G_DBUS_PROXY_CLASS (class)->g_signal = bus_engine_proxy_g_signal;

    parent_initable_iface = (GInitableIface *)g_type_interface_peek (
            bus_engine_proxy_parent_class,
            G_TYPE_INITABLE);

    /* install properties */
    g_object_class_install_property (gobject_class,
                    PROP_ENGINE_DESC,
                    g_param_spec_object ("desc",
                        "desc",
                        "desc",
                        IBUS_TYPE_ENGINE_DESC,
                        G_PARAM_READWRITE |
                        G_PARAM_CONSTRUCT_ONLY |
                        G_PARAM_STATIC_NAME |
                        G_PARAM_STATIC_BLURB |
                        G_PARAM_STATIC_NICK
                        ));

    /* install glib signals that will be sent when corresponding D-Bus signals
     * are sent from an engine process.
     */
    engine_signals[COMMIT_TEXT] =
        g_signal_new (I_("commit-text"),
            G_TYPE_FROM_CLASS (class),
            G_SIGNAL_RUN_LAST,
            0,
            NULL, NULL,
            bus_marshal_VOID__OBJECT,
            G_TYPE_NONE,
            1,
            IBUS_TYPE_TEXT);

    engine_signals[FORWARD_KEY_EVENT] =
        g_signal_new (I_("forward-key-event"),
            G_TYPE_FROM_CLASS (class),
            G_SIGNAL_RUN_LAST,
            0,
            NULL, NULL,
            bus_marshal_VOID__UINT_UINT_UINT,
            G_TYPE_NONE,
            3,
            G_TYPE_UINT,
            G_TYPE_UINT,
            G_TYPE_UINT);

    engine_signals[DELETE_SURROUNDING_TEXT] =
        g_signal_new (I_("delete-surrounding-text"),
            G_TYPE_FROM_CLASS (class),
            G_SIGNAL_RUN_LAST,
            0,
            NULL, NULL,
            bus_marshal_VOID__INT_UINT,
            G_TYPE_NONE,
            2,
            G_TYPE_INT,
            G_TYPE_UINT);

    engine_signals[REQUIRE_SURROUNDING_TEXT] =
        g_signal_new (I_("require-surrounding-text"),
            G_TYPE_FROM_CLASS (class),
            G_SIGNAL_RUN_LAST,
            0,
            NULL, NULL,
            bus_marshal_VOID__VOID,
            G_TYPE_NONE,
            0);

    engine_signals[UPDATE_PREEDIT_TEXT] =
        g_signal_new (I_("update-preedit-text"),
            G_TYPE_FROM_CLASS (class),
            G_SIGNAL_RUN_LAST,
            0,
            NULL, NULL,
            bus_marshal_VOID__OBJECT_UINT_BOOLEAN_UINT,
            G_TYPE_NONE,
            4,
            IBUS_TYPE_TEXT,
            G_TYPE_UINT,
            G_TYPE_BOOLEAN,
            G_TYPE_UINT);

    engine_signals[SHOW_PREEDIT_TEXT] =
        g_signal_new (I_("show-preedit-text"),
            G_TYPE_FROM_CLASS (class),
            G_SIGNAL_RUN_LAST,
            0,
            NULL, NULL,
            bus_marshal_VOID__VOID,
            G_TYPE_NONE,
            0);

    engine_signals[HIDE_PREEDIT_TEXT] =
        g_signal_new (I_("hide-preedit-text"),
            G_TYPE_FROM_CLASS (class),
            G_SIGNAL_RUN_LAST,
            0,
            NULL, NULL,
            bus_marshal_VOID__VOID,
            G_TYPE_NONE,
            0);

    engine_signals[UPDATE_AUXILIARY_TEXT] =
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

    engine_signals[SHOW_AUXILIARY_TEXT] =
        g_signal_new (I_("show-auxiliary-text"),
            G_TYPE_FROM_CLASS (class),
            G_SIGNAL_RUN_LAST,
            0,
            NULL, NULL,
            bus_marshal_VOID__VOID,
            G_TYPE_NONE,
            0);

    engine_signals[HIDE_AUXILIARY_TEXT] =
        g_signal_new (I_("hide-auxiliary-text"),
            G_TYPE_FROM_CLASS (class),
            G_SIGNAL_RUN_LAST,
            0,
            NULL, NULL,
            bus_marshal_VOID__VOID,
            G_TYPE_NONE,
            0);

    engine_signals[UPDATE_LOOKUP_TABLE] =
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

    engine_signals[SHOW_LOOKUP_TABLE] =
        g_signal_new (I_("show-lookup-table"),
            G_TYPE_FROM_CLASS (class),
            G_SIGNAL_RUN_LAST,
            0,
            NULL, NULL,
            bus_marshal_VOID__VOID,
            G_TYPE_NONE,
            0);

    engine_signals[HIDE_LOOKUP_TABLE] =
        g_signal_new (I_("hide-lookup-table"),
            G_TYPE_FROM_CLASS (class),
            G_SIGNAL_RUN_LAST,
            0,
            NULL, NULL,
            bus_marshal_VOID__VOID,
            G_TYPE_NONE,
            0);

    engine_signals[PAGE_UP_LOOKUP_TABLE] =
        g_signal_new (I_("page-up-lookup-table"),
            G_TYPE_FROM_CLASS (class),
            G_SIGNAL_RUN_LAST,
            0,
            NULL, NULL,
            bus_marshal_VOID__VOID,
            G_TYPE_NONE,
            0);

    engine_signals[PAGE_DOWN_LOOKUP_TABLE] =
        g_signal_new (I_("page-down-lookup-table"),
            G_TYPE_FROM_CLASS (class),
            G_SIGNAL_RUN_LAST,
            0,
            NULL, NULL,
            bus_marshal_VOID__VOID,
            G_TYPE_NONE,
            0);

    engine_signals[CURSOR_UP_LOOKUP_TABLE] =
        g_signal_new (I_("cursor-up-lookup-table"),
            G_TYPE_FROM_CLASS (class),
            G_SIGNAL_RUN_LAST,
            0,
            NULL, NULL,
            bus_marshal_VOID__VOID,
            G_TYPE_NONE,
            0);

    engine_signals[CURSOR_DOWN_LOOKUP_TABLE] =
        g_signal_new (I_("cursor-down-lookup-table"),
            G_TYPE_FROM_CLASS (class),
            G_SIGNAL_RUN_LAST,
            0,
            NULL, NULL,
            bus_marshal_VOID__VOID,
            G_TYPE_NONE,
            0);

    engine_signals[REGISTER_PROPERTIES] =
        g_signal_new (I_("register-properties"),
            G_TYPE_FROM_CLASS (class),
            G_SIGNAL_RUN_LAST,
            G_STRUCT_OFFSET (BusEngineProxyClass, register_properties),
            NULL, NULL,
            bus_marshal_VOID__OBJECT,
            G_TYPE_NONE,
            1,
            IBUS_TYPE_PROP_LIST);

    engine_signals[UPDATE_PROPERTY] =
        g_signal_new (I_("update-property"),
            G_TYPE_FROM_CLASS (class),
            G_SIGNAL_RUN_LAST,
            G_STRUCT_OFFSET (BusEngineProxyClass, update_property),
            NULL, NULL,
            bus_marshal_VOID__OBJECT,
            G_TYPE_NONE,
            1,
            IBUS_TYPE_PROPERTY);

    engine_signals[PANEL_EXTENSION] =
        g_signal_new (I_("panel-extension"),
            G_TYPE_FROM_CLASS (class),
            G_SIGNAL_RUN_LAST,
            0,
            NULL, NULL,
            bus_marshal_VOID__OBJECT,
            G_TYPE_NONE,
            1,
            IBUS_TYPE_EXTENSION_EVENT);

    text_empty = ibus_text_new_from_static_string ("");
    g_object_ref_sink (text_empty);

    prop_list_empty = ibus_prop_list_new ();
    g_object_ref_sink (prop_list_empty);
}

static void
bus_engine_proxy_init (BusEngineProxy *engine)
{
    engine->surrounding_text = g_object_ref_sink (text_empty);
    engine->prop_list = g_object_ref_sink (prop_list_empty);
}

static void
bus_engine_proxy_set_property (BusEngineProxy *engine,
                               guint           prop_id,
                               const GValue   *value,
                               GParamSpec     *pspec)
{
    switch (prop_id) {
    case PROP_ENGINE_DESC:
        g_assert (engine->desc == NULL);
        engine->desc = g_value_dup_object (value);
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (engine, prop_id, pspec);
    }
}

static void
bus_engine_proxy_get_property (BusEngineProxy *engine,
                               guint           prop_id,
                               GValue         *value,
                               GParamSpec     *pspec)
{
    switch (prop_id) {
    case PROP_ENGINE_DESC:
        g_value_set_object (value, bus_engine_proxy_get_desc (engine));
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (engine, prop_id, pspec);
    }

}

static void
bus_engine_proxy_real_register_properties (BusEngineProxy *engine,
                                           IBusPropList   *prop_list)
{
    g_assert (IBUS_IS_PROP_LIST (prop_list));

    if (engine->prop_list != prop_list_empty)
        g_object_unref (engine->prop_list);
    engine->prop_list = (IBusPropList *) g_object_ref_sink (prop_list);
}

static void
bus_engine_proxy_real_update_property (BusEngineProxy *engine,
                                       IBusProperty   *prop)
{
    g_return_if_fail (prop);
    if (engine->prop_list)
        ibus_prop_list_update_property (engine->prop_list, prop);
}

static void
bus_engine_proxy_real_destroy (IBusProxy *proxy)
{
    BusEngineProxy *engine = (BusEngineProxy *)proxy;

    if (engine->desc) {
        g_object_unref (engine->desc);
        engine->desc = NULL;
    }

    if (engine->keymap) {
        g_object_unref (engine->keymap);
        engine->keymap = NULL;
    }

    if (engine->surrounding_text) {
        g_object_unref (engine->surrounding_text);
        engine->surrounding_text = NULL;
    }

    if (engine->prop_list) {
        g_object_unref (engine->prop_list);
        engine->prop_list = NULL;
    }

    IBUS_PROXY_CLASS (bus_engine_proxy_parent_class)->destroy (
            (IBusProxy *)engine);
}

static void
_g_object_unref_if_floating (gpointer instance)
{
    if (g_object_is_floating (instance))
        g_object_unref (instance);
}

/**
 * bus_engine_proxy_g_signal:
 *
 * Handle all D-Bus signals from the engine process. This function emits
 * corresponding glib signal for the D-Bus signal.
 */
static void
bus_engine_proxy_g_signal (GDBusProxy  *proxy,
                           const gchar *sender_name,
                           const gchar *signal_name,
                           GVariant    *parameters)
{
    BusEngineProxy *engine = (BusEngineProxy *)proxy;

    /* The list of nullary D-Bus signals. */
    static const struct {
        const gchar *signal_name;
        const guint  signal_id;
    } signals [] = {
        { "ShowPreeditText",        SHOW_PREEDIT_TEXT },
        { "HidePreeditText",        HIDE_PREEDIT_TEXT },
        { "ShowAuxiliaryText",      SHOW_AUXILIARY_TEXT },
        { "HideAuxiliaryText",      HIDE_AUXILIARY_TEXT },
        { "ShowLookupTable",        SHOW_LOOKUP_TABLE },
        { "HideLookupTable",        HIDE_LOOKUP_TABLE },
        { "PageUpLookupTable",      PAGE_UP_LOOKUP_TABLE },
        { "PageDownLookupTable",    PAGE_DOWN_LOOKUP_TABLE },
        { "CursorUpLookupTable",    CURSOR_UP_LOOKUP_TABLE },
        { "CursorDownLookupTable",  CURSOR_DOWN_LOOKUP_TABLE },
        { "RequireSurroundingText", REQUIRE_SURROUNDING_TEXT },
    };

    gint i;
    for (i = 0; i < G_N_ELEMENTS (signals); i++) {
        if (g_strcmp0 (signal_name, signals[i].signal_name) == 0) {
            g_signal_emit (engine, engine_signals[signals[i].signal_id], 0);
            return;
        }
    }

    /* Handle D-Bus signals with parameters. Deserialize them and emit a glib
     * signal.
     */
    if (g_strcmp0 (signal_name, "CommitText") == 0) {
        GVariant *arg0 = NULL;
        g_variant_get (parameters, "(v)", &arg0);
        g_return_if_fail (arg0 != NULL);

        IBusText *text = IBUS_TEXT (ibus_serializable_deserialize (arg0));
        g_variant_unref (arg0);
        g_return_if_fail (text != NULL);
        g_signal_emit (engine, engine_signals[COMMIT_TEXT], 0, text);
        _g_object_unref_if_floating (text);
        return;
    }

    if (g_strcmp0 (signal_name, "ForwardKeyEvent") == 0) {
        guint32 keyval = 0;
        guint32 keycode = 0;
        guint32 states = 0;
        g_variant_get (parameters, "(uuu)", &keyval, &keycode, &states);

        g_signal_emit (engine,
                       engine_signals[FORWARD_KEY_EVENT],
                       0,
                       keyval,
                       keycode,
                       states);
        return;
    }

    if (g_strcmp0 (signal_name, "DeleteSurroundingText") == 0) {
        gint  offset_from_cursor = 0;
        guint nchars = 0;
        g_variant_get (parameters, "(iu)", &offset_from_cursor, &nchars);

        g_signal_emit (engine,
                       engine_signals[DELETE_SURROUNDING_TEXT],
                       0, offset_from_cursor, nchars);
        return;
    }

    if (g_strcmp0 (signal_name, "UpdatePreeditText") == 0) {
        GVariant *arg0 = NULL;
        guint cursor_pos = 0;
        gboolean visible = FALSE;
        guint mode = 0;

        g_variant_get (parameters, "(vubu)",
                       &arg0, &cursor_pos, &visible, &mode);
        g_return_if_fail (arg0 != NULL);

        IBusText *text = IBUS_TEXT (ibus_serializable_deserialize (arg0));
        g_variant_unref (arg0);
        g_return_if_fail (text != NULL);

        g_signal_emit (engine,
                       engine_signals[UPDATE_PREEDIT_TEXT],
                       0, text, cursor_pos, visible, mode);

        _g_object_unref_if_floating (text);
        return;
    }

    if (g_strcmp0 (signal_name, "UpdateAuxiliaryText") == 0) {
        GVariant *arg0 = NULL;
        gboolean visible = FALSE;

        g_variant_get (parameters, "(vb)", &arg0, &visible);
        g_return_if_fail (arg0 != NULL);

        IBusText *text = IBUS_TEXT (ibus_serializable_deserialize (arg0));
        g_variant_unref (arg0);
        g_return_if_fail (text != NULL);

        g_signal_emit (engine,
                       engine_signals[UPDATE_AUXILIARY_TEXT],
                       0,
                       text,
                       visible);
        _g_object_unref_if_floating (text);
        return;
    }

    if (g_strcmp0 (signal_name, "UpdateLookupTable") == 0) {
        GVariant *arg0 = NULL;
        gboolean visible = FALSE;

        g_variant_get (parameters, "(vb)", &arg0, &visible);
        g_return_if_fail (arg0 != NULL);

        IBusLookupTable *table =
                IBUS_LOOKUP_TABLE (ibus_serializable_deserialize (arg0));
        g_variant_unref (arg0);
        g_return_if_fail (table != NULL);

        g_signal_emit (engine,
                       engine_signals[UPDATE_LOOKUP_TABLE],
                       0,
                       table,
                       visible);
        _g_object_unref_if_floating (table);
        return;
    }

    if (g_strcmp0 (signal_name, "RegisterProperties") == 0) {
        GVariant *arg0 = NULL;
        g_variant_get (parameters, "(v)", &arg0);
        g_return_if_fail (arg0 != NULL);

        IBusPropList *prop_list =
                IBUS_PROP_LIST (ibus_serializable_deserialize (arg0));
        g_variant_unref (arg0);
        g_return_if_fail (prop_list != NULL);

        g_signal_emit (engine,
                       engine_signals[REGISTER_PROPERTIES],
                       0,
                       prop_list);
        _g_object_unref_if_floating (prop_list);
        return;
    }

    if (g_strcmp0 (signal_name, "UpdateProperty") == 0) {
        GVariant *arg0 = NULL;
        g_variant_get (parameters, "(v)", &arg0);
        g_return_if_fail (arg0 != NULL);

        IBusProperty *prop =
                IBUS_PROPERTY (ibus_serializable_deserialize (arg0));
        g_variant_unref (arg0);
        g_return_if_fail (prop != NULL);

        g_signal_emit (engine, engine_signals[UPDATE_PROPERTY], 0, prop);
        _g_object_unref_if_floating (prop);
        return;
    }

    if (g_strcmp0 (signal_name, "PanelExtension") == 0) {
        GVariant *arg0 = NULL;
        g_variant_get (parameters, "(v)", &arg0);
        g_return_if_fail (arg0 != NULL);

        IBusExtensionEvent *event = IBUS_EXTENSION_EVENT (
                ibus_serializable_deserialize (arg0));
        g_variant_unref (arg0);
        g_return_if_fail (event != NULL);
        g_signal_emit (engine, engine_signals[PANEL_EXTENSION], 0, event);
        _g_object_unref_if_floating (event);
        return;
    }

    g_return_if_reached ();
}

static BusEngineProxy *
bus_engine_proxy_new_internal (const gchar     *path,
                               IBusEngineDesc  *desc,
                               GDBusConnection *connection)
{
    GDBusProxyFlags flags;
    BusEngineProxy *engine;
    BusIBusImpl *ibus = BUS_DEFAULT_IBUS;
    GHashTable *hash_table = NULL;

    g_assert (path);
    g_assert (IBUS_IS_ENGINE_DESC (desc));
    g_assert (G_IS_DBUS_CONNECTION (connection));

    flags = G_DBUS_PROXY_FLAGS_DO_NOT_AUTO_START;
    engine = (BusEngineProxy *) g_initable_new (
            BUS_TYPE_ENGINE_PROXY,
            NULL,
            NULL,
            "desc",              desc,
            "g-connection",      connection,
            "g-interface-name",  IBUS_INTERFACE_ENGINE,
            "g-object-path",     path,
            "g-default-timeout", g_gdbus_timeout,
            "g-flags",           flags,
            NULL);
    const gchar *layout = ibus_engine_desc_get_layout (desc);
    if (layout != NULL && layout[0] != '\0') {
        engine->keymap = ibus_keymap_get (layout);
    }

    g_return_val_if_fail (ibus, engine);

    hash_table = bus_ibus_impl_get_engine_focus_id_table (ibus);
    if (hash_table) {
        EngineFocusCategory category;
        category = (EngineFocusCategory)GPOINTER_TO_INT (
                g_hash_table_lookup (hash_table,
                                     ibus_engine_desc_get_name (desc)));
        if (category == ENGINE_FOCUS_CATEGORY_HAS_ID)
            engine->has_focus_id = TRUE;
        else if (category == ENGINE_FOCUS_CATEGORY_NO_ID)
            engine->has_focus_id = FALSE;
        else
            bus_engine_proxy_get_has_focus_id (engine);
    }

    hash_table = bus_ibus_impl_get_engine_active_surrounding_text_table (ibus);
    if (hash_table) {
        EngineSurroundingTextCategory category;
        category = (EngineSurroundingTextCategory)GPOINTER_TO_INT (
                g_hash_table_lookup (hash_table,
                                     ibus_engine_desc_get_name (desc)));
        if (category == ENGINE_SURROUNDING_TEXT_CATEGORY_HAS_ACTIVE)
            engine->has_active_surrounding_text = TRUE;
        else if (category == ENGINE_SURROUNDING_TEXT_CATEGORY_NOT_ACTIVE)
            engine->has_active_surrounding_text = FALSE;
        else
            bus_engine_proxy_get_active_surrounding_text (engine);
    }

    return engine;
}

typedef struct {
    GTask           *task;
    IBusEngineDesc  *desc;
    BusComponent    *component;
    BusFactoryProxy *factory;
    GCancellable *cancellable;
    gulong cancelled_handler_id;
    guint handler_id;
    guint timeout_id;
    gint timeout;
} EngineProxyNewData;

static void
engine_proxy_new_data_free (EngineProxyNewData *data)
{
    if (data->task != NULL) {
        g_object_unref (data->task);
    }

    if (data->desc != NULL) {
        g_object_unref (data->desc);
    }

    if (data->component != NULL) {
        if (data->handler_id != 0) {
            g_signal_handler_disconnect (data->component, data->handler_id);
        }
        g_object_unref (data->component);
    }

    if (data->factory != NULL) {
        g_object_unref (data->factory);
    }

    if (data->timeout_id != 0) {
        g_source_remove (data->timeout_id);
    }

    if (data->cancellable != NULL) {
        if (data->cancelled_handler_id != 0) {
            g_cancellable_disconnect (data->cancellable,
                                      data->cancelled_handler_id);
        }
        g_object_unref (data->cancellable);
    }

    g_slice_free (EngineProxyNewData, data);
}

/**
 * create_engine_ready_cb:
 *
 * A callback function to be called when bus_factory_proxy_create_engine
 * finishes.
 * Create an BusEngineProxy object and call the GAsyncReadyCallback.
 */
static void
create_engine_ready_cb (BusFactoryProxy    *factory,
                        GAsyncResult       *res,
                        EngineProxyNewData *data)
{
    g_return_if_fail (data->task != NULL);

    GError *error = NULL;
    gchar *path = bus_factory_proxy_create_engine_finish (factory,
                                                          res,
                                                          &error);
    if (path == NULL) {
        g_task_return_error (data->task, error);
        engine_proxy_new_data_free (data);
        return;
    }

    BusEngineProxy *engine =
            bus_engine_proxy_new_internal (path,
                                           data->desc,
                                           g_dbus_proxy_get_connection ((GDBusProxy *)data->factory));
    g_free (path);

    /* FIXME: set destroy callback ? */
    g_task_return_pointer (data->task, engine, NULL);

    engine_proxy_new_data_free (data);
}

/**
 * notify_factory_cb:
 *
 * A callback function to be called when bus_component_start() emits
 * "notify::factory" signal within 5 seconds.
 * Call bus_factory_proxy_create_engine to create the engine proxy
 * asynchronously.
 */
static void
notify_factory_cb (BusComponent       *component,
                   GParamSpec         *spec,
                   EngineProxyNewData *data)
{
    data->factory = bus_component_get_factory (data->component);

    if (data->factory != NULL) {
        g_object_ref (data->factory);
        /* Timeout should be removed */
        if (data->timeout_id != 0) {
            g_source_remove (data->timeout_id);
            data->timeout_id = 0;
        }
        /* Handler of notify::factory should be removed. */
        if (data->handler_id != 0) {
            g_signal_handler_disconnect (data->component, data->handler_id);
            data->handler_id = 0;
        }

        /* We *have to* disconnect the cancelled_cb here, since
         * g_dbus_proxy_call calls create_engine_ready_cb even if the proxy
         * call is cancelled, and in this case, create_engine_ready_cb itself
         * will return error using g_task_return_error().
         */
        if (data->cancellable && data->cancelled_handler_id != 0) {
            g_cancellable_disconnect (data->cancellable, 
                                      data->cancelled_handler_id);
            data->cancelled_handler_id = 0;
        }

        /* Create engine from factory. */
        bus_factory_proxy_create_engine (
                data->factory,
                data->desc,
                data->timeout,
                data->cancellable,
                (GAsyncReadyCallback) create_engine_ready_cb,
                data);
    }
    /* If factory is NULL, we will continue wait for
     * factory notify signal or timeout */
}

/**
 * timeout_cb:
 *
 * A callback function to be called when bus_component_start() does not emit
 * "notify::factory" signal within 5 seconds.
 * Call the GAsyncReadyCallback and stop the 5 sec timer.
 */
static gboolean
timeout_cb (EngineProxyNewData *data)
{
    g_task_return_new_error (data->task,
                             G_DBUS_ERROR,
                             G_DBUS_ERROR_FAILED,
                             "Timeout was reached");
    engine_proxy_new_data_free (data);

    return FALSE;
}

/**
 * cancelled_cb:
 *
 * A callback function to be called when someone calls g_cancellable_cancel()
 * for the cancellable object for bus_engine_proxy_new.
 * Call the GAsyncReadyCallback.
 */
static gboolean
cancelled_idle_cb (EngineProxyNewData *data)
{
    g_task_return_new_error (data->task,
                             G_DBUS_ERROR,
                             G_DBUS_ERROR_FAILED,
                             "Operation was cancelled");

    engine_proxy_new_data_free (data);

    return FALSE;
}

static void
cancelled_cb (GCancellable       *cancellable,
              EngineProxyNewData *data)
{
    /* Cancel the bus_engine_proxy_new() in idle to avoid deadlock.
     * And use HIGH priority to avoid timeout event happening before
     * idle callback. */
    g_idle_add_full (G_PRIORITY_HIGH,
                    (GSourceFunc) cancelled_idle_cb,
                    data, NULL);
}

void
bus_engine_proxy_new (IBusEngineDesc      *desc,
                      gint                 timeout,
                      GCancellable        *cancellable,
                      GAsyncReadyCallback  callback,
                      gpointer             user_data)
{
    GTask *task;

    g_assert (IBUS_IS_ENGINE_DESC (desc));
    g_assert (cancellable == NULL || G_IS_CANCELLABLE (cancellable));
    g_assert (callback);

    task = g_task_new (NULL, cancellable, callback, user_data);
    g_task_set_source_tag (task, bus_engine_proxy_new);

    if (g_cancellable_is_cancelled (cancellable)) {
        g_task_return_new_error (task,
                                 G_DBUS_ERROR,
                                 G_DBUS_ERROR_FAILED,
                                 "Operation was cancelled");
        g_object_unref (task);
        return;
    }

    EngineProxyNewData *data = g_slice_new0 (EngineProxyNewData);
    data->desc = g_object_ref (desc);
    data->component = bus_component_from_engine_desc (desc);
    g_object_ref (data->component);
    data->task = task;
    data->timeout = timeout;

    data->factory = bus_component_get_factory (data->component);

    if (data->factory == NULL) {
        /* The factory is not ready yet. Create the factory first, and wait for
         * the "notify::factory" signal. In the handler of "notify::factory",
         * we'll create the engine proxy. */
        data->handler_id = g_signal_connect (data->component,
                                             "notify::factory",
                                             G_CALLBACK (notify_factory_cb),
                                             data);
        data->timeout_id = g_timeout_add (timeout,
                                          (GSourceFunc) timeout_cb,
                                          data);
        if (cancellable) {
            data->cancellable = (GCancellable *) g_object_ref (cancellable);
            data->cancelled_handler_id = g_cancellable_connect (cancellable,
                                                                (GCallback) cancelled_cb,
                                                                data,
                                                                NULL);
        }
        bus_component_start (data->component, g_verbose);
    }
    else {
        /* The factory is ready. We'll create the engine proxy directly. */
        g_object_ref (data->factory);

        /* We don't have to connect to cancelled_cb here, since
         * g_dbus_proxy_call calls create_engine_ready_cb even if the proxy
         * call is cancelled, and in this case, create_engine_ready_cb itself
         * can return error using g_task_return_error().
         */
        bus_factory_proxy_create_engine (
                data->factory,
                data->desc,
                timeout,
                cancellable,
                (GAsyncReadyCallback) create_engine_ready_cb,
                data);
    }
}

BusEngineProxy *
bus_engine_proxy_new_finish (GAsyncResult   *res,
                             GError       **error)
{
    GTask *task;
    gboolean had_error;
    BusEngineProxy *retval = NULL;

    g_assert (error == NULL || *error == NULL);
    g_assert (g_task_is_valid (res, NULL));

    task = G_TASK (res);
    g_assert (g_task_get_source_tag (task) == bus_engine_proxy_new);

    /* g_task_propagate_error() is not a public API and
     * g_task_had_error() needs to be called before
     * g_task_propagate_pointer() clears task->error.
     */
    had_error = g_task_had_error (task);
    retval = g_task_propagate_pointer (task, error);
    if (had_error) {
        g_assert (retval == NULL);
        return NULL;
    }

    return retval;
}

void
bus_engine_proxy_process_key_event (BusEngineProxy      *engine,
                                    guint                keyval,
                                    guint                keycode,
                                    guint                state,
                                    GAsyncReadyCallback  callback,
                                    gpointer             user_data)
{
    g_assert (BUS_IS_ENGINE_PROXY (engine));

    if (keycode != 0 &&
        bus_ibus_impl_is_use_sys_layout (BUS_DEFAULT_IBUS) == FALSE) {
        /* Since use_sys_layout is false, we don't rely on XKB. Try to convert
         * keyval from keycode by using our own mapping.
         */
        IBusKeymap *keymap = engine->keymap;
        if (keymap == NULL)
            keymap = BUS_DEFAULT_KEYMAP;
        if (keymap != NULL) {
            guint t = ibus_keymap_lookup_keysym (keymap, keycode, state);
            if (t != IBUS_KEY_VoidSymbol) {
                keyval = t;
            }
        }
    }

    g_dbus_proxy_call ((GDBusProxy *)engine,
                       "ProcessKeyEvent",
                       g_variant_new ("(uuu)", keyval, keycode, state),
                       G_DBUS_CALL_FLAGS_NONE,
                       -1,
                       NULL,
                       callback,
                       user_data);
}

void
bus_engine_proxy_set_cursor_location (BusEngineProxy *engine,
                                      gint            x,
                                      gint            y,
                                      gint            w,
                                      gint            h)
{
    g_assert (BUS_IS_ENGINE_PROXY (engine));

    if (engine->x != x || engine->y != y || engine->w != w || engine->h != h) {
        engine->x = x;
        engine->y = y;
        engine->w = w;
        engine->h = h;
        g_dbus_proxy_call ((GDBusProxy *)engine,
                           "SetCursorLocation",
                           g_variant_new ("(iiii)", x, y, w, h),
                           G_DBUS_CALL_FLAGS_NONE,
                           -1,
                           NULL,
                           NULL,
                           NULL);
    }
}

void
bus_engine_proxy_process_hand_writing_event
                                  (BusEngineProxy        *engine,
                                   GVariant              *coordinates)
{
    g_assert (BUS_IS_ENGINE_PROXY (engine));

    g_dbus_proxy_call ((GDBusProxy *)engine,
                       "ProcessHandWritingEvent",
                       coordinates,
                       G_DBUS_CALL_FLAGS_NONE,
                       -1,
                       NULL,
                       NULL,
                       NULL);
}

void
bus_engine_proxy_cancel_hand_writing
                                  (BusEngineProxy        *engine,
                                   guint                  n_strokes)
{
    g_assert (BUS_IS_ENGINE_PROXY (engine));

    g_dbus_proxy_call ((GDBusProxy *)engine,
                       "CancelHandWriting",
                       g_variant_new ("(u)", n_strokes),
                       G_DBUS_CALL_FLAGS_NONE,
                       -1,
                       NULL,
                       NULL,
                       NULL);
}

void
bus_engine_proxy_set_capabilities (BusEngineProxy *engine,
                                   guint           caps)
{
    g_assert (BUS_IS_ENGINE_PROXY (engine));

    if (engine->capabilities != caps) {
        engine->capabilities = caps;
        g_dbus_proxy_call ((GDBusProxy *)engine,
                           "SetCapabilities",
                           g_variant_new ("(u)", caps),
                           G_DBUS_CALL_FLAGS_NONE,
                           -1,
                           NULL,
                           NULL,
                           NULL);
    }
}

void
bus_engine_proxy_property_activate (BusEngineProxy *engine,
                                    const gchar    *prop_name,
                                    guint           prop_state)
{
    g_assert (BUS_IS_ENGINE_PROXY (engine));
    g_assert (prop_name != NULL);

    g_dbus_proxy_call ((GDBusProxy *)engine,
                       "PropertyActivate",
                       g_variant_new ("(su)", prop_name, prop_state),
                       G_DBUS_CALL_FLAGS_NONE,
                       -1,
                       NULL,
                       NULL,
                       NULL);
}

void
bus_engine_proxy_property_show (BusEngineProxy *engine,
                                const gchar    *prop_name)
{
    g_assert (BUS_IS_ENGINE_PROXY (engine));
    g_assert (prop_name != NULL);

    g_dbus_proxy_call ((GDBusProxy *)engine,
                       "PropertyShow",
                       g_variant_new ("(s)", prop_name),
                       G_DBUS_CALL_FLAGS_NONE,
                       -1,
                       NULL,
                       NULL,
                       NULL);
}

void bus_engine_proxy_property_hide (BusEngineProxy *engine,
                                     const gchar    *prop_name)
{
    g_assert (BUS_IS_ENGINE_PROXY (engine));
    g_assert (prop_name != NULL);

    g_dbus_proxy_call ((GDBusProxy *)engine,
                       "PropertyHide",
                       g_variant_new ("(s)", prop_name),
                       G_DBUS_CALL_FLAGS_NONE,
                       -1,
                       NULL,
                       NULL,
                       NULL);
}

void bus_engine_proxy_set_surrounding_text (BusEngineProxy *engine,
                                            IBusText       *text,
                                            guint           cursor_pos,
                                            guint           anchor_pos)
{
    g_assert (BUS_IS_ENGINE_PROXY (engine));
    g_assert (text != NULL);

    if (!engine->surrounding_text ||
        g_strcmp0 (text->text, engine->surrounding_text->text) != 0 ||
        cursor_pos != engine->surrounding_cursor_pos ||
        anchor_pos != engine->selection_anchor_pos) {
        GVariant *variant =
                ibus_serializable_serialize ((IBusSerializable *)text);
        if (engine->surrounding_text)
            g_object_unref (engine->surrounding_text);
        engine->surrounding_text = (IBusText *) g_object_ref_sink (text);
        engine->surrounding_cursor_pos = cursor_pos;
        engine->selection_anchor_pos = anchor_pos;

        g_dbus_proxy_call ((GDBusProxy *)engine,
                           "SetSurroundingText",
                           g_variant_new ("(vuu)",
                                          variant,
                                          cursor_pos,
                                          anchor_pos),
                           G_DBUS_CALL_FLAGS_NONE,
                           -1,
                           NULL,
                           NULL,
                           NULL);
    }
}

void
bus_engine_proxy_set_content_type (BusEngineProxy *engine,
                                   guint           purpose,
                                   guint           hints)
{
    g_assert (BUS_IS_ENGINE_PROXY (engine));

    GVariant *cached_content_type =
        g_dbus_proxy_get_cached_property ((GDBusProxy *) engine,
                                          "ContentType");
    GVariant *content_type = g_variant_new ("(uu)", purpose, hints);

    g_variant_ref_sink (content_type);
    if (cached_content_type == NULL ||
        !g_variant_equal (content_type, cached_content_type)) {
        g_dbus_proxy_call ((GDBusProxy *) engine,
                           "org.freedesktop.DBus.Properties.Set",
                           g_variant_new ("(ssv)",
                                          IBUS_INTERFACE_ENGINE,
                                          "ContentType",
                                          content_type),
                           G_DBUS_CALL_FLAGS_NONE,
                           -1,
                           NULL,
                           NULL,
                           NULL);

        /* Need to update the cache by manual since there is a timing issue. */
        g_dbus_proxy_set_cached_property ((GDBusProxy *) engine,
                                          "ContentType",
                                          content_type);
    }

    if (cached_content_type != NULL)
        g_variant_unref (cached_content_type);
    g_variant_unref (content_type);
}

static void
bus_engine_proxy_get_engine_property (BusEngineProxy     *engine,
                                      const gchar        *prop_name,
                                      GAsyncReadyCallback callback,
                                      GHashTable         *hash_table)
{
    g_assert (BUS_IS_ENGINE_PROXY (engine));
    g_assert (hash_table);
    g_dbus_proxy_call ((GDBusProxy *) engine,
                       "org.freedesktop.DBus.Properties.Get",
                       g_variant_new ("(ss)",
                                      IBUS_INTERFACE_ENGINE,
                                      prop_name),
                       G_DBUS_CALL_FLAGS_NONE,
                       -1,
                       NULL,
                       callback,
                       g_hash_table_ref (hash_table));
}

static void
_get_has_focus_id_cb (GObject        *object,
                      GAsyncResult   *res,
                      gpointer        user_data)
{
    GHashTable *hash_table = (GHashTable*)user_data;
    BusEngineProxy *engine;
    GError *error = NULL;
    GVariant *result;

    g_return_if_fail (BUS_IS_ENGINE_PROXY (object));
    engine = BUS_ENGINE_PROXY (object);
    result = g_dbus_proxy_call_finish (G_DBUS_PROXY (object), res, &error);

    if (result != NULL) {
        GVariant *variant = NULL;
        gpointer value;
        g_variant_get (result, "(v)", &variant);
        engine->has_focus_id = g_variant_get_boolean (variant);
        g_variant_unref (variant);
        g_variant_unref (result);
        value =  GINT_TO_POINTER (engine->has_focus_id
                                  ? ENGINE_FOCUS_CATEGORY_HAS_ID
                                  : ENGINE_FOCUS_CATEGORY_NO_ID);
        g_hash_table_replace (
            hash_table,
            (gpointer)ibus_engine_desc_get_name (engine->desc),
            value);
    }
    g_hash_table_unref (hash_table);
}

static void
bus_engine_proxy_get_has_focus_id (BusEngineProxy *engine)
{
    BusIBusImpl *ibus = BUS_DEFAULT_IBUS;
    g_assert (ibus);
    bus_engine_proxy_get_engine_property (
                               engine,
                               "FocusId",
                               _get_has_focus_id_cb,
                               bus_ibus_impl_get_engine_focus_id_table (ibus));
}

static void
_get_has_active_surrounding_text_cb (GObject        *object,
                                     GAsyncResult   *res,
                                     gpointer        user_data)
{
    GHashTable *hash_table = (GHashTable*)user_data;
    BusEngineProxy *engine;
    GError *error = NULL;
    GVariant *result;

    g_return_if_fail (BUS_IS_ENGINE_PROXY (object));
    engine = BUS_ENGINE_PROXY (object);
    result = g_dbus_proxy_call_finish (G_DBUS_PROXY (object), res, &error);

    if (result != NULL) {
        GVariant *variant = NULL;
        gpointer value;
        g_variant_get (result, "(v)", &variant);
        engine->has_active_surrounding_text = g_variant_get_boolean (variant);
        g_variant_unref (variant);
        g_variant_unref (result);
        value =  GINT_TO_POINTER (
                                engine->has_active_surrounding_text
                                ? ENGINE_SURROUNDING_TEXT_CATEGORY_HAS_ACTIVE:
                                  ENGINE_SURROUNDING_TEXT_CATEGORY_NOT_ACTIVE);
        g_hash_table_replace (
            hash_table,
            (gpointer)ibus_engine_desc_get_name (engine->desc),
            value);
    }
    g_hash_table_unref (hash_table);
}

static void
bus_engine_proxy_get_active_surrounding_text (BusEngineProxy *engine)
{
    BusIBusImpl *ibus = BUS_DEFAULT_IBUS;
    g_assert (ibus);
    bus_engine_proxy_get_engine_property (
               engine,
               "ActiveSurroundingText",
               _get_has_active_surrounding_text_cb,
               bus_ibus_impl_get_engine_active_surrounding_text_table (ibus));
}

/* a macro to generate a function to call a nullary D-Bus method. */
#define DEFINE_FUNCTION(Name, name)                         \
    void                                                    \
    bus_engine_proxy_##name (BusEngineProxy *engine)        \
    {                                                       \
        g_assert (BUS_IS_ENGINE_PROXY (engine));            \
        g_dbus_proxy_call ((GDBusProxy *)engine,            \
                           #Name,                           \
                           NULL,                            \
                           G_DBUS_CALL_FLAGS_NONE,          \
                           -1, NULL, NULL, NULL);           \
    }

DEFINE_FUNCTION (Reset, reset)
DEFINE_FUNCTION (PageUp, page_up)
DEFINE_FUNCTION (PageDown, page_down)
DEFINE_FUNCTION (CursorUp, cursor_up)
DEFINE_FUNCTION (CursorDown, cursor_down)

#undef DEFINE_FUNCTION

void
bus_engine_proxy_focus_in (BusEngineProxy *engine,
                           const gchar    *object_path,
                           const gchar    *client)
{
    g_assert (BUS_IS_ENGINE_PROXY (engine));
    if (engine->has_focus)
        return;
    engine->has_focus = TRUE;
    if (engine->has_active_surrounding_text)
        g_signal_emit (engine, engine_signals[REQUIRE_SURROUNDING_TEXT], 0);
    if (engine->has_focus_id) {
        g_dbus_proxy_call ((GDBusProxy *)engine,
                           "FocusInId",
                           g_variant_new ("(ss)", object_path, client),
                           G_DBUS_CALL_FLAGS_NONE,
                           -1,
                           NULL,
                           NULL,
                           NULL);
    } else {
        g_dbus_proxy_call ((GDBusProxy *)engine,
                           "FocusIn",
                           NULL,
                           G_DBUS_CALL_FLAGS_NONE,
                           -1,
                           NULL,
                           NULL,
                           NULL);
    }
}

void
bus_engine_proxy_focus_out (BusEngineProxy *engine,
                            const gchar    *object_path)
{
    g_assert (BUS_IS_ENGINE_PROXY (engine));
    if (!engine->has_focus)
        return;
    engine->has_focus = FALSE;
    if (engine->has_focus_id) {
        g_dbus_proxy_call ((GDBusProxy *)engine,
                           "FocusOutId",
                           g_variant_new ("(s)", object_path),
                           G_DBUS_CALL_FLAGS_NONE,
                           -1,
                           NULL,
                           NULL,
                           NULL);
    } else {
        g_dbus_proxy_call ((GDBusProxy *)engine,
                           "FocusOut",
                           NULL,
                           G_DBUS_CALL_FLAGS_NONE,
                           -1,
                           NULL,
                           NULL,
                           NULL);
    }
}

void
bus_engine_proxy_enable (BusEngineProxy *engine)
{
    g_assert (BUS_IS_ENGINE_PROXY (engine));
    if (!engine->enabled) {
        engine->enabled = TRUE;
        if (engine->has_active_surrounding_text)
            g_signal_emit (engine, engine_signals[REQUIRE_SURROUNDING_TEXT], 0);
        g_dbus_proxy_call ((GDBusProxy *)engine,
                           "Enable",
                           NULL,
                           G_DBUS_CALL_FLAGS_NONE,
                           -1,
                           NULL,
                           NULL,
                           NULL);
    }
}

void
bus_engine_proxy_disable (BusEngineProxy *engine)
{
    g_assert (BUS_IS_ENGINE_PROXY (engine));
    if (engine->enabled) {
        engine->enabled = FALSE;
        g_dbus_proxy_call ((GDBusProxy *)engine,
                           "Disable",
                           NULL,
                           G_DBUS_CALL_FLAGS_NONE,
                           -1,
                           NULL,
                           NULL,
                           NULL);
    }
}

void
bus_engine_proxy_candidate_clicked (BusEngineProxy *engine,
                                    guint           index,
                                    guint           button,
                                    guint           state)
{
    g_assert (BUS_IS_ENGINE_PROXY (engine));

    g_dbus_proxy_call ((GDBusProxy *)engine,
                       "CandidateClicked",
                       g_variant_new ("(uuu)", index, button, state),
                       G_DBUS_CALL_FLAGS_NONE,
                       -1,
                       NULL,
                       NULL,
                       NULL);
}

IBusEngineDesc *
bus_engine_proxy_get_desc (BusEngineProxy *engine)
{
    g_assert (BUS_IS_ENGINE_PROXY (engine));

    return engine->desc;
}

IBusPropList *
bus_engine_proxy_get_properties (BusEngineProxy *engine)
{
    g_assert (BUS_IS_ENGINE_PROXY (engine));

    return engine->prop_list;
}

gboolean
bus_engine_proxy_is_enabled (BusEngineProxy *engine)
{
    g_assert (BUS_IS_ENGINE_PROXY (engine));

    return engine->enabled;
}

void
bus_engine_proxy_panel_extension_received (BusEngineProxy     *engine,
                                           IBusExtensionEvent *event)
{
    GVariant *variant;
    g_assert (BUS_IS_ENGINE_PROXY (engine));
    g_assert (IBUS_IS_EXTENSION_EVENT (event));

    variant = ibus_serializable_serialize_object (
            IBUS_SERIALIZABLE (event));
    g_return_if_fail (variant != NULL);
    g_dbus_proxy_call ((GDBusProxy *)engine,
                       "PanelExtensionReceived",
                       g_variant_new ("(v)", variant),
                       G_DBUS_CALL_FLAGS_NONE,
                       -1,
                       NULL,
                       NULL,
                       NULL);
}

void
bus_engine_proxy_panel_extension_register_keys (BusEngineProxy *engine,
                                                GVariant       *parameters)
{
    g_assert (BUS_IS_ENGINE_PROXY (engine));
    g_assert (parameters);

    g_dbus_proxy_call ((GDBusProxy *)engine,
                       "PanelExtensionRegisterKeys",
                       g_variant_new ("(v)", g_variant_ref (parameters)),
                       G_DBUS_CALL_FLAGS_NONE,
                       -1,
                       NULL,
                       NULL,
                       NULL);
}

static gboolean
initable_init (GInitable     *initable,
               GCancellable  *cancellable,
               GError       **error)
{
    BusEngineProxy *engine = BUS_ENGINE_PROXY (initable);
    if (engine->desc == NULL) {
        *error = g_error_new (G_DBUS_ERROR,
                              G_DBUS_ERROR_FAILED,
                              "Desc is NULL");
        return FALSE;
    }

    return parent_initable_iface->init (initable,
                                        cancellable,
                                        error);
}

static void
bus_engine_proxy_initable_iface_init (GInitableIface *initable_iface)
{
    initable_iface->init = initable_init;
}
