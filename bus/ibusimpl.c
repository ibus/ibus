/* -*- mode: C; c-basic-offset: 4; indent-tabs-mode: nil; -*- */
/* vim:set et sts=4: */
/* ibus - The Input Bus
 * Copyright (C) 2008-2013 Peng Huang <shawn.p.huang@gmail.com>
 * Copyright (C) 2011-2023 Takao Fujiwara <takao.fujiwara1@gmail.com>
 * Copyright (C) 2008-2021 Red Hat, Inc.
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

#include "ibusimpl.h"

#include <locale.h>
#include <signal.h>
#include <strings.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#include "connection.h"
#include "dbusimpl.h"
#include "factoryproxy.h"
#include "global.h"
#include "inputcontext.h"
#include "panelproxy.h"
#include "server.h"
#include "types.h"

struct _BusIBusImpl {
    IBusService parent;
    /* instance members */
    GHashTable *factory_dict;

    /* registered components */
    GList *registered_components;
    GList *contexts;

    /* a fake input context for global engine support */
    BusInputContext *fake_context;
    
    /* a list of engines that are started by a user (without the --ibus
     * command line flag.) */
    GList *register_engine_list;

    /* if TRUE, ibus-daemon uses a keysym translated by the system
     * (i.e. XKB) as-is. otherwise, ibus-daemon itself converts keycode
     * into keysym. */
    gboolean use_sys_layout;

    gboolean embed_preedit_text;

    IBusRegistry    *registry;

    /* a list of BusComponent objects that are created from component XML
     * files (or from the cache of them). */
    GList *components;

    /* a mapping from an engine name (e.g. 'pinyin') to the corresponding
     * IBusEngineDesc object. */
    GHashTable *engine_table;

    GHashTable *engine_focus_id_table;
    GHashTable *engine_active_surrounding_text_table;

    BusInputContext *focused_context;
    BusPanelProxy   *panel;
    BusPanelProxy   *emoji_extension;
    gboolean         enable_emoji_extension;

    /* a default keymap of ibus-daemon (usually "us") which is used only
     * when use_sys_layout is FALSE. */
    IBusKeymap      *keymap;

    gboolean use_global_engine;
    gchar *global_engine_name;
    gchar *global_previous_engine_name;
    GVariant *extension_register_keys;
};

struct _BusIBusImplClass {
    IBusServiceClass parent;

    /* class members */
};

enum {
    LAST_SIGNAL,
};

enum {
    PROP_0,
};

/*
static guint            _signals[LAST_SIGNAL] = { 0 };
*/

/* functions prototype */
static void     bus_ibus_impl_destroy   (BusIBusImpl        *ibus);
static void     bus_ibus_impl_service_method_call
                                        (IBusService        *service,
                                         GDBusConnection    *connection,
                                         const gchar        *sender,
                                         const gchar        *object_path,
                                         const gchar        *interface_name,
                                         const gchar        *method_name,
                                         GVariant           *parameters,
                                         GDBusMethodInvocation
                                                            *invocation);
static GVariant *
                bus_ibus_impl_service_get_property
                                        (IBusService        *service,
                                         GDBusConnection    *connection,
                                         const gchar        *sender,
                                         const gchar        *object_path,
                                         const gchar        *interface_name,
                                         const gchar        *property_name,
                                         GError            **error);
static gboolean
                bus_ibus_impl_service_set_property
                                        (IBusService        *service,
                                         GDBusConnection    *connection,
                                         const gchar        *sender,
                                         const gchar        *object_path,
                                         const gchar        *interface_name,
                                         const gchar        *property_name,
                                         GVariant           *value,
                                         GError            **error);
static void     bus_ibus_impl_registry_init
                                        (BusIBusImpl        *ibus);
static void     bus_ibus_impl_registry_changed
                                        (BusIBusImpl        *ibus);
static void     bus_ibus_impl_registry_destroy
                                        (BusIBusImpl        *ibus);
static void     bus_ibus_impl_component_name_owner_changed
                                        (BusIBusImpl        *ibus,
                                         const gchar        *name,
                                         const gchar        *old_name,
                                         const gchar        *new_name);
static void     bus_ibus_impl_global_engine_changed
                                        (BusIBusImpl        *ibus);
static void     bus_ibus_impl_set_context_engine_from_desc
                                        (BusIBusImpl        *ibus,
                                         BusInputContext    *context,
                                         IBusEngineDesc     *desc);
static BusInputContext
               *bus_ibus_impl_create_input_context
                                        (BusIBusImpl        *ibus,
                                         BusConnection      *connection,
                                         const gchar        *client);
static IBusEngineDesc
               *bus_ibus_impl_get_engine_desc
                                        (BusIBusImpl        *ibus,
                                         const gchar        *engine_name);
static void     bus_ibus_impl_set_focused_context
                                        (BusIBusImpl        *ibus,
                                         BusInputContext    *context);
static void     bus_ibus_impl_property_changed
                                        (BusIBusImpl        *service,
                                         const gchar        *property_name,
                                         GVariant           *value);
/* some callback functions */
static void     _context_engine_changed_cb
                                        (BusInputContext    *context,
                                         BusIBusImpl        *ibus);

/* The interfaces available in this class, which consists of a list of
 * methods this class implements and a list of signals this class may emit.
 * Method calls to the interface that are not defined in this XML will
 * be automatically rejected by the GDBus library (see src/ibusservice.c
 * for details.)
 * Implement org.freedesktop.DBus.Properties
 * http://dbus.freedesktop.org/doc/dbus-specification.html#standard-interfaces-properties */
static const gchar introspection_xml[] =
    "<node>\n"
    "  <interface name='org.freedesktop.IBus'>\n"
    "    <property name='Address' type='s' access='read' />\n"
    "    <property name='CurrentInputContext' type='o' access='read' />\n"
    "    <property name='Engines' type='av' access='read' />\n"
    "    <property name='GlobalEngine' type='v' access='read' />\n"
    "    <property name='PreloadEngines' type='as' access='write'>\n"
    "      <annotation\n"
    "          name='org.freedesktop.DBus.Property.EmitsChangedSignal'\n"
    "          value='true' />\n"
    "    </property>\n"
    "    <property name='EmbedPreeditText' type='b' access='readwrite'>\n"
    "      <annotation\n"
    "          name='org.freedesktop.DBus.Property.EmitsChangedSignal'\n"
    "          value='true' />\n"
    "    </property>\n"
    "    <method name='CreateInputContext'>\n"
    "      <arg direction='in'  type='s' name='client_name' />\n"
    "      <arg direction='out' type='o' name='object_path' />\n"
    "    </method>\n"
    "    <method name='RegisterComponent'>\n"
    "      <arg direction='in'  type='v' name='component' />\n"
    "    </method>\n"
    "    <method name='GetEnginesByNames'>\n"
    "      <arg direction='in'  type='as' name='names' />\n"
    "      <arg direction='out' type='av' name='engines' />\n"
    "    </method>\n"
    "    <method name='Exit'>\n"
    "      <arg direction='in'  type='b' name='restart' />\n"
    "    </method>\n"
    "    <method name='Ping'>\n"
    "      <arg direction='in'  type='v' name='data' />\n"
    "      <arg direction='out' type='v' name='data' />\n"
    "    </method>\n"
    "    <method name='SetGlobalEngine'>\n"
    "      <arg direction='in'  type='s' name='engine_name' />\n"
    "    </method>\n"
    "    <signal name='RegistryChanged'>\n"
    "    </signal>\n"
    "    <signal name='GlobalEngineChanged'>\n"
    "      <arg type='s' name='engine_name' />\n"
    "    </signal>\n"
    "    <property name='ActiveEngines' type='av' access='read' />\n"
    "    <method name='GetAddress'>\n"
    "      <arg direction='out' type='s' name='address' />\n"
    "      <annotation name='org.freedesktop.DBus.Deprecated' value='true'/>\n"
    "    </method>\n"
    "    <method name='CurrentInputContext'>\n"
    "      <arg direction='out' type='o' name='object_path' />\n"
    "      <annotation name='org.freedesktop.DBus.Deprecated' value='true'/>\n"
    "    </method>\n"
    "    <method name='ListEngines'>\n"
    "      <arg direction='out' type='av' name='engines' />\n"
    "      <annotation name='org.freedesktop.DBus.Deprecated' value='true'/>\n"
    "    </method>\n"
    "    <method name='ListActiveEngines'>\n"
    "      <arg direction='out' type='av' name='engines' />\n"
    "      <annotation name='org.freedesktop.DBus.Deprecated' value='true'/>\n"
    "    </method>\n"
    "    <method name='GetUseSysLayout'>\n"
    "      <arg direction='out' type='b' name='enabled' />\n"
    "      <annotation name='org.freedesktop.DBus.Deprecated' value='true'/>\n"
    "    </method>\n"
    "    <method name='GetUseGlobalEngine'>\n"
    "      <arg direction='out' type='b' name='enabled' />\n"
    "    </method>\n"
    "    <method name='IsGlobalEngineEnabled'>\n"
    "      <arg direction='out' type='b' name='enabled' />\n"
    "      <annotation name='org.freedesktop.DBus.Deprecated' value='true'/>\n"
    "    </method>\n"
    "    <method name='GetGlobalEngine'>\n"
    "      <arg direction='out' type='v' name='desc' />\n"
    "      <annotation name='org.freedesktop.DBus.Deprecated' value='true'/>\n"
    "    </method>\n"
    "  </interface>\n"
    "</node>\n";


G_DEFINE_TYPE (BusIBusImpl, bus_ibus_impl, IBUS_TYPE_SERVICE)

static void
bus_ibus_impl_class_init (BusIBusImplClass *class)
{
    IBUS_OBJECT_CLASS (class)->destroy =
            (IBusObjectDestroyFunc) bus_ibus_impl_destroy;

    /* override the parent class's implementation. */
    IBUS_SERVICE_CLASS (class)->service_method_call =
            bus_ibus_impl_service_method_call;
    IBUS_SERVICE_CLASS (class)->service_get_property =
            bus_ibus_impl_service_get_property;
    IBUS_SERVICE_CLASS (class)->service_set_property =
            bus_ibus_impl_service_set_property;
    /* register the xml so that bus_ibus_impl_service_method_call will be
     * called on a method call defined in the xml (e.g. 'GetAddress'.) */
    ibus_service_class_add_interfaces (IBUS_SERVICE_CLASS (class),
                                       introspection_xml);
}

/**
 * _panel_destroy_cb:
 *
 * A callback function which is called when (1) the connection to the panel
 * process is terminated,
 * or (2) ibus_proxy_destroy (ibus->panel); is called. See src/ibusproxy.c for
 * details.
 */
static void
_panel_destroy_cb (BusPanelProxy *panel,
                   BusIBusImpl   *ibus)
{
    g_assert (BUS_IS_PANEL_PROXY (panel));
    g_assert (BUS_IS_IBUS_IMPL (ibus));

    if (ibus->panel == panel)
        ibus->panel = NULL;
    else if (ibus->emoji_extension == panel)
        ibus->emoji_extension = NULL;
    else
        g_return_if_reached ();
    g_object_unref (panel);
}

static void
bus_ibus_impl_set_panel_extension_mode (BusIBusImpl        *ibus,
                                        IBusExtensionEvent *event)
{
    gboolean is_extension = FALSE;
    g_return_if_fail (BUS_IS_IBUS_IMPL (ibus));
    g_return_if_fail (IBUS_IS_EXTENSION_EVENT (event));

    if (!ibus->emoji_extension) {
        g_warning ("Panel extension is not running.");
        return;
    }

    g_return_if_fail (BUS_IS_PANEL_PROXY (ibus->emoji_extension));

    ibus->enable_emoji_extension = ibus_extension_event_is_enabled (event);
    is_extension = ibus_extension_event_is_extension (event);
    if (ibus->focused_context != NULL) {
        if (ibus->enable_emoji_extension) {
            bus_input_context_set_emoji_extension (ibus->focused_context,
                                                   ibus->emoji_extension);
        } else {
            bus_input_context_set_emoji_extension (ibus->focused_context, NULL);
        }
        if (is_extension)
            bus_input_context_panel_extension_received (ibus->focused_context,
                                                        event);
    }
    if (is_extension)
        return;

    /* Use the DBus method because it seems any DBus signal,
     * g_dbus_message_new_signal(), cannot be reached to the server. */
    bus_panel_proxy_panel_extension_received (ibus->emoji_extension,
                                              event);
}

static void
bus_ibus_impl_set_panel_extension_keys (BusIBusImpl *ibus,
                                        GVariant    *parameters)
{
    BusEngineProxy *engine = NULL;

    g_return_if_fail (BUS_IS_IBUS_IMPL (ibus));
    g_return_if_fail (parameters);

    if (!ibus->emoji_extension) {
        g_warning ("Panel extension is not running.");
        return;
    }

    if (ibus->extension_register_keys)
        g_variant_unref (ibus->extension_register_keys);
    ibus->extension_register_keys = g_variant_ref_sink (parameters);
    if (ibus->focused_context)
        engine = bus_input_context_get_engine (ibus->focused_context);
    if (!engine)
        return;
    bus_engine_proxy_panel_extension_register_keys (engine, parameters);
}

static void
_panel_panel_extension_cb (BusPanelProxy      *panel,
                           IBusExtensionEvent *event,
                           BusIBusImpl        *ibus)
{
    bus_ibus_impl_set_panel_extension_mode (ibus, event);
}

static void
_panel_panel_extension_register_keys_cb (BusInputContext *context,
                                         GVariant        *parameters,
                                         BusIBusImpl     *ibus)
{
    bus_ibus_impl_set_panel_extension_keys (ibus, parameters);
}

static void
_panel_update_preedit_text_received_cb (BusPanelProxy *panel,
                                        IBusText      *text,
                                        guint          cursor_pos,
                                        gboolean       visible,
                                        BusIBusImpl   *ibus)
{
    g_return_if_fail (BUS_IS_IBUS_IMPL (ibus));

    if (!ibus->focused_context)
        return;
    bus_input_context_update_preedit_text (ibus->focused_context,
        text, cursor_pos, visible, IBUS_ENGINE_PREEDIT_CLEAR, FALSE);
}

static void
_panel_update_lookup_table_received_cb (BusPanelProxy   *panel,
                                        IBusLookupTable *table,
                                        gboolean         visible,
                                        BusIBusImpl     *ibus)
{
    g_return_if_fail (BUS_IS_IBUS_IMPL (ibus));
    g_return_if_fail (IBUS_IS_LOOKUP_TABLE (table));

    if (!ibus->focused_context)
        return;
    /* Call bus_input_context_update_lookup_table() instead of
     * bus_panel_proxy_update_lookup_table() for panel extensions because
     * bus_input_context_page_up() can call bus_panel_proxy_page_up_received().
     */
    bus_input_context_update_lookup_table (
            ibus->focused_context, table, visible, TRUE);
}

static void
_panel_update_auxiliary_text_received_cb (BusPanelProxy *panel,
                                          IBusText      *text,
                                          gboolean       visible,
                                          BusIBusImpl   *ibus)
{
    g_return_if_fail (BUS_IS_IBUS_IMPL (ibus));
    g_return_if_fail (IBUS_IS_TEXT (text));

    if (!ibus->panel)
        return;
    bus_panel_proxy_update_auxiliary_text (ibus->panel, text, visible);
}

static void
_registry_changed_cb (IBusRegistry *registry,
                      BusIBusImpl  *ibus)
{
    bus_ibus_impl_registry_changed (ibus);
}

/*
 * _dbus_name_owner_changed_cb:
 *
 * A callback function to be called when the name-owner-changed signal is sent
 * to the dbus object.
 * This usually means a client (e.g. a panel/config/engine process or an
 * application) is connected/disconnected to/from the bus.
 */
static void
_dbus_name_owner_changed_cb (BusDBusImpl   *dbus,
                             BusConnection *orig_connection,
                             const gchar   *name,
                             const gchar   *old_name,
                             const gchar   *new_name,
                             BusIBusImpl   *ibus)
{
    PanelType panel_type = PANEL_TYPE_NONE;

    g_assert (BUS_IS_DBUS_IMPL (dbus));
    g_assert (name != NULL);
    g_assert (old_name != NULL);
    g_assert (new_name != NULL);
    g_assert (BUS_IS_IBUS_IMPL (ibus));

    if (!g_strcmp0 (name, IBUS_SERVICE_PANEL))
        panel_type = PANEL_TYPE_PANEL;
    else if (!g_strcmp0 (name, IBUS_SERVICE_PANEL_EXTENSION_EMOJI))
        panel_type = PANEL_TYPE_EXTENSION_EMOJI;

    do {
        if (panel_type == PANEL_TYPE_NONE)
            break;
        if (g_strcmp0 (new_name, "") != 0) {
            /* a Panel process is started. */
            BusConnection *connection;
            BusInputContext *context = NULL;
            BusPanelProxy   **panel = (panel_type == PANEL_TYPE_PANEL) ?
                                      &ibus->panel : &ibus->emoji_extension;
            GDBusConnection *dbus_connection = NULL;

            if (*panel != NULL) {
                ibus_proxy_destroy ((IBusProxy *)(*panel));
                /* panel should be NULL after destroy. See _panel_destroy_cb
                 * for details. */
                g_assert (*panel == NULL);
            }

            connection = bus_dbus_impl_get_connection_by_name (BUS_DEFAULT_DBUS,
                                                               new_name);
            g_return_if_fail (connection != NULL);

            dbus_connection = bus_connection_get_dbus_connection (connection);
            /* rhbz#1349148 rhbz#1385349
             * Avoid SEGV of BUS_IS_PANEL_PROXY (ibus->panel)
             * This function is called during destroying the connection
             * in this case? */
            if (dbus_connection == NULL ||
                g_dbus_connection_is_closed (dbus_connection)) {
                new_name = "";
                break;
            }

            *panel = bus_panel_proxy_new (connection, panel_type);
            if (panel_type == PANEL_TYPE_EXTENSION_EMOJI)
                ibus->enable_emoji_extension = FALSE;

            g_signal_connect (*panel,
                              "destroy",
                              G_CALLBACK (_panel_destroy_cb),
                              ibus);
            g_signal_connect (*panel,
                              "panel-extension",
                              G_CALLBACK (_panel_panel_extension_cb),
                              ibus);
            g_signal_connect (*panel,
                              "panel-extension-register-keys",
                              G_CALLBACK (
                                      _panel_panel_extension_register_keys_cb),
                              ibus);
            g_signal_connect (
                    *panel,
                    "update-preedit-text-received",
                    G_CALLBACK (_panel_update_preedit_text_received_cb),
                    ibus);
            g_signal_connect (
                    *panel,
                    "update-lookup-table-received",
                    G_CALLBACK (_panel_update_lookup_table_received_cb),
                    ibus);
            g_signal_connect (
                    *panel,
                    "update-auxiliary-text-received",
                    G_CALLBACK (_panel_update_auxiliary_text_received_cb),
                    ibus);

            if (ibus->focused_context != NULL) {
                context = ibus->focused_context;
            }
            else if (ibus->use_global_engine) {
                context = ibus->fake_context;
            }

            if (context != NULL) {
                BusEngineProxy *engine;

                bus_panel_proxy_focus_in (*panel, context);

                engine = bus_input_context_get_engine (context);
                if (engine != NULL) {
                    IBusPropList *prop_list =
                        bus_engine_proxy_get_properties (engine);
                    bus_panel_proxy_register_properties (*panel, prop_list);
                }
            }
        }
    } while (0);

    bus_ibus_impl_component_name_owner_changed (ibus, name, old_name, new_name);
}

/**
 * bus_ibus_impl_init:
 *
 * The constructor of #BusIBusImpl. Initialize all member variables of a
 * #BusIBusImpl object.
 */
static void
bus_ibus_impl_init (BusIBusImpl *ibus)
{
    ibus->factory_dict = g_hash_table_new_full (
                            g_str_hash,
                            g_str_equal,
                            NULL,
                            (GDestroyNotify) g_object_unref);

    ibus->fake_context = bus_input_context_new (NULL, "fake");
    g_object_ref_sink (ibus->fake_context);
    bus_dbus_impl_register_object (BUS_DEFAULT_DBUS,
                                   (IBusService *) ibus->fake_context);
    bus_input_context_set_capabilities (ibus->fake_context,
                                        IBUS_CAP_PREEDIT_TEXT |
                                        IBUS_CAP_FOCUS |
                                        IBUS_CAP_SURROUNDING_TEXT);
    g_signal_connect (ibus->fake_context,
                      "engine-changed",
                      G_CALLBACK (_context_engine_changed_cb),
                      ibus);
    bus_input_context_focus_in (ibus->fake_context);

    ibus->register_engine_list = NULL;
    ibus->contexts = NULL;
    ibus->focused_context = NULL;
    ibus->panel = NULL;
    ibus->emoji_extension = NULL;

    ibus->keymap = ibus_keymap_get ("us");

    ibus->use_sys_layout = TRUE;
    ibus->embed_preedit_text = TRUE;
    ibus->use_global_engine = TRUE;
    ibus->global_engine_name = NULL;
    ibus->global_previous_engine_name = NULL;
    ibus->engine_focus_id_table = g_hash_table_new (g_str_hash, g_str_equal);
    ibus->engine_active_surrounding_text_table = g_hash_table_new (g_str_hash,
                                                                   g_str_equal);

    /* focus the fake_context, if use_global_engine is enabled. */
    if (ibus->use_global_engine)
        bus_ibus_impl_set_focused_context (ibus, ibus->fake_context);

    g_signal_connect (BUS_DEFAULT_DBUS,
                      "name-owner-changed",
                      G_CALLBACK (_dbus_name_owner_changed_cb),
                      ibus);

    bus_ibus_impl_registry_init (ibus);
}

/**
 * bus_ibus_impl_destroy:
 *
 * The destructor of BusIBusImpl.
 */
static void
bus_ibus_impl_destroy (BusIBusImpl *ibus)
{
    pid_t pid;
    glong timeout;
    gint status;
    gboolean flag;

    g_list_foreach (ibus->components, (GFunc) bus_component_stop, NULL);

    timeout = 0;
    flag = FALSE;
    while (1) {
        while ((pid = waitpid (0, &status, WNOHANG)) > 0);

        if (pid == -1) { /* all children finished */
            break;
        }
        if (pid == 0) { /* no child status changed */
            g_usleep (1000);
            timeout += 1000;
            if (timeout >= G_USEC_PER_SEC) {
                if (flag == FALSE) {
                    gpointer old;
                    old = signal (SIGTERM, SIG_IGN);
                    /* send TERM signal to the whole process group (i.e.
                     * engines, panel, and config daemon.) */
                    kill (-getpid (), SIGTERM);
                    signal (SIGTERM, old);
                    flag = TRUE;
                }
                else {
                    g_warning ("Not every child processes exited!");
                    break;
                }
            }
        }
    }

    g_list_free_full (ibus->register_engine_list, g_object_unref);
    ibus->register_engine_list = NULL;

    if (ibus->factory_dict)
        g_clear_pointer (&ibus->factory_dict, g_hash_table_destroy);

    if (ibus->keymap)
        g_clear_pointer (&ibus->keymap, g_object_unref);

    g_clear_pointer (&ibus->global_engine_name, g_free);
    g_clear_pointer (&ibus->global_previous_engine_name, g_free);

    if (ibus->fake_context)
        g_clear_pointer (&ibus->fake_context, g_object_unref);

    bus_ibus_impl_registry_destroy (ibus);

    if (ibus->engine_focus_id_table)
        g_clear_pointer (&ibus->engine_focus_id_table, g_hash_table_destroy);
    if (ibus->engine_active_surrounding_text_table != NULL) {
        g_clear_pointer (&ibus->engine_active_surrounding_text_table,
                         g_hash_table_destroy);
    }

    IBUS_OBJECT_CLASS (bus_ibus_impl_parent_class)->destroy (
            IBUS_OBJECT (ibus));
}

/**
 * _ibus_get_address:
 *
 * Implement the "Address" method call of the org.freedesktop.IBus interface.
 */
static GVariant *
_ibus_get_address (BusIBusImpl     *ibus,
                   GDBusConnection *connection,
                   GError         **error)
{
    GVariant *retval = g_variant_new_string (bus_server_get_address ());
    if (!retval) {
        g_set_error (error,
                     G_DBUS_ERROR,
                     G_DBUS_ERROR_FAILED,
                     "Cannot get IBus address");
    }
    return retval;
}

static void
_ibus_get_address_depre (BusIBusImpl           *ibus,
                         GVariant              *parameters,
                         GDBusMethodInvocation *invocation)
{
    GDBusConnection *connection =
            g_dbus_method_invocation_get_connection (invocation);
    GError *error = NULL;
    GVariant *variant = NULL;

    g_warning ("org.freedesktop.IBus.GetAddress() is deprecated!");

    variant = _ibus_get_address (ibus, connection, &error);

    g_dbus_method_invocation_return_value (invocation,
            g_variant_new ("(s)", g_variant_get_string (variant, NULL)));

    g_variant_unref (variant);
}

static IBusEngineDesc *
_find_engine_desc_by_name (BusIBusImpl *ibus,
                           const gchar *engine_name)
{
    IBusEngineDesc *desc = NULL;
    GList *p;

    /* find engine in registered engine list */
    for (p = ibus->register_engine_list; p != NULL; p = p->next) {
        desc = (IBusEngineDesc *) p->data;
        if (g_strcmp0 (ibus_engine_desc_get_name (desc), engine_name) == 0)
            return desc;
    }

    return NULL;
}

/**
 * _context_request_engine_cb:
 *
 * A callback function to be called when the "request-engine" signal is sent to
 * the context.
 */
static IBusEngineDesc *
_context_request_engine_cb (BusInputContext *context,
                            const gchar     *engine_name,
                            BusIBusImpl     *ibus)
{
    if (engine_name == NULL || engine_name[0] == '\0')
        engine_name = DEFAULT_ENGINE;

    return bus_ibus_impl_get_engine_desc (ibus, engine_name);
}

/**
 * bus_ibus_impl_get_engine_desc:
 *
 * Get the IBusEngineDesc by engine_name. If the engine_name is NULL, return
 * a default engine desc.
 */
static IBusEngineDesc *
bus_ibus_impl_get_engine_desc (BusIBusImpl *ibus,
                               const gchar *engine_name)
{
    g_return_val_if_fail (engine_name != NULL, NULL);
    g_return_val_if_fail (engine_name[0] != '\0', NULL);

    IBusEngineDesc *desc = _find_engine_desc_by_name (ibus, engine_name);
    if (desc == NULL) {
        desc = (IBusEngineDesc *) g_hash_table_lookup (ibus->engine_table,
                                                       engine_name);
    }
    return desc;
}

static void
bus_ibus_impl_set_context_engine_from_desc (BusIBusImpl     *ibus,
                                            BusInputContext *context,
                                            IBusEngineDesc  *desc)
{
    bus_input_context_set_engine_by_desc (
            context,
            desc,
            g_gdbus_timeout, /* timeout in msec. */
            NULL, /* we do not cancel the call. */
            NULL, /* use the default callback function. */
            NULL);
}

static void
_context_panel_extension_cb (BusInputContext    *context,
                             IBusExtensionEvent *event,
                             BusIBusImpl        *ibus)
{
    bus_ibus_impl_set_panel_extension_mode (ibus, event);
}

const static struct {
    const gchar *name;
    GCallback    callback;
} context_signals [] = {
    { "panel-extension",             G_CALLBACK (_context_panel_extension_cb) }
};

/**
 * bus_ibus_impl_set_focused_context:
 *
 * Set the current focused context.
 */
static void
bus_ibus_impl_set_focused_context (BusIBusImpl     *ibus,
                                   BusInputContext *context)
{
    gint i;
    BusEngineProxy *engine = NULL;
    guint purpose = 0;
    guint hints = 0;

    g_assert (BUS_IS_IBUS_IMPL (ibus));
    g_assert (context == NULL || BUS_IS_INPUT_CONTEXT (context));
    g_assert (context == NULL ||
              bus_input_context_get_capabilities (context) & IBUS_CAP_FOCUS);

    /* Do noting if it is focused context. */
    if (ibus->focused_context == context) {
        return;
    }

    if (ibus->focused_context) {
        if (ibus->use_global_engine) {
            /* dettach engine from the focused context */
            engine = bus_input_context_get_engine (ibus->focused_context);
            if (engine) {
                g_object_ref (engine);
                /* _ic_focus_in() can be called before _ic_focus_out() is
                 * called under the async processes of two ibus clients.
                 * E.g. gedit is a little slower v.s. a simple GtkTextView
                 * application is the fastest when you click a Hangul
                 * preedit text between the applications.
                 * preedit will be committed with focus-out in the ibus client
                 * likes ibus-im.so
                 * so do not commit preedit here in focus-in event.
                 */
                bus_input_context_clear_preedit_text (ibus->focused_context,
                                                      FALSE);
                bus_input_context_set_engine (ibus->focused_context, NULL);
                bus_input_context_set_emoji_extension (ibus->focused_context,
                                                       NULL);
            }
        }

        if (ibus->panel != NULL)
            bus_panel_proxy_focus_out (ibus->panel, ibus->focused_context);
        if (ibus->emoji_extension != NULL) {
            bus_panel_proxy_focus_out (ibus->emoji_extension,
                                       ibus->focused_context);
        }
        bus_input_context_set_emoji_extension (ibus->focused_context, NULL);

        bus_input_context_get_content_type (ibus->focused_context,
                                            &purpose, &hints);
        for (i = 0; i < G_N_ELEMENTS(context_signals); i++) {
            g_signal_handlers_disconnect_by_func (ibus->focused_context,
                    context_signals[i].callback, ibus);
        }
        g_object_unref (ibus->focused_context);
        ibus->focused_context = NULL;
    }

    if (context == NULL && ibus->use_global_engine) {
        context = ibus->fake_context;
        if (context)
            bus_input_context_set_content_type (context, purpose, hints);
    }

    if (context) {
        ibus->focused_context = (BusInputContext *) g_object_ref (context);
        /* attach engine to the focused context */
        if (engine != NULL) {
            bus_input_context_set_engine (context, engine);
            bus_input_context_enable (context);
            if (ibus->enable_emoji_extension) {
                bus_input_context_set_emoji_extension (context,
                                                       ibus->emoji_extension);
            } else {
                bus_input_context_set_emoji_extension (context, NULL);
            }
        }
        for (i = 0; i < G_N_ELEMENTS(context_signals); i++) {
            g_signal_connect (ibus->focused_context,
                              context_signals[i].name,
                              context_signals[i].callback,
                              ibus);
        }

        if (ibus->panel != NULL)
            bus_panel_proxy_focus_in (ibus->panel, context);
        if (ibus->emoji_extension != NULL)
            bus_panel_proxy_focus_in (ibus->emoji_extension, context);
    }

    if (engine != NULL)
        g_object_unref (engine);
}

static void
bus_ibus_impl_set_global_engine (BusIBusImpl    *ibus,
                                 BusEngineProxy *engine)
{
    if (!ibus->use_global_engine)
        return;

    if (ibus->focused_context) {
        bus_input_context_set_engine (ibus->focused_context, engine);
        if (ibus->enable_emoji_extension) {
            bus_input_context_set_emoji_extension (ibus->focused_context,
                                                   ibus->emoji_extension);
        } else {
            bus_input_context_set_emoji_extension (ibus->focused_context, NULL);
        }
    } else if (ibus->fake_context) {
        bus_input_context_set_engine (ibus->fake_context, engine);
    }
}

static void
bus_ibus_impl_set_global_engine_by_name (BusIBusImpl *ibus,
                                         const gchar *name)
{
    if (!ibus->use_global_engine)
        return;

    BusInputContext *context = ibus->focused_context != NULL
                               ? ibus->focused_context : ibus->fake_context;

    if (context == NULL) {
        return;
    }

    if (g_strcmp0 (name, ibus->global_engine_name) == 0) {
        /* If the user requested the same global engine, then we just enable the
         * original one. */
        bus_input_context_enable (context);
        return;
    }

    /* If there is a focused input context, then we just change the engine of
     * the focused context, which will then change the global engine
     * automatically. Otherwise, we need to change the global engine directly.
     */
    IBusEngineDesc *desc = NULL;
    desc = bus_ibus_impl_get_engine_desc (ibus, name);
    if (desc != NULL) {
        bus_ibus_impl_set_context_engine_from_desc (ibus,
                                                    context,
                                                    desc);
    }
}

/* When preload_engines and register_engiens are changed, this function
 * will check the global engine. If necessay, it will change the global engine.
 */
static void
bus_ibus_impl_check_global_engine (BusIBusImpl *ibus)
{
    GList *engine_list = NULL;

    /* do nothing */
    if (!ibus->use_global_engine)
        return;

    /* The current global engine is not removed, so do nothing. */
    if (ibus->global_engine_name != NULL &&
        _find_engine_desc_by_name (ibus, ibus->global_engine_name)) {
        return;
    }

    /* If the previous engine is available, then just switch to it. */
    if (ibus->global_previous_engine_name != NULL &&
        _find_engine_desc_by_name (ibus, ibus->global_previous_engine_name)) {
        bus_ibus_impl_set_global_engine_by_name (
            ibus, ibus->global_previous_engine_name);
        return;
    }

    /* Just switch to the fist engine in the list. */
    engine_list = ibus->register_engine_list;

    if (engine_list) {
        IBusEngineDesc *engine_desc = (IBusEngineDesc *)engine_list->data;
        bus_ibus_impl_set_global_engine_by_name (ibus,
                        ibus_engine_desc_get_name (engine_desc));
        return;
    }

    /* No engine available? Just disable global engine. */
    bus_ibus_impl_set_global_engine (ibus, NULL);
}

/**
 * _context_engine_changed_cb:
 *
 * A callback function to be called when the "engine-changed" signal is sent to
 * the context.
 * Update global engine as well if necessary.
 */
static void
_context_engine_changed_cb (BusInputContext *context,
                            BusIBusImpl     *ibus)
{
    if (!ibus->use_global_engine)
        return;

    if ((context == ibus->focused_context) ||
        (ibus->focused_context == NULL && context == ibus->fake_context)) {
        BusEngineProxy *engine = bus_input_context_get_engine (context);
        if (engine != NULL) {
            /* only set global engine if engine is not NULL */
            const gchar *name = ibus_engine_desc_get_name (
                    bus_engine_proxy_get_desc (engine));
            if (g_strcmp0 (name, ibus->global_engine_name) == 0)
                return;
            g_free (ibus->global_previous_engine_name);
            ibus->global_previous_engine_name = ibus->global_engine_name;
            ibus->global_engine_name = g_strdup (name);
            bus_ibus_impl_global_engine_changed (ibus);
        }
    }
}

/**
 * _context_focus_in_cb:
 *
 * A callback function to be called when the "focus-in" signal is sent to the
 * context.
 * If necessary, enables the global engine on the context and update
 * ibus->focused_context.
 */
static void
_context_focus_in_cb (BusInputContext *context,
                      BusIBusImpl     *ibus)
{
    g_assert (BUS_IS_IBUS_IMPL (ibus));
    g_assert (BUS_IS_INPUT_CONTEXT (context));

    /* Do nothing if context does not support focus.
     * The global engine shoule be detached from the focused context. */
    if ((bus_input_context_get_capabilities (context) & IBUS_CAP_FOCUS) == 0) {
        return;
    }

    bus_ibus_impl_set_focused_context (ibus, context);
}

/**
 * _context_focus_out_cb:
 *
 * A callback function to be called when the "focus-out" signal is sent to the
 * context.
 */
static void
_context_focus_out_cb (BusInputContext    *context,
                       BusIBusImpl        *ibus)
{
    g_assert (BUS_IS_IBUS_IMPL (ibus));
    g_assert (BUS_IS_INPUT_CONTEXT (context));

    /* Do noting if context does not support focus.
     * Actually, the context should emit focus signals, if it does not support
     * focus */
    if ((bus_input_context_get_capabilities (context) & IBUS_CAP_FOCUS) == 0) {
        return;
    }

    /* Do noting if it is not focused context. */
    if (ibus->focused_context != context) {
        return;
    }

    bus_ibus_impl_set_focused_context (ibus, NULL);
}

/**
 * _context_destroy_cb:
 *
 * A callback function to be called when the "destroy" signal is sent to the
 * context.
 */
static void
_context_destroy_cb (BusInputContext    *context,
                     BusIBusImpl        *ibus)
{
    g_assert (BUS_IS_IBUS_IMPL (ibus));
    g_assert (BUS_IS_INPUT_CONTEXT (context));

    if (context == ibus->focused_context)
        bus_ibus_impl_set_focused_context (ibus, NULL);

    if (ibus->panel &&
        bus_input_context_get_capabilities (context) & IBUS_CAP_FOCUS) {
        bus_panel_proxy_destroy_context (ibus->panel, context);
    }
    if (ibus->emoji_extension &&
        bus_input_context_get_capabilities (context) & IBUS_CAP_FOCUS) {
        bus_panel_proxy_destroy_context (ibus->emoji_extension, context);
    }

    ibus->contexts = g_list_remove (ibus->contexts, context);
    g_object_unref (context);
}

/**
 * bus_ibus_impl_create_input_context:
 * @client: A name of a client. e.g. "gtk-im"
 * @returns: A BusInputContext object.
 *
 * Create a new BusInputContext object for the client.
 */
static BusInputContext *
bus_ibus_impl_create_input_context (BusIBusImpl   *ibus,
                                    BusConnection *connection,
                                    const gchar   *client)
{
    BusInputContext *context = bus_input_context_new (connection, client);
    g_object_ref_sink (context);
    ibus->contexts = g_list_append (ibus->contexts, context);

    /* Installs glib signal handlers so that the ibus object could be notified
     * when e.g. an IBus.InputContext D-Bus method is called. */
    static const struct {
        gchar *name;
        GCallback callback;
    } signals [] = {
        { "request-engine", G_CALLBACK (_context_request_engine_cb) },
        { "engine-changed", G_CALLBACK (_context_engine_changed_cb) },
        { "focus-in",       G_CALLBACK (_context_focus_in_cb) },
        { "focus-out",      G_CALLBACK (_context_focus_out_cb) },
        { "destroy",        G_CALLBACK (_context_destroy_cb) },
    };

    gint i;
    for (i = 0; i < G_N_ELEMENTS (signals); i++) {
        g_signal_connect (context,
                          signals[i].name,
                          signals[i].callback,
                          ibus);
    }

    bus_input_context_enable (context);

    /* register the context object so that the object could handle
     * IBus.InputContext method calls. */
    bus_dbus_impl_register_object (BUS_DEFAULT_DBUS,
                                   (IBusService *) context);
    g_object_ref (context);
    return context;
}

/**
 * _ibus_create_input_context:
 *
 * Implement the "CreateInputContext" method call of the org.freedesktop.IBus
 * interface.
 */
static void
_ibus_create_input_context (BusIBusImpl           *ibus,
                            GVariant              *parameters,
                            GDBusMethodInvocation *invocation)
{
    const gchar *client_name = NULL;  /* e.g. "gtk-im" */
    g_variant_get (parameters, "(&s)", &client_name);

    BusConnection *connection =
            bus_connection_lookup (g_dbus_method_invocation_get_connection (invocation));
    BusInputContext *context =
            bus_ibus_impl_create_input_context (ibus,
                                                connection,
                                                client_name);
    if (context) {
        const gchar *path =
                ibus_service_get_object_path ((IBusService *) context);
        /* the format-string 'o' is for a D-Bus object path. */
        g_dbus_method_invocation_return_value (invocation,
                                               g_variant_new ("(o)", path));
        g_object_unref (context);
    }
    else {
        g_dbus_method_invocation_return_error (invocation,
                                               G_DBUS_ERROR,
                                               G_DBUS_ERROR_FAILED,
                                               "Create input context failed!");
    }
}

/**
 * _ibus_get_current_input_context:
 *
 * Implement the "CurrentInputContext" method call of the
 * org.freedesktop.IBus interface.
 */
static GVariant *
_ibus_get_current_input_context (BusIBusImpl     *ibus,
                                 GDBusConnection *connection,
                                 GError         **error)
{
    GVariant *retval = NULL;

    if (!ibus->focused_context)
    {
        g_set_error (error, G_DBUS_ERROR, G_DBUS_ERROR_FAILED,
                     "No focused input context");
        return NULL;
    }
    else {
        const gchar *path = ibus_service_get_object_path (
                (IBusService *) ibus->focused_context);
        /* the format-string 'o' is for a D-Bus object path. */
        retval = g_variant_new_object_path (path);
        if (!retval) {
            g_set_error (error, G_DBUS_ERROR, G_DBUS_ERROR_FAILED,
                         "Could not get object path from %s",
                         path ? path : "(null)");
        }
    }
    return retval;
}

static void
_ibus_current_input_context_depre (BusIBusImpl           *ibus,
                                   GVariant              *parameters,
                                   GDBusMethodInvocation *invocation)
{
    GDBusConnection *connection =
            g_dbus_method_invocation_get_connection (invocation);
    GVariant *variant = NULL;
    GError *error = NULL;

    g_warning ("org.freedesktop.IBus.CurrentInputContext() is deprecated!");

    variant = _ibus_get_current_input_context (ibus, connection, &error);

    if (variant == NULL) {
        g_dbus_method_invocation_return_error (
                invocation, G_DBUS_ERROR, G_DBUS_ERROR_FAILED,
                "%s", error->message);
        g_error_free (error);
    } else {
        const gchar *path = g_variant_get_string (variant, NULL);
        g_dbus_method_invocation_return_value (invocation,
                                               g_variant_new ("(o)", path));
        g_variant_unref (variant);
    }
}

static void
_component_destroy_cb (BusComponent *component,
                       BusIBusImpl  *ibus)
{
    g_assert (BUS_IS_IBUS_IMPL (ibus));
    g_assert (BUS_IS_COMPONENT (component));

    ibus->registered_components = g_list_remove (ibus->registered_components,
                                                 component);

    /* remove engines from engine_list */
    GList *engines = bus_component_get_engines (component);
    GList *p;
    for (p = engines; p != NULL; p = p->next) {
        if (g_list_find (ibus->register_engine_list, p->data)) {
            ibus->register_engine_list =
                    g_list_remove (ibus->register_engine_list, p->data);
            g_object_unref (p->data);
        }
    }
    g_list_free (engines);

    g_object_unref (component);

    bus_ibus_impl_check_global_engine (ibus);
}

/**
 * _ibus_register_component:
 *
 * Implement the "RegisterComponent" method call of the
 * org.freedesktop.IBus interface.
 */
static void
_ibus_register_component (BusIBusImpl           *ibus,
                          GVariant              *parameters,
                          GDBusMethodInvocation *invocation)
{
    GVariant *variant = g_variant_get_child_value (parameters, 0);
    IBusComponent *component =
            (IBusComponent *) ibus_serializable_deserialize (variant);

    if (!IBUS_IS_COMPONENT (component)) {
        if (component)
            g_object_unref (component);
        g_dbus_method_invocation_return_error (
                invocation, G_DBUS_ERROR, G_DBUS_ERROR_FAILED,
                "The first argument should be an IBusComponent.");
        return;
    }

    BusConnection *connection = bus_connection_lookup (
            g_dbus_method_invocation_get_connection (invocation));
    BusFactoryProxy *factory = bus_factory_proxy_new (connection);

    if (factory == NULL) {
        g_object_unref (component);
        g_dbus_method_invocation_return_error (
                invocation, G_DBUS_ERROR, G_DBUS_ERROR_FAILED,
                "Create factory failed.");
        return;
    }

    g_object_ref_sink (component);

    BusComponent *buscomp = bus_component_new (component, factory);
    bus_component_set_destroy_with_factory (buscomp, TRUE);
    g_object_unref (component);
    g_object_unref (factory);

    ibus->registered_components = g_list_append (ibus->registered_components,
                                                g_object_ref_sink (buscomp));
    GList *engines = bus_component_get_engines (buscomp);
    g_list_foreach (engines, (GFunc) g_object_ref, NULL);
    ibus->register_engine_list = g_list_concat (ibus->register_engine_list,
                                               engines);

    g_signal_connect (buscomp,
                      "destroy",
                      G_CALLBACK (_component_destroy_cb),
                      ibus);

    g_dbus_method_invocation_return_value (invocation, NULL);
}

/**
 * _ibus_get_engines:
 *
 * Implement the "Engines" method call of the org.freedesktop.IBus interface.
 */
static GVariant *
_ibus_get_engines (BusIBusImpl     *ibus,
                   GDBusConnection *connection,
                   GError         **error)
{
    GVariant *retval;
    GVariantBuilder builder;
    GList *engines = NULL;
    GList *p;

    g_variant_builder_init (&builder, G_VARIANT_TYPE ("av"));

    engines = g_hash_table_get_values (ibus->engine_table);

    for (p = engines; p != NULL; p = p->next) {
        g_variant_builder_add (
                &builder, "v",
                ibus_serializable_serialize ((IBusSerializable *) p->data));
    }

    g_list_free (engines);

    retval = g_variant_builder_end (&builder);
    g_assert (retval);
    return retval;
}

static void
_ibus_list_engines_depre (BusIBusImpl           *ibus,
                          GVariant              *parameters,
                          GDBusMethodInvocation *invocation)
{
    GDBusConnection *connection =
            g_dbus_method_invocation_get_connection (invocation);
    GError *error = NULL;
    GVariant *variant = NULL;

    g_warning ("org.freedesktop.IBus.ListEngines() is deprecated!");

    variant = _ibus_get_engines (ibus, connection, &error);

    g_dbus_method_invocation_return_value (invocation,
                                           g_variant_new ("(@av)", variant));
}

/**
 * _ibus_get_engines_by_names:
 *
 * Implement the "GetEnginesByNames" method call of the org.freedesktop.IBus
 * interface.
 */
static void
_ibus_get_engines_by_names (BusIBusImpl           *ibus,
                            GVariant              *parameters,
                            GDBusMethodInvocation *invocation)
{
    const gchar **names = NULL;
    g_variant_get (parameters, "(^a&s)", &names);

    g_assert (names != NULL);

    gint i = 0;
    GVariantBuilder builder;
    g_variant_builder_init (&builder, G_VARIANT_TYPE ("av"));
    while (names[i] != NULL) {
        IBusEngineDesc *desc = (IBusEngineDesc *) g_hash_table_lookup (
                ibus->engine_table, names[i++]);
        if (desc == NULL)
            continue;
        g_variant_builder_add (
                &builder,
                "v",
                ibus_serializable_serialize ((IBusSerializable *)desc));
    }
    g_dbus_method_invocation_return_value (invocation,
                                           g_variant_new ("(av)", &builder));
}

/**
 * _ibus_get_active_engines:
 *
 * Implement the "ActiveEngines" method call of the
 * org.freedesktop.IBus interface.
 */
static GVariant *
_ibus_get_active_engines (BusIBusImpl     *ibus,
                          GDBusConnection *connection,
                          GError         **error)
{
    GVariant *retval;
    GVariantBuilder builder;
    GList *p;

    g_variant_builder_init (&builder, G_VARIANT_TYPE ("av"));

    for (p = ibus->register_engine_list; p != NULL; p = p->next) {
        g_variant_builder_add (
                &builder, "v",
                ibus_serializable_serialize ((IBusSerializable *) p->data));
    }

    retval = g_variant_builder_end (&builder);
    g_assert (retval);
    return retval;
}

static void
_ibus_list_active_engines_depre (BusIBusImpl           *ibus,
                                 GVariant              *parameters,
                                 GDBusMethodInvocation *invocation)
{
    GDBusConnection *connection =
            g_dbus_method_invocation_get_connection (invocation);
    GError *error = NULL;
    GVariant *variant = NULL;

    g_warning ("org.freedesktop.IBus.ListActiveEngines() is deprecated!");

    variant = _ibus_get_active_engines (ibus, connection, &error);

    g_dbus_method_invocation_return_value (invocation,
                                           g_variant_new ("(@av)", variant));
}

/**
 * _ibus_exit:
 *
 * Implement the "Exit" method call of the org.freedesktop.IBus interface.
 */
static void
_ibus_exit (BusIBusImpl           *ibus,
            GVariant              *parameters,
            GDBusMethodInvocation *invocation)
{
    gboolean restart = FALSE;
    g_variant_get (parameters, "(b)", &restart);

    g_dbus_method_invocation_return_value (invocation, NULL);

    /* Make sure the reply has been sent out before exit */
    g_dbus_connection_flush_sync (
            g_dbus_method_invocation_get_connection (invocation),
            NULL,
            NULL);

    bus_server_quit (restart);
}

/**
 * _ibus_ping:
 *
 * Implement the "Ping" method call of the org.freedesktop.IBus interface.
 */
static void
_ibus_ping (BusIBusImpl           *ibus,
            GVariant              *parameters,
            GDBusMethodInvocation *invocation)
{
    g_dbus_method_invocation_return_value (invocation, parameters);
}

/**
 * _ibus_get_use_sys_layout:
 *
 * Implement the "UseSysLayout" method call of the
 * org.freedesktop.IBus interface.
 */
static GVariant *
_ibus_get_use_sys_layout (BusIBusImpl     *ibus,
                          GDBusConnection *connection,
                          GError         **error)
{
    if (error)
        *error = NULL;
    return g_variant_new_boolean (ibus->use_sys_layout);
}

static void
_ibus_get_use_sys_layout_depre (BusIBusImpl           *ibus,
                                GVariant              *parameters,
                                GDBusMethodInvocation *invocation)
{
    GDBusConnection *connection =
            g_dbus_method_invocation_get_connection (invocation);
    GError *error = NULL;
    GVariant *variant = NULL;

    g_warning ("org.freedesktop.IBus.GetUseSysLayout() is deprecated!");

    variant = _ibus_get_use_sys_layout (ibus, connection, &error);

    g_dbus_method_invocation_return_value (invocation,
            g_variant_new ("(b)", g_variant_get_boolean (variant)));

    g_variant_unref (variant);
}

/**
 * _ibus_get_use_global_engine:
 *
 * Implement the "UseGlobalEngine" method call of the
 * org.freedesktop.IBus interface.
 */
static GVariant *
_ibus_get_use_global_engine (BusIBusImpl     *ibus,
                             GDBusConnection *connection,
                             GError         **error)
{
    if (error)
        *error = NULL;
    return g_variant_new_boolean (ibus->use_global_engine);
}

static void
_ibus_get_use_global_engine_depre (BusIBusImpl           *ibus,
                                   GVariant              *parameters,
                                   GDBusMethodInvocation *invocation)
{
    GDBusConnection *connection =
            g_dbus_method_invocation_get_connection (invocation);
    GError *error = NULL;
    GVariant *variant = NULL;

    g_warning ("org.freedesktop.IBus.GetUseGlobalEngine() is deprecated!");

    variant = _ibus_get_use_global_engine (ibus, connection, &error);

    g_dbus_method_invocation_return_value (invocation,
            g_variant_new ("(b)", g_variant_get_boolean (variant)));

    g_variant_unref (variant);
}

/**
 * _ibus_get_global_engine:
 *
 * Implement the "GlobalEngine" method call of the
 * org.freedesktop.IBus interface.
 */
static GVariant *
_ibus_get_global_engine (BusIBusImpl     *ibus,
                         GDBusConnection *connection,
                         GError         **error)
{
    IBusEngineDesc *desc = NULL;
    GVariant *retval = NULL;

    do {
        if (!ibus->use_global_engine)
            break;
        BusInputContext *context = ibus->focused_context;
        if (context == NULL)
            context = ibus->fake_context;

        desc = bus_input_context_get_engine_desc (context);

        if (desc == NULL)
            break;

        GVariant *variant = ibus_serializable_serialize (
                (IBusSerializable *) desc);
        /* Set type "v" for introspection_xml. */
        retval = g_variant_new_variant (variant);
        if (!retval) {
            g_set_error (error,
                         G_DBUS_ERROR, G_DBUS_ERROR_FAILED,
                         "Failed to serialize engine desc.");
        }
        return retval;
    } while (0);

    g_set_error (error,
                 G_DBUS_ERROR, G_DBUS_ERROR_FAILED,
                 "No global engine.");
    return NULL;
}

static void
_ibus_get_global_engine_depre (BusIBusImpl           *ibus,
                               GVariant              *parameters,
                               GDBusMethodInvocation *invocation)
{
    GDBusConnection *connection =
            g_dbus_method_invocation_get_connection (invocation);
    GError *error = NULL;
    GVariant *variant = NULL;

    g_warning ("org.freedesktop.IBus.GetGlobalEngine() is deprecated!");

    variant = _ibus_get_global_engine (ibus, connection, &error);

    if (variant == NULL) {
        g_dbus_method_invocation_return_error (
                invocation,
                G_DBUS_ERROR, G_DBUS_ERROR_FAILED,
                "%s", error->message);
        g_error_free (error);
    } else {
        GVariant *retval = g_variant_get_variant (variant);
        g_dbus_method_invocation_return_value (invocation,
                g_variant_new ("(v)", retval));
        g_variant_unref (variant);
    }
}

struct _SetGlobalEngineData {
    BusIBusImpl *ibus;
    GDBusMethodInvocation *invocation;
};
typedef struct _SetGlobalEngineData SetGlobalEngineData;

static void
_ibus_set_global_engine_ready_cb (BusInputContext       *context,
                                  GAsyncResult          *res,
                                  SetGlobalEngineData   *data)
{
    BusIBusImpl *ibus = data->ibus;

    GError *error = NULL;
    if (!bus_input_context_set_engine_by_desc_finish (context, res, &error)) {
        g_dbus_method_invocation_return_error (data->invocation,
                                               G_DBUS_ERROR,
                                               G_DBUS_ERROR_FAILED,
                                               "Set global engine failed: %s",
                                               error->message);
        g_error_free (error);
    }
    else {
        g_dbus_method_invocation_return_value (data->invocation, NULL);

        BusEngineProxy *engine = bus_input_context_get_engine (context);
        if (ibus->use_global_engine && (context != ibus->focused_context)) {
            /* context and ibus->focused_context don't match. This means that
             * the focus is moved before _ibus_set_global_engine() asynchronous
             * call finishes. In this case, the engine for the context currently
             * being focused hasn't been updated. Update the engine here so that
             * subsequent _ibus_get_global_engine() call could return a
             * consistent engine name. */
            if (engine && ibus->focused_context != NULL) {
                g_object_ref (engine);
                bus_input_context_set_engine (context, NULL);
                bus_input_context_set_emoji_extension (context, NULL);
                bus_input_context_set_engine (ibus->focused_context, engine);
                if (ibus->enable_emoji_extension) {
                    bus_input_context_set_emoji_extension (
                            ibus->focused_context,
                            ibus->emoji_extension);
                } else {
                    bus_input_context_set_emoji_extension (
                            ibus->focused_context,
                            NULL);
                }
                g_object_unref (engine);
            }
        }
        if (engine && ibus->extension_register_keys) {
            bus_engine_proxy_panel_extension_register_keys (
                    engine,
                    ibus->extension_register_keys);
        }
    }

    g_object_unref (ibus);
    g_slice_free (SetGlobalEngineData, data);
}

/**
 * _ibus_set_global_engine:
 *
 * Implement the "SetGlobalEngine" method call of the
 * org.freedesktop.IBus interface.
 */
static void
_ibus_set_global_engine (BusIBusImpl           *ibus,
                         GVariant              *parameters,
                         GDBusMethodInvocation *invocation)
{
    if (!ibus->use_global_engine) {
        g_dbus_method_invocation_return_error (invocation,
                        G_DBUS_ERROR, G_DBUS_ERROR_FAILED,
                        "Global engine feature is disabled.");
        return;
    }

    BusInputContext *context = ibus->focused_context;
    if (context == NULL)
        context = ibus->fake_context;

    const gchar *engine_name = NULL;
    g_variant_get (parameters, "(&s)", &engine_name);

    IBusEngineDesc *desc = bus_ibus_impl_get_engine_desc(ibus, engine_name);
    if (desc == NULL) {
        g_dbus_method_invocation_return_error (invocation,
                                               G_DBUS_ERROR,
                                               G_DBUS_ERROR_FAILED,
                                               "Cannot find engine %s.",
                                               engine_name);
        return;
    }

    SetGlobalEngineData *data = g_slice_new0 (SetGlobalEngineData);
    data->ibus = g_object_ref (ibus);
    data->invocation = invocation;
    bus_input_context_set_engine_by_desc (
            context,
            desc,
            g_gdbus_timeout, /* timeout in msec. */
            NULL, /* we do not cancel the call. */
            (GAsyncReadyCallback) _ibus_set_global_engine_ready_cb,
            data);
}

/**
 * _ibus_get_global_engine_enabled:
 *
 * Implement the "GlobalEngineEnabled" method call of the
 * org.freedesktop.IBus interface.
 */
static GVariant *
_ibus_get_global_engine_enabled (BusIBusImpl     *ibus,
                                 GDBusConnection *connection,
                                 GError         **error)
{
    gboolean enabled = FALSE;

    if (error)
        *error = NULL;
    do {
        if (!ibus->use_global_engine)
            break;

        BusInputContext *context = ibus->focused_context;
        if (context == NULL)
            context = ibus->fake_context;
        if (context == NULL)
            break;

        enabled = TRUE;
    } while (0);

    return g_variant_new_boolean (enabled);
}

static void
_ibus_is_global_engine_enabled_depre (BusIBusImpl           *ibus,
                                      GVariant              *parameters,
                                      GDBusMethodInvocation *invocation)
{
    GDBusConnection *connection =
            g_dbus_method_invocation_get_connection (invocation);
    GError *error = NULL;
    GVariant *variant = NULL;

    g_warning ("org.freedesktop.IBus.IsGlobalEngineEnabled() is deprecated!");

    variant = _ibus_get_global_engine_enabled (ibus, connection, &error);

    g_dbus_method_invocation_return_value (invocation,
            g_variant_new ("(b)", g_variant_get_boolean (variant)));

    g_variant_unref (variant);
}

/**
 * _ibus_set_preload_engines:
 *
 * Implement the "PreloadEngines" method call of the
 * org.freedesktop.IBus interface.
 */
static gboolean
_ibus_set_preload_engines (BusIBusImpl     *ibus,
                           GDBusConnection *connection,
                           GVariant        *value,
                           GError         **error)
{
    int i, j;
    const gchar **names = NULL;
    IBusEngineDesc *desc = NULL;
    BusComponent *component = NULL;
    BusFactoryProxy *factory = NULL;
    GPtrArray *array = g_ptr_array_new ();

    g_variant_get (value, "^a&s", &names);

    for (i = 0; names[i] != NULL; i++) {
        gboolean has_component = FALSE;

        desc = bus_ibus_impl_get_engine_desc(ibus, names[i]);

        if (desc == NULL) {
            g_set_error (error,
                         G_DBUS_ERROR,
                         G_DBUS_ERROR_FAILED,
                         "Cannot find engine %s.",
                         names[i]);
            g_ptr_array_free (array, FALSE);
            return FALSE;
        }

        component = bus_component_from_engine_desc (desc);
        factory = bus_component_get_factory (component);

        if (factory != NULL) {
            continue;
        }

        for (j = 0; j < array->len; j++) {
            if (component == (BusComponent *) g_ptr_array_index (array, j)) {
                has_component = TRUE;
                break;
            }
        }

        if (!has_component) {
            g_ptr_array_add (array, component);
        }
    }

    for (j = 0; j < array->len; j++) {
        bus_component_start ((BusComponent *) g_ptr_array_index (array, j),
                             g_verbose);
    }

    g_ptr_array_free (array, FALSE);

    bus_ibus_impl_property_changed (ibus, "PreloadEngines", value);

    return TRUE;
}

/**
 * _ibus_get_embed_preedit_text:
 *
 * Implement the "EmbedPreeditText" method call of
 * the org.freedesktop.IBus interface.
 */
static GVariant *
_ibus_get_embed_preedit_text (BusIBusImpl     *ibus,
                              GDBusConnection *connection,
                              GError         **error)
{
    GVariant *retval = g_variant_new_boolean (ibus->embed_preedit_text);
    g_assert (retval);
    return retval;
}

/**
 * _ibus_set_embed_preedit_text:
 *
 * Implement the "EmbedPreeditText" method call of
 * the org.freedesktop.IBus interface.
 */
static gboolean
_ibus_set_embed_preedit_text (BusIBusImpl     *ibus,
                              GDBusConnection *connection,
                              GVariant        *value,
                              GError         **error)
{
    gboolean embed_preedit_text = g_variant_get_boolean (value);
    if (embed_preedit_text != ibus->embed_preedit_text) {
        ibus->embed_preedit_text = embed_preedit_text;
        bus_ibus_impl_property_changed (ibus, "EmbedPreeditText", value);
    }

    return TRUE;
}

/**
 * bus_ibus_impl_service_method_call:
 *
 * Handle a D-Bus method call whose destination and interface name are
 * both "org.freedesktop.IBus"
 */
static void
bus_ibus_impl_service_method_call (IBusService           *service,
                                   GDBusConnection       *connection,
                                   const gchar           *sender,
                                   const gchar           *object_path,
                                   const gchar           *interface_name,
                                   const gchar           *method_name,
                                   GVariant              *parameters,
                                   GDBusMethodInvocation *invocation)
{
    if (g_strcmp0 (interface_name, IBUS_INTERFACE_IBUS) != 0) {
        IBUS_SERVICE_CLASS (bus_ibus_impl_parent_class)->service_method_call (
                        service, connection, sender, object_path,
                        interface_name, method_name,
                        parameters, invocation);
        return;
    }

    /* all methods in the xml definition above should be listed here. */
    static const struct {
        const gchar *method_name;
        void (* method_callback) (BusIBusImpl *,
                                  GVariant *,
                                  GDBusMethodInvocation *);
    } methods [] =  {
        /* IBus interface */
        { "CreateInputContext",    _ibus_create_input_context },
        { "RegisterComponent",     _ibus_register_component },
        { "GetEnginesByNames",     _ibus_get_engines_by_names },
        { "Exit",                  _ibus_exit },
        { "Ping",                  _ibus_ping },
        { "SetGlobalEngine",       _ibus_set_global_engine },
        /* Start of deprecated methods */
        { "GetAddress",            _ibus_get_address_depre },
        { "CurrentInputContext",   _ibus_current_input_context_depre },
        { "ListEngines",           _ibus_list_engines_depre },
        { "ListActiveEngines",     _ibus_list_active_engines_depre },
        { "GetUseSysLayout",       _ibus_get_use_sys_layout_depre },
        { "GetUseGlobalEngine",    _ibus_get_use_global_engine_depre },
        { "GetGlobalEngine",       _ibus_get_global_engine_depre },
        { "IsGlobalEngineEnabled", _ibus_is_global_engine_enabled_depre },
        /* End of deprecated methods */
    };

    gint i;
    for (i = 0; i < G_N_ELEMENTS (methods); i++) {
        if (g_strcmp0 (methods[i].method_name, method_name) == 0) {
            methods[i].method_callback ((BusIBusImpl *) service,
                                        parameters,
                                        invocation);
            return;
        }
    }

    g_warning ("service_method_call received an unknown method: %s",
               method_name ? method_name : "(null)");
}

/**
 * bus_ibus_impl_service_get_property:
 *
 * Handle org.freedesktop.DBus.Properties.Get
 */
static GVariant *
bus_ibus_impl_service_get_property (IBusService     *service,
                                    GDBusConnection *connection,
                                    const gchar     *sender,
                                    const gchar     *object_path,
                                    const gchar     *interface_name,
                                    const gchar     *property_name,
                                    GError         **error)
{
    gint i;

    static const struct {
        const gchar *method_name;
        GVariant * (* method_callback) (BusIBusImpl *,
                                        GDBusConnection *,
                                        GError **);
    } methods [] =  {
        { "Address",               _ibus_get_address },
        { "CurrentInputContext",   _ibus_get_current_input_context },
        { "Engines",               _ibus_get_engines },
        { "ActiveEngines",         _ibus_get_active_engines },
        { "GlobalEngine",          _ibus_get_global_engine },
        { "EmbedPreeditText",      _ibus_get_embed_preedit_text },
    };

    if (error)
        *error = NULL;
    if (g_strcmp0 (interface_name, IBUS_INTERFACE_IBUS) != 0) {
        return IBUS_SERVICE_CLASS (
                bus_ibus_impl_parent_class)->service_get_property (
                        service, connection, sender, object_path,
                        interface_name, property_name,
                        error);
    }

    for (i = 0; i < G_N_ELEMENTS (methods); i++) {
        if (g_strcmp0 (methods[i].method_name, property_name) == 0) {
            return methods[i].method_callback ((BusIBusImpl *) service,
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

/**
 * bus_ibus_impl_service_set_property:
 *
 * Handle org.freedesktop.DBus.Properties.Set
 */
static gboolean
bus_ibus_impl_service_set_property (IBusService     *service,
                                    GDBusConnection *connection,
                                    const gchar     *sender,
                                    const gchar     *object_path,
                                    const gchar     *interface_name,
                                    const gchar     *property_name,
                                    GVariant        *value,
                                    GError         **error)
{
    gint i;

    static const struct {
        const gchar *method_name;
        gboolean (* method_callback) (BusIBusImpl *,
                                      GDBusConnection *,
                                      GVariant *,
                                      GError **);
    } methods [] =  {
        { "PreloadEngines",        _ibus_set_preload_engines },
        { "EmbedPreeditText",      _ibus_set_embed_preedit_text },
    };

    if (error)
        *error = NULL;
    if (g_strcmp0 (interface_name, IBUS_INTERFACE_IBUS) != 0) {
        return IBUS_SERVICE_CLASS (
                bus_ibus_impl_parent_class)->service_set_property (
                        service, connection, sender, object_path,
                        interface_name, property_name,
                        value, error);
    }

    for (i = 0; i < G_N_ELEMENTS (methods); i++) {
        if (g_strcmp0 (methods[i].method_name, property_name) == 0) {
            return methods[i].method_callback ((BusIBusImpl *) service,
                                               connection,
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

BusIBusImpl *
bus_ibus_impl_get_default (void)
{
    static BusIBusImpl *ibus = NULL;

    if (ibus == NULL) {
        ibus = (BusIBusImpl *) g_object_new (BUS_TYPE_IBUS_IMPL,
                                             "object-path", IBUS_PATH_IBUS,
                                             NULL);
    }
    return ibus;
}

BusFactoryProxy *
bus_ibus_impl_lookup_factory (BusIBusImpl *ibus,
                              const gchar *path)
{
    BusFactoryProxy *factory;

    g_assert (BUS_IS_IBUS_IMPL (ibus));
    factory = (BusFactoryProxy *) g_hash_table_lookup (ibus->factory_dict,
                                                       path);
    return factory;
}

IBusKeymap *
bus_ibus_impl_get_keymap (BusIBusImpl *ibus)
{
    g_assert (BUS_IS_IBUS_IMPL (ibus));

    return ibus->keymap;
}

/**
 * bus_ibus_impl_registry_init:
 *
 * Initialize IBusRegistry.
 */
static void
bus_ibus_impl_registry_init (BusIBusImpl *ibus)
{
    GList *p;
    GList *components;
    IBusRegistry *registry = ibus_registry_new ();

    ibus->registry = NULL;
    ibus->components = NULL;
    ibus->engine_table = g_hash_table_new (g_str_hash, g_str_equal);

    if (g_strcmp0 (g_cache, "none") == 0) {
        /* Only load registry, but not read and write cache. */
        ibus_registry_load (registry);
    }
    else if (g_strcmp0 (g_cache, "refresh") == 0) {
        /* Load registry and overwrite the cache. */
        ibus_registry_load (registry);
        ibus_registry_save_cache (registry, TRUE);
    }
    else if (g_strcmp0 (g_cache, "auto") == 0) {
        /* Load registry from cache. If the cache does not exist or
         * it is outdated, then generate it.
         */
        if (ibus_registry_load_cache (registry, FALSE) == FALSE ||
            ibus_registry_check_modification (registry)) {

            ibus_object_destroy (IBUS_OBJECT (registry));
            registry = ibus_registry_new ();

            if (ibus_registry_load_cache (registry, TRUE) == FALSE ||
                ibus_registry_check_modification (registry)) {

                ibus_object_destroy (IBUS_OBJECT (registry));
                registry = ibus_registry_new ();
                ibus_registry_load (registry);
                ibus_registry_save_cache (registry, TRUE);
            }
        }
    }

    ibus->registry = registry;
    components = ibus_registry_get_components (registry);

    for (p = components; p != NULL; p = p->next) {
        IBusComponent *component = (IBusComponent *) p->data;
        BusComponent *buscomp = bus_component_new (component,
                                                   NULL /* factory */);
        GList *engines = NULL;
        GList *p1;

        g_object_ref_sink (buscomp);
        ibus->components = g_list_append (ibus->components, buscomp);

        engines = bus_component_get_engines (buscomp);
        for (p1 = engines; p1 != NULL; p1 = p1->next) {
            IBusEngineDesc *desc = (IBusEngineDesc *) p1->data;
            const gchar *name = ibus_engine_desc_get_name (desc);
            if (g_hash_table_lookup (ibus->engine_table, name) == NULL) {
                g_hash_table_insert (ibus->engine_table,
                                     (gpointer) name,
                                     desc);
            } else {
                g_message ("Engine %s is already registered by other component",
                           name);
            }
        }
        g_list_free (engines);
    }

    g_list_free (components);

    g_signal_connect (ibus->registry,
                      "changed",
                      G_CALLBACK (_registry_changed_cb),
                      ibus);
    ibus_registry_start_monitor_changes (ibus->registry);
}

static void
bus_ibus_impl_registry_destroy (BusIBusImpl *ibus)
{
    g_list_free_full (ibus->components, g_object_unref);
    ibus->components = NULL;

    g_clear_pointer (&ibus->engine_table, g_hash_table_destroy);

    /* g_clear_pointer() does not set the cast. */
    ibus_object_destroy (IBUS_OBJECT (ibus->registry));
    ibus->registry = NULL;

    if (ibus->extension_register_keys)
        g_clear_pointer (&ibus->extension_register_keys, g_variant_unref);
}

static gint
_component_is_name_cb (BusComponent *component,
                       const gchar  *name)
{
    g_assert (BUS_IS_COMPONENT (component));
    g_assert (name);

    return g_strcmp0 (bus_component_get_name (component), name);
}

static void
bus_ibus_impl_component_name_owner_changed (BusIBusImpl *ibus,
                                            const gchar *name,
                                            const gchar *old_name,
                                            const gchar *new_name)
{
    BusComponent *component;
    BusFactoryProxy *factory;

    g_assert (BUS_IS_IBUS_IMPL (ibus));
    g_assert (name);
    g_assert (old_name);
    g_assert (new_name);

    component = bus_ibus_impl_lookup_component_by_name (ibus, name);

    if (component == NULL) {
        /* name is a unique name, or a well-known name we don't know. */
        return;
    }

    if (g_strcmp0 (old_name, "") != 0) {
        /* the component is stopped. */
        factory = bus_component_get_factory (component);

        if (factory != NULL) {
            ibus_proxy_destroy ((IBusProxy *) factory);
        }
    }

    if (g_strcmp0 (new_name, "") != 0) {
        /* the component is started. */
        BusConnection *connection =
                bus_dbus_impl_get_connection_by_name (BUS_DEFAULT_DBUS,
                                                      new_name);
        if (connection == NULL)
            return;

        factory = bus_factory_proxy_new (connection);
        if (factory == NULL)
            return;
        bus_component_set_factory (component, factory);
        g_object_unref (factory);
    }
}

BusComponent *
bus_ibus_impl_lookup_component_by_name (BusIBusImpl *ibus,
                                        const gchar *name)
{
    GList *p;

    g_assert (BUS_IS_IBUS_IMPL (ibus));
    g_assert (name);

    p = g_list_find_custom (ibus->components,
                            name,
                            (GCompareFunc) _component_is_name_cb);
    if (p) {
        return (BusComponent *) p->data;
    }
    else {
        return NULL;
    }
}

/**
 * bus_ibus_impl_property_changed
 *
 * Send a "PropertiesChanged" D-Bus signal for a property to buses
 * (connections) that are listening to the signal.
 */
static void
bus_ibus_impl_property_changed (BusIBusImpl *service,
                                const gchar *property_name,
                                GVariant    *value)
{
    GDBusMessage *message =
        g_dbus_message_new_signal ("/org/freedesktop/IBus",
                                   "org.freedesktop.DBus.Properties",
                                   "PropertiesChanged");

    /* set a non-zero serial to make libdbus happy */
    g_dbus_message_set_serial (message, 1);
    g_dbus_message_set_sender (message, "org.freedesktop.IBus");

    GVariantBuilder *builder = g_variant_builder_new (G_VARIANT_TYPE_ARRAY);
    g_variant_builder_add (builder, "{sv}", property_name, value);
    g_dbus_message_set_body (message,
                             g_variant_new ("(sa{sv}as)",
                                            "org.freedesktop.IBus",
                                            builder,
                                            NULL));
    g_variant_builder_unref (builder);

    bus_dbus_impl_dispatch_message_by_rule (BUS_DEFAULT_DBUS, message, NULL);
    g_object_unref (message);
}

/**
 * bus_ibus_impl_emit_signal:
 *
 * Send a D-Bus signal to buses (connections) that are listening to the signal.
 */
static void
bus_ibus_impl_emit_signal (BusIBusImpl *ibus,
                           const gchar *signal_name,
                           GVariant    *parameters)
{
    GDBusMessage *message = g_dbus_message_new_signal ("/org/freedesktop/IBus",
                                                       "org.freedesktop.IBus",
                                                       signal_name);
    /* set a non-zero serial to make libdbus happy */
    g_dbus_message_set_serial (message, 1);
    g_dbus_message_set_sender (message, "org.freedesktop.IBus");
    if (parameters)
        g_dbus_message_set_body (message, parameters);
    bus_dbus_impl_dispatch_message_by_rule (BUS_DEFAULT_DBUS, message, NULL);
    g_object_unref (message);
}

static void
bus_ibus_impl_registry_changed (BusIBusImpl *ibus)
{
    bus_ibus_impl_emit_signal (ibus, "RegistryChanged", NULL);
}

static void
bus_ibus_impl_global_engine_changed (BusIBusImpl *ibus)
{
    const gchar *name = ibus->global_engine_name ? ibus->global_engine_name : "";
    bus_ibus_impl_emit_signal (ibus, "GlobalEngineChanged",
                               g_variant_new ("(s)", name));
}

gboolean
bus_ibus_impl_is_use_sys_layout (BusIBusImpl *ibus)
{
    g_assert (BUS_IS_IBUS_IMPL (ibus));

    return ibus->use_sys_layout;
}

gboolean
bus_ibus_impl_is_embed_preedit_text (BusIBusImpl *ibus)
{
    g_assert (BUS_IS_IBUS_IMPL (ibus));

    return ibus->embed_preedit_text;
}

BusInputContext *
bus_ibus_impl_get_focused_input_context (BusIBusImpl *ibus)
{
    g_assert (BUS_IS_IBUS_IMPL (ibus));

    return ibus->focused_context;
}

GHashTable *
bus_ibus_impl_get_engine_focus_id_table (BusIBusImpl *ibus)
{

    g_assert (BUS_IS_IBUS_IMPL (ibus));

    return ibus->engine_focus_id_table;
}

GHashTable *
bus_ibus_impl_get_engine_active_surrounding_text_table (BusIBusImpl *ibus)
{

    g_assert (BUS_IS_IBUS_IMPL (ibus));

    return ibus->engine_active_surrounding_text_table;
}
