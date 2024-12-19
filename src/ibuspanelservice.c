/* -*- mode: C; c-basic-offset: 4; indent-tabs-mode: nil; -*- */
/* vim:set et sts=4: */
/* ibus - The Input Bus
 * Copyright (c) 2009-2014 Google Inc. All rights reserved.
 * Copyright (C) 2010-2014 Peng Huang <shawn.p.huang@gmail.com>
 * Copyright (C) 2017-2024 Takao Fujiwara <takao.fujiwara1@gmail.com>
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
#include "ibusshare.h"
#include "ibuspanelservice.h"
#include "ibusmarshalers.h"
#include "ibusinternal.h"

#define IBUS_PANEL_SERVICE_GET_PRIVATE(o)  \
   (G_TYPE_INSTANCE_GET_PRIVATE ((o), IBUS_TYPE_PANEL_SERVICE, \
                                 IBusPanelServicePrivate))

enum {
    UPDATE_PREEDIT_TEXT,
    UPDATE_AUXILIARY_TEXT,
    UPDATE_LOOKUP_TABLE,
    FOCUS_IN,
    FOCUS_OUT,
    REGISTER_PROPERTIES,
    UPDATE_PROPERTY,
    SET_CURSOR_LOCATION,
    SET_CURSOR_LOCATION_RELATIVE,
    CURSOR_UP_LOOKUP_TABLE,
    CURSOR_DOWN_LOOKUP_TABLE,
    HIDE_AUXILIARY_TEXT,
    HIDE_LANGUAGE_BAR,
    HIDE_LOOKUP_TABLE,
    HIDE_PREEDIT_TEXT,
    PAGE_UP_LOOKUP_TABLE,
    PAGE_DOWN_LOOKUP_TABLE,
    RESET,
    SHOW_AUXILIARY_TEXT,
    SHOW_LANGUAGE_BAR,
    SHOW_LOOKUP_TABLE,
    SHOW_PREEDIT_TEXT,
    START_SETUP,
    STATE_CHANGED,
    DESTROY_CONTEXT,
    SET_CONTENT_TYPE,
    PANEL_EXTENSION_RECEIVED,
    PROCESS_KEY_EVENT,
    COMMIT_TEXT_RECEIVED,
    CANDIDATE_CLICKED_LOOKUP_TABLE,
    LAST_SIGNAL,
};

enum {
    PROP_0,
};

static guint            panel_signals[LAST_SIGNAL] = { 0 };

/* functions prototype */
static void      ibus_panel_service_set_property
                                   (IBusPanelService       *panel,
                                    guint                   prop_id,
                                    const GValue           *value,
                                    GParamSpec             *pspec);
static void      ibus_panel_service_get_property
                                   (IBusPanelService       *panel,
                                    guint                   prop_id,
                                    GValue                 *value,
                                    GParamSpec             *pspec);
static void      ibus_panel_service_real_destroy
                                   (IBusPanelService       *panel);
static void      ibus_panel_service_service_method_call
                                   (IBusService            *service,
                                    GDBusConnection        *connection,
                                    const gchar            *sender,
                                    const gchar            *object_path,
                                    const gchar            *interface_name,
                                    const gchar            *method_name,
                                    GVariant               *parameters,
                                    GDBusMethodInvocation  *invocation);
static GVariant *ibus_panel_service_service_get_property
                                   (IBusService            *service,
                                    GDBusConnection        *connection,
                                    const gchar            *sender,
                                    const gchar            *object_path,
                                    const gchar            *interface_name,
                                    const gchar            *property_name,
                                    GError                **error);
static gboolean  ibus_panel_service_service_set_property
                                   (IBusService            *service,
                                    GDBusConnection        *connection,
                                    const gchar            *sender,
                                    const gchar            *object_path,
                                    const gchar            *interface_name,
                                    const gchar            *property_name,
                                    GVariant               *value,
                                    GError                **error);
static void      ibus_panel_service_not_implemented
                                   (IBusPanelService       *panel);
static void      ibus_panel_service_focus_in
                                   (IBusPanelService       *panel,
                                    const gchar            *input_context_path);
static void      ibus_panel_service_focus_out
                                   (IBusPanelService       *panel,
                                    const gchar            *input_context_path);
static void      ibus_panel_service_destroy_context
                                   (IBusPanelService       *panel,
                                    const gchar            *input_context_path);
static void      ibus_panel_service_register_properties
                                   (IBusPanelService       *panel,
                                    IBusPropList           *prop_list);
static void      ibus_panel_service_set_cursor_location
                                   (IBusPanelService       *panel,
                                    gint                    x,
                                    gint                    y,
                                    gint                    w,
                                    gint                    h);
static void      ibus_panel_service_set_cursor_location_relative
                                   (IBusPanelService       *panel,
                                    gint                    x,
                                    gint                    y,
                                    gint                    w,
                                    gint                    h);
static void      ibus_panel_service_update_auxiliary_text
                                   (IBusPanelService       *panel,
                                    IBusText               *text,
                                    gboolean                visible);
static void      ibus_panel_service_update_lookup_table
                                   (IBusPanelService       *panel,
                                    IBusLookupTable        *lookup_table,
                                    gboolean                visible);
static void      ibus_panel_service_update_preedit_text
                                   (IBusPanelService       *panel,
                                    IBusText               *text,
                                    guint                   cursor_pos,
                                    gboolean                visible);
static void      ibus_panel_service_update_property
                                   (IBusPanelService       *panel,
                                    IBusProperty           *prop);
static void      ibus_panel_service_set_content_type
                                   (IBusPanelService       *panel,
                                    guint                   purpose,
                                    guint                   hints);
static void      ibus_panel_service_panel_extension_received
                                   (IBusPanelService       *panel,
                                    IBusExtensionEvent     *event);

G_DEFINE_TYPE (IBusPanelService, ibus_panel_service, IBUS_TYPE_SERVICE)

static const gchar introspection_xml[] =
    "<node>"
    "  <interface name='org.freedesktop.IBus.Panel'>"
    /* Methods */
    "    <method name='UpdatePreeditText'>"
    "      <arg direction='in'  type='v' name='text' />"
    "      <arg direction='in'  type='u' name='cursor_pos' />"
    "      <arg direction='in'  type='b' name='visible' />"
    "    </method>"
    "    <method name='ShowPreeditText' />"
    "    <method name='HidePreeditText' />"
    "    <method name='UpdateAuxiliaryText'>"
    "      <arg direction='in'  type='v' name='text' />"
    "      <arg direction='in'  type='b' name='visible' />"
    "    </method>"
    "    <method name='ShowAuxiliaryText' />"
    "    <method name='HideAuxiliaryText' />"
    "    <method name='UpdateLookupTable'>"
    "      <arg direction='in' type='v' name='table' />"
    "      <arg direction='in' type='b' name='visible' />"
    "    </method>"
    "    <method name='ShowLookupTable' />"
    "    <method name='HideLookupTable' />"
    "    <method name='CursorUpLookupTable' />"
    "    <method name='CursorDownLookupTable' />"
    "    <method name='PageUpLookupTable' />"
    "    <method name='PageDownLookupTable' />"
    "    <method name='CandidateClickedLookupTable'>"
    "      <arg direction='in' type='u' name='index' />"
    "      <arg direction='in' type='u' name='button' />"
    "      <arg direction='in' type='u' name='state' />"
    "    </method>"
    "    <method name='RegisterProperties'>"
    "      <arg direction='in'  type='v' name='props' />"
    "    </method>"
    "    <method name='UpdateProperty'>"
    "      <arg direction='in'  type='v' name='prop' />"
    "    </method>"
    "    <method name='FocusIn'>"
    "      <arg direction='in'  type='o' name='ic' />"
    "    </method>"
    "    <method name='FocusOut'>"
    "      <arg direction='in'  type='o' name='ic' />"
    "    </method>"
    "    <method name='DestroyContext'>"
    "      <arg direction='in'  type='o' name='ic' />"
    "    </method>"
    "    <method name='SetCursorLocation'>"
    "      <arg direction='in' type='i' name='x' />"
    "      <arg direction='in' type='i' name='y' />"
    "      <arg direction='in' type='i' name='w' />"
    "      <arg direction='in' type='i' name='h' />"
    "    </method>"
    "    <method name='SetCursorLocationRelative'>"
    "      <arg direction='in' type='i' name='x' />"
    "      <arg direction='in' type='i' name='y' />"
    "      <arg direction='in' type='i' name='w' />"
    "      <arg direction='in' type='i' name='h' />"
    "    </method>"
    "    <method name='Reset' />"
    "    <method name='StartSetup' />"
    "    <method name='StateChanged' />"
    "    <method name='HideLanguageBar' />"
    "    <method name='ShowLanguageBar' />"
    "    <method name='ContentType'>"
    "      <arg direction='in'  type='u' name='purpose' />"
    "      <arg direction='in'  type='u' name='hints' />"
    "    </method>"
    "    <method name='PanelExtensionReceived'>"
    "      <arg direction='in' type='v' name='event' />"
    "    </method>"
    "    <method name='ProcessKeyEvent'>"
    "      <arg direction='in'  type='u' name='keyval' />"
    "      <arg direction='in'  type='u' name='keycode' />"
    "      <arg direction='in'  type='u' name='state' />"
    "      <arg direction='out' type='b' />"
    "    </method>"
    "    <method name='CommitTextReceived'>"
    "      <arg direction='in' type='v' name='text' />"
    "    </method>"
    /* Signals */
    "    <signal name='CursorUp' />"
    "    <signal name='CursorDown' />"
    "    <signal name='PageUp' />"
    "    <signal name='PageDown' />"
    "    <signal name='PropertyActivate'>"
    "      <arg type='s' name='prop_name' />"
    "      <arg type='i' name='prop_state' />"
    "    </signal>"
    "    <signal name='PropertyShow'>"
    "      <arg type='s' name='prop_name' />"
    "    </signal>"
    "    <signal name='PropertyHide'>"
    "      <arg type='s' name='prop_name' />"
    "    </signal>"
    "    <signal name='CandidateClicked'>"
    "      <arg type='u' name='index' />"
    "      <arg type='u' name='button' />"
    "      <arg type='u' name='state' />"
    "    </signal>"
    "    <signal name='CommitText'>"
    "      <arg type='v' name='text' />"
    "    </signal>"
    "    <signal name='PanelExtension'>"
    "      <arg type='v' name='event' />"
    "    </signal>"
    "    <method name='PanelExtensionRegisterKeys'>"
    "      <arg type='v' name='data' />"
    "    </method>"
    "    <signal name='UpdatePreeditTextReceived'>"
    "      <arg type='v' name='text' />"
    "      <arg type='u' name='cursor_pos' />"
    "      <arg type='b' name='visible' />"
    "    </signal>"
    "    <signal name='UpdateAuxiliaryTextReceived'>"
    "      <arg type='v' name='text' />"
    "      <arg type='b' name='visible' />"
    "    </signal>"
    "    <signal name='UpdateLookupTableReceived'>"
    "      <arg type='v' name='table' />"
    "      <arg type='b' name='visible' />"
    "    </signal>"
    "    <signal name='ForwardProcessKeyEvent'>"
    "      <arg type='u' name='keyval' />"
    "      <arg type='u' name='keycode' />"
    "      <arg type='u' name='state' />"
    "    </signal>"
    "  </interface>"
    "</node>";

static void
ibus_panel_service_class_init (IBusPanelServiceClass *class)
{
    GObjectClass *gobject_class = G_OBJECT_CLASS (class);
    ibus_panel_service_parent_class =
            IBUS_SERVICE_CLASS (g_type_class_peek_parent (class));

    gobject_class->set_property =
            (GObjectSetPropertyFunc) ibus_panel_service_set_property;
    gobject_class->get_property =
            (GObjectGetPropertyFunc) ibus_panel_service_get_property;

    IBUS_OBJECT_CLASS (gobject_class)->destroy =
            (IBusObjectDestroyFunc) ibus_panel_service_real_destroy;

    IBUS_SERVICE_CLASS (class)->service_method_call  =
            ibus_panel_service_service_method_call;
    IBUS_SERVICE_CLASS (class)->service_get_property =
            ibus_panel_service_service_get_property;
    IBUS_SERVICE_CLASS (class)->service_set_property =
            ibus_panel_service_service_set_property;

    ibus_service_class_add_interfaces (IBUS_SERVICE_CLASS (class),
                                       introspection_xml);

    class->focus_in              = ibus_panel_service_focus_in;
    class->focus_out             = ibus_panel_service_focus_out;
    class->destroy_context       = ibus_panel_service_destroy_context;
    class->register_properties   = ibus_panel_service_register_properties;
    class->set_cursor_location   = ibus_panel_service_set_cursor_location;
    class->set_cursor_location_relative = ibus_panel_service_set_cursor_location_relative;
    class->update_lookup_table   = ibus_panel_service_update_lookup_table;
    class->update_auxiliary_text = ibus_panel_service_update_auxiliary_text;
    class->update_preedit_text   = ibus_panel_service_update_preedit_text;
    class->update_property       = ibus_panel_service_update_property;
    class->set_content_type      = ibus_panel_service_set_content_type;
    class->panel_extension_received =
            ibus_panel_service_panel_extension_received;

    class->cursor_down_lookup_table = ibus_panel_service_not_implemented;
    class->cursor_up_lookup_table   = ibus_panel_service_not_implemented;
    class->hide_auxiliary_text      = ibus_panel_service_not_implemented;
    class->hide_language_bar        = ibus_panel_service_not_implemented;
    class->hide_lookup_table        = ibus_panel_service_not_implemented;
    class->hide_preedit_text        = ibus_panel_service_not_implemented;
    class->page_down_lookup_table   = ibus_panel_service_not_implemented;
    class->page_up_lookup_table     = ibus_panel_service_not_implemented;
    class->reset                    = ibus_panel_service_not_implemented;
    class->show_auxiliary_text      = ibus_panel_service_not_implemented;
    class->show_language_bar        = ibus_panel_service_not_implemented;
    class->show_lookup_table        = ibus_panel_service_not_implemented;
    class->show_preedit_text        = ibus_panel_service_not_implemented;
    class->start_setup              = ibus_panel_service_not_implemented;
    class->state_changed            = ibus_panel_service_not_implemented;

    /* install signals */
    /**
     * IBusPanelService::update-preedit-text:
     * @panel: An #IBusPanelService
     * @text: A preedit text to be updated.
     * @cursor_pos: The cursor position of the text.
     * @visible: Whether the update is visible.
     *
     * Emitted when the client application get the ::update-preedit-text.
     * Implement the member function
     * IBusPanelServiceClass::update_preedit_text in extended class
     * to receive this signal.
     *
     * <note><para>Argument @user_data is ignored in this function.</para>
     * </note>
     */
    panel_signals[UPDATE_PREEDIT_TEXT] =
        g_signal_new (I_("update-preedit-text"),
            G_TYPE_FROM_CLASS (gobject_class),
            G_SIGNAL_RUN_LAST,
            G_STRUCT_OFFSET (IBusPanelServiceClass, update_preedit_text),
            NULL, NULL,
            _ibus_marshal_VOID__OBJECT_UINT_BOOLEAN,
            G_TYPE_NONE,
            3,
            IBUS_TYPE_TEXT,
            G_TYPE_UINT,
            G_TYPE_BOOLEAN);

    /**
     * IBusPanelService::update-auxiliary-text:
     * @panel: An #IBusPanelService
     * @text: A preedit text to be updated.
     * @visible: Whether the update is visible.
     *
     * Emitted when the client application get the ::update-auxiliary-text.
     * Implement the member function
     * IBusPanelServiceClass::update_auxiliary_text in extended class
     * to receive this signal.
     *
     * <note><para>Argument @user_data is ignored in this function.</para>
     * </note>
     */
    panel_signals[UPDATE_AUXILIARY_TEXT] =
        g_signal_new (I_("update-auxiliary-text"),
            G_TYPE_FROM_CLASS (gobject_class),
            G_SIGNAL_RUN_LAST,
            G_STRUCT_OFFSET (IBusPanelServiceClass, update_auxiliary_text),
            NULL, NULL,
            _ibus_marshal_VOID__OBJECT_BOOLEAN,
            G_TYPE_NONE,
            2,
            IBUS_TYPE_TEXT,
            G_TYPE_BOOLEAN);

    /**
     * IBusPanelService::update-lookup-table:
     * @panel: An #IBusPanelService
     * @lookup_table: A lookup table to be updated.
     * @visible: Whether the update is visible.
     *
     * Emitted when the client application get the ::update-lookup-table.
     * Implement the member function
     * IBusPanelServiceClass::update_lookup_table in extended class
     * to receive this signal.
     *
     * <note><para>Argument @user_data is ignored in this function.</para>
     * </note>
     */
    panel_signals[UPDATE_LOOKUP_TABLE] =
        g_signal_new (I_("update-lookup-table"),
            G_TYPE_FROM_CLASS (gobject_class),
            G_SIGNAL_RUN_LAST,
            G_STRUCT_OFFSET (IBusPanelServiceClass, update_lookup_table),
            NULL, NULL,
            _ibus_marshal_VOID__OBJECT_BOOLEAN,
            G_TYPE_NONE,
            2,
            IBUS_TYPE_LOOKUP_TABLE,
            G_TYPE_BOOLEAN);

    /**
     * IBusPanelService::focus-in:
     * @panel: An #IBusPanelService
     * @input_context_path: Object path of InputContext.
     *
     * Emitted when the client application get the ::focus-in.
     * Implement the member function
     * IBusPanelServiceClass::focus_in in extended class to receive
     * this signal.
     *
     * <note><para>Argument @user_data is ignored in this function.</para>
     * </note>
     */
    panel_signals[FOCUS_IN] =
        g_signal_new (I_("focus-in"),
            G_TYPE_FROM_CLASS (gobject_class),
            G_SIGNAL_RUN_LAST,
            G_STRUCT_OFFSET (IBusPanelServiceClass, focus_in),
            NULL, NULL,
            _ibus_marshal_VOID__STRING,
            G_TYPE_NONE,
            1,
            G_TYPE_STRING);

    /**
     * IBusPanelService::focus-out:
     * @panel: An #IBusPanelService
     * @input_context_path: Object path of InputContext.
     *
     * Emitted when the client application get the ::focus-out.
     * Implement the member function
     * IBusPanelServiceClass::focus_out in extended class to receive
     * this signal.
     *
     * <note><para>Argument @user_data is ignored in this function.</para>
     * </note>
     */
    panel_signals[FOCUS_OUT] =
        g_signal_new (I_("focus-out"),
            G_TYPE_FROM_CLASS (gobject_class),
            G_SIGNAL_RUN_LAST,
            G_STRUCT_OFFSET (IBusPanelServiceClass, focus_out),
            NULL, NULL,
            _ibus_marshal_VOID__STRING,
            G_TYPE_NONE,
            1,
            G_TYPE_STRING);

    /**
     * IBusPanelService::register-properties:
     * @panel: An #IBusPanelService
     * @prop_list: An IBusPropList that contains properties.
     *
     * Emitted when the client application get the ::register-properties.
     * Implement the member function
     * IBusPanelServiceClass::register_properties in extended class
     * to receive this signal.
     *
     * <note><para>Argument @user_data is ignored in this function.</para>
     * </note>
     */
    panel_signals[REGISTER_PROPERTIES] =
        g_signal_new (I_("register-properties"),
            G_TYPE_FROM_CLASS (gobject_class),
            G_SIGNAL_RUN_LAST,
            G_STRUCT_OFFSET (IBusPanelServiceClass, register_properties),
            NULL, NULL,
            _ibus_marshal_VOID__OBJECT,
            G_TYPE_NONE,
            1,
            IBUS_TYPE_PROP_LIST);

    /**
     * IBusPanelService::update-property:
     * @panel: An #IBusPanelService
     * @prop: The IBusProperty to be updated.
     *
     * Emitted when the client application get the ::update-property.
     * Implement the member function
     * IBusPanelServiceClass::update_property in extended class to
     * receive this signal.
     *
     * <note><para>Argument @user_data is ignored in this function.</para>
     * </note>
     */
    panel_signals[UPDATE_PROPERTY] =
        g_signal_new (I_("update-property"),
            G_TYPE_FROM_CLASS (gobject_class),
            G_SIGNAL_RUN_LAST,
            G_STRUCT_OFFSET (IBusPanelServiceClass, update_property),
            NULL, NULL,
            _ibus_marshal_VOID__OBJECT,
            G_TYPE_NONE,
            1,
            IBUS_TYPE_PROPERTY);

    /**
     * IBusPanelService::set-cursor-location:
     * @panel: An #IBusPanelService
     * @x: X coordinate of the cursor.
     * @y: Y coordinate of the cursor.
     * @w: Width of the cursor.
     * @h: Height of the cursor.
     *
     * Emitted when the client application get the ::set-cursor-location.
     * Implement the member function
     * IBusPanelServiceClass::set_cursor_location in extended class
     * to receive this signal.
     *
     * <note><para>Argument @user_data is ignored in this function.</para>
     * </note>
     */
    panel_signals[SET_CURSOR_LOCATION] =
        g_signal_new (I_("set-cursor-location"),
            G_TYPE_FROM_CLASS (gobject_class),
            G_SIGNAL_RUN_LAST,
            G_STRUCT_OFFSET (IBusPanelServiceClass, set_cursor_location),
            NULL, NULL,
            _ibus_marshal_VOID__INT_INT_INT_INT,
            G_TYPE_NONE,
            4,
            G_TYPE_INT,
            G_TYPE_INT,
            G_TYPE_INT,
            G_TYPE_INT);

    /**
     * IBusPanelService::set-cursor-location-relative:
     * @panel: An #IBusPanelService
     * @x: X coordinate of the cursor.
     * @y: Y coordinate of the cursor.
     * @w: Width of the cursor.
     * @h: Height of the cursor.
     *
     * Emitted when the client application get the set-cursor-location-relative.
     * Implement the member function set_cursor_location_relative() in
     * extended class to receive this signal.
     *
     * <note><para>Argument @user_data is ignored in this function.</para>
     * </note>
     */
    panel_signals[SET_CURSOR_LOCATION_RELATIVE] =
        g_signal_new (I_("set-cursor-location-relative"),
            G_TYPE_FROM_CLASS (gobject_class),
            G_SIGNAL_RUN_LAST,
            G_STRUCT_OFFSET (IBusPanelServiceClass, set_cursor_location_relative),
            NULL, NULL,
            _ibus_marshal_VOID__INT_INT_INT_INT,
            G_TYPE_NONE,
            4,
            G_TYPE_INT,
            G_TYPE_INT,
            G_TYPE_INT,
            G_TYPE_INT);

    /**
     * IBusPanelService::cursor-up-lookup-table:
     * @panel: An #IBusPanelService
     *
     * Emitted when the client application get the ::cursor-up-lookup-table.
     * Implement the member function
     * IBusPanelServiceClass::cursor_up_lookup_table in extended
     * class to receive this signal.
     *
     * <note><para>Argument @user_data is ignored in this function.</para>
     * </note>
     */
    panel_signals[CURSOR_UP_LOOKUP_TABLE] =
        g_signal_new (I_("cursor-up-lookup-table"),
            G_TYPE_FROM_CLASS (gobject_class),
            G_SIGNAL_RUN_LAST,
            G_STRUCT_OFFSET (IBusPanelServiceClass, cursor_up_lookup_table),
            NULL, NULL,
            _ibus_marshal_VOID__VOID,
            G_TYPE_NONE, 0);

    /**
     * IBusPanelService::cursor-down-lookup-table:
     * @panel: An #IBusPanelService
     *
     * Emitted when the client application get the ::cursor-down-lookup-table.
     * Implement the member function
     * IBusPanelServiceClass::cursor_down_lookup_table in extended
     * class to receive this signal.
     *
     * <note><para>Argument @user_data is ignored in this function.</para>
     * </note>
     */
    panel_signals[CURSOR_DOWN_LOOKUP_TABLE] =
        g_signal_new (I_("cursor-down-lookup-table"),
            G_TYPE_FROM_CLASS (gobject_class),
            G_SIGNAL_RUN_LAST,
            G_STRUCT_OFFSET (IBusPanelServiceClass, cursor_down_lookup_table),
            NULL, NULL,
            _ibus_marshal_VOID__VOID,
            G_TYPE_NONE, 0);

    /**
     * IBusPanelService::hide-auxiliary-text:
     * @panel: An #IBusPanelService
     *
     * Emitted when the client application get the ::hide-auxiliary-text.
     * Implement the member function
     * IBusPanelServiceClass::hide_auxiliary_text in extended class
     * to receive this signal.
     *
     * <note><para>Argument @user_data is ignored in this function.</para>
     * </note>
     */
    panel_signals[HIDE_AUXILIARY_TEXT] =
        g_signal_new (I_("hide-auxiliary-text"),
            G_TYPE_FROM_CLASS (gobject_class),
            G_SIGNAL_RUN_LAST,
            G_STRUCT_OFFSET (IBusPanelServiceClass, hide_auxiliary_text),
            NULL, NULL,
            _ibus_marshal_VOID__VOID,
            G_TYPE_NONE, 0);

    /**
     * IBusPanelService::hide-language-bar:
     * @panel: An #IBusPanelService
     *
     * Emitted when the client application get the ::hide-language-bar.
     * Implement the member function
     * IBusPanelServiceClass::hide_language_bar in extended class to
     * receive this signal.
     *
     * <note><para>Argument @user_data is ignored in this function.</para>
     * </note>
     */
    panel_signals[HIDE_LANGUAGE_BAR] =
        g_signal_new (I_("hide-language-bar"),
            G_TYPE_FROM_CLASS (gobject_class),
            G_SIGNAL_RUN_LAST,
            G_STRUCT_OFFSET (IBusPanelServiceClass, hide_language_bar),
            NULL, NULL,
            _ibus_marshal_VOID__VOID,
            G_TYPE_NONE, 0);

    /**
     * IBusPanelService::hide-lookup-table:
     * @panel: An #IBusPanelService
     *
     * Emitted when the client application get the ::hide-lookup-table.
     * Implement the member function
     * IBusPanelServiceClass::hide_lookup_table in extended class to
     * receive this signal.
     *
     * <note><para>Argument @user_data is ignored in this function.</para>
     * </note>
     */
    panel_signals[HIDE_LOOKUP_TABLE] =
        g_signal_new (I_("hide-lookup-table"),
            G_TYPE_FROM_CLASS (gobject_class),
            G_SIGNAL_RUN_LAST,
            G_STRUCT_OFFSET (IBusPanelServiceClass, hide_lookup_table),
            NULL, NULL,
            _ibus_marshal_VOID__VOID,
            G_TYPE_NONE, 0);

    /**
     * IBusPanelService::hide-preedit-text:
     * @panel: An #IBusPanelService
     *
     * Emitted when the client application get the ::hide-preedit-text.
     * Implement the member function
     * IBusPanelServiceClass::hide_preedit_text in extended class to
     * receive this signal.
     *
     * <note><para>Argument @user_data is ignored in this function.</para>
     * </note>
     */
    panel_signals[HIDE_PREEDIT_TEXT] =
        g_signal_new (I_("hide-preedit-text"),
            G_TYPE_FROM_CLASS (gobject_class),
            G_SIGNAL_RUN_LAST,
            G_STRUCT_OFFSET (IBusPanelServiceClass, hide_preedit_text),
            NULL, NULL,
            _ibus_marshal_VOID__VOID,
            G_TYPE_NONE, 0);

    /**
     * IBusPanelService::page-up-lookup-table:
     * @panel: An #IBusPanelService
     *
     * Emitted when the client application get the ::page-up-lookup-table.
     * Implement the member function 
     * IBusPanelServiceClass::page_up_lookup_table in extended class
     * to receive this signal.
     *
     * <note><para>Argument @user_data is ignored in this function.</para>
     * </note>
     */
    panel_signals[PAGE_UP_LOOKUP_TABLE] =
        g_signal_new (I_("page-up-lookup-table"),
            G_TYPE_FROM_CLASS (gobject_class),
            G_SIGNAL_RUN_LAST,
            G_STRUCT_OFFSET (IBusPanelServiceClass, page_up_lookup_table),
            NULL, NULL,
            _ibus_marshal_VOID__VOID,
            G_TYPE_NONE, 0);

    /**
     * IBusPanelService::page-down-lookup-table:
     * @panel: An #IBusPanelService
     *
     * Emitted when the client application get the ::page-down-lookup-table.
     * Implement the member function
     * IBusPanelServiceClass::page_down_lookup_table in extended
     * class to receive this signal.
     *
     * <note><para>Argument @user_data is ignored in this function.</para>
     * </note>
     */
    panel_signals[PAGE_DOWN_LOOKUP_TABLE] =
        g_signal_new (I_("page-down-lookup-table"),
            G_TYPE_FROM_CLASS (gobject_class),
            G_SIGNAL_RUN_LAST,
            G_STRUCT_OFFSET (IBusPanelServiceClass, page_down_lookup_table),
            NULL, NULL,
            _ibus_marshal_VOID__VOID,
            G_TYPE_NONE, 0);

    /**
     * IBusPanelService::reset:
     * @panel: An #IBusPanelService
     *
     * Emitted when the client application get the ::reset.
     * Implement the member function
     * IBusPanelServiceClass::reset in extended class to receive this
     * signal.
     *
     * <note><para>Argument @user_data is ignored in this function.</para>
     * </note>
     */
    panel_signals[RESET] =
        g_signal_new (I_("reset"),
            G_TYPE_FROM_CLASS (gobject_class),
            G_SIGNAL_RUN_LAST,
            G_STRUCT_OFFSET (IBusPanelServiceClass, reset),
            NULL, NULL,
            _ibus_marshal_VOID__VOID,
            G_TYPE_NONE, 0);

    /**
     * IBusPanelService::show-auxiliary-text:
     * @panel: An #IBusPanelService
     *
     * Emitted when the client application get the ::show-auxiliary-text.
     * Implement the member function
     * IBusPanelServiceClass::show_auxiliary_text in extended class
     * to receive this signal.
     *
     * <note><para>Argument @user_data is ignored in this function.</para>
     * </note>
     */
    panel_signals[SHOW_AUXILIARY_TEXT] =
        g_signal_new (I_("show-auxiliary-text"),
            G_TYPE_FROM_CLASS (gobject_class),
            G_SIGNAL_RUN_LAST,
            G_STRUCT_OFFSET (IBusPanelServiceClass, show_auxiliary_text),
            NULL, NULL,
            _ibus_marshal_VOID__VOID,
            G_TYPE_NONE, 0);

    /**
     * IBusPanelService::show-language-bar:
     * @panel: An #IBusPanelService
     *
     * Emitted when the client application get the ::show-language-bar.
     * Implement the member function
     * IBusPanelServiceClass::show_language_bar in extended class to
     * receive this signal.
     *
     * <note><para>Argument @user_data is ignored in this function.</para>
     * </note>
     */
    panel_signals[SHOW_LANGUAGE_BAR] =
        g_signal_new (I_("show-language-bar"),
            G_TYPE_FROM_CLASS (gobject_class),
            G_SIGNAL_RUN_LAST,
            G_STRUCT_OFFSET (IBusPanelServiceClass, show_language_bar),
            NULL, NULL,
            _ibus_marshal_VOID__VOID,
            G_TYPE_NONE, 0);

    /**
     * IBusPanelService::show-lookup-table:
     * @panel: An #IBusPanelService
     *
     * Emitted when the client application get the ::show-lookup-table.
     * Implement the member function
     * IBusPanelServiceClass::show_lookup_table in extended class to
     * receive this signal.
     *
     * <note><para>Argument @user_data is ignored in this function.</para>
     * </note>
     */
    panel_signals[SHOW_LOOKUP_TABLE] =
        g_signal_new (I_("show-lookup-table"),
            G_TYPE_FROM_CLASS (gobject_class),
            G_SIGNAL_RUN_LAST,
            G_STRUCT_OFFSET (IBusPanelServiceClass, show_lookup_table),
            NULL, NULL,
            _ibus_marshal_VOID__VOID,
            G_TYPE_NONE, 0);

    /**
     * IBusPanelService::show-preedit-text:
     * @panel: An #IBusPanelService
     *
     * Emitted when the client application get the ::show-preedit-text.
     * Implement the member function
     * IBusPanelServiceClass::show_preedit_text in extended class to
     * receive this signal.
     *
     * <note><para>Argument @user_data is ignored in this function.</para>
     * </note>
     */
    panel_signals[SHOW_PREEDIT_TEXT] =
        g_signal_new (I_("show-preedit-text"),
            G_TYPE_FROM_CLASS (gobject_class),
            G_SIGNAL_RUN_LAST,
            G_STRUCT_OFFSET (IBusPanelServiceClass, show_preedit_text),
            NULL, NULL,
            _ibus_marshal_VOID__VOID,
            G_TYPE_NONE, 0);

    /**
     * IBusPanelService::start-setup:
     * @panel: An #IBusPanelService
     *
     * Emitted when the client application get the ::start-setup.
     * Implement the member function
     * IBusPanelServiceClass::start_setup in extended class to
     * receive this signal.
     *
     * <note><para>Argument @user_data is ignored in this function.</para>
     * </note>
     */
    panel_signals[START_SETUP] =
        g_signal_new (I_("start-setup"),
            G_TYPE_FROM_CLASS (gobject_class),
            G_SIGNAL_RUN_LAST,
            G_STRUCT_OFFSET (IBusPanelServiceClass, start_setup),
            NULL, NULL,
            _ibus_marshal_VOID__VOID,
            G_TYPE_NONE, 0);

    /**
     * IBusPanelService::state-changed:
     * @panel: An #IBusPanelService
     *
     * Emitted when the client application get the ::state-changed.
     * Implement the member function
     * IBusPanelServiceClass::state_changed in extended class to
     * receive this signal.
     *
     * <note><para>Argument @user_data is ignored in this function.</para>
     * </note>
     */
    panel_signals[STATE_CHANGED] =
        g_signal_new (I_("state-changed"),
            G_TYPE_FROM_CLASS (gobject_class),
            G_SIGNAL_RUN_LAST,
            G_STRUCT_OFFSET (IBusPanelServiceClass, state_changed),
            NULL, NULL,
            _ibus_marshal_VOID__VOID,
            G_TYPE_NONE, 0);

    /**
     * IBusPanelService::destroy-context:
     * @panel: An #IBusPanelService
     * @input_context_path: Object path of InputContext.
     *
     * Emitted when the client application destroys.
     * Implement the member function
     * IBusPanelServiceClass::destroy_context in extended class to
     * receive this signal.
     *
     * <note><para>Argument @user_data is ignored in this function.</para>
     * </note>
     */
    panel_signals[DESTROY_CONTEXT] =
        g_signal_new (I_("destroy-context"),
            G_TYPE_FROM_CLASS (gobject_class),
            G_SIGNAL_RUN_LAST,
            G_STRUCT_OFFSET (IBusPanelServiceClass, destroy_context),
            NULL, NULL,
            _ibus_marshal_VOID__STRING,
            G_TYPE_NONE,
            1,
            G_TYPE_STRING);

    /**
     * IBusPanelService::set-content-type:
     * @panel: An #IBusPanelService
     * @purpose: Input purpose.
     * @hints: Input hints.
     *
     * Emitted when the client application get the ::set-content-type.
     * Implement the member function
     * IBusPanelServiceClass::set_content_type in extended class to
     * receive this signal.
     *
     * <note><para>Argument @user_data is ignored in this function.</para>
     * </note>
     */
    panel_signals[SET_CONTENT_TYPE] =
        g_signal_new (I_("set-content-type"),
            G_TYPE_FROM_CLASS (gobject_class),
            G_SIGNAL_RUN_LAST,
            G_STRUCT_OFFSET (IBusPanelServiceClass, set_content_type),
            NULL, NULL,
            _ibus_marshal_VOID__UINT_UINT,
            G_TYPE_NONE,
            2,
            G_TYPE_UINT,
            G_TYPE_UINT);

    /**
     * IBusPanelService::panel-extension-received:
     * @panel: An #IBusPanelService
     * @data: A #GVariant
     *
     * Emitted when the client application get the ::panel-extension-received.
     * Implement the member function
     * IBusPanelServiceClass::panel_extension_received in extended class to
     * receive this signal.
     *
     * <note><para>Argument @user_data is ignored in this function.</para>
     * </note>
     */
    panel_signals[PANEL_EXTENSION_RECEIVED] =
        g_signal_new (I_("panel-extension-received"),
            G_TYPE_FROM_CLASS (gobject_class),
            G_SIGNAL_RUN_LAST,
            G_STRUCT_OFFSET (IBusPanelServiceClass, panel_extension_received),
            NULL, NULL,
            _ibus_marshal_VOID__OBJECT,
            G_TYPE_NONE,
            1,
            IBUS_TYPE_EXTENSION_EVENT);

    /**
     * IBusPanelService::process-key-event:
     * @panel: An #IBusPanelService
     * @keyval: Key symbol of the key press.
     * @keycode: KeyCode of the key press.
     * @state: Key modifier flags.
     *
     * Emitted when a key event is received.
     * Implement the member function IBusPanelServiceClass::process_key_event
     * in extended class to receive this signal.
     * Both the key symbol and keycode are passed to the member function.
     * See ibus_input_context_process_key_event() for further explanation of
     * key symbol, keycode and which to use.
     *
     * Returns: %TRUE for successfully process the key; %FALSE otherwise.
     * See also:  ibus_input_context_process_key_event().
     *
     * <note><para>Argument @user_data is ignored in this function.</para>
     * </note>
     */
    panel_signals[PROCESS_KEY_EVENT] =
        g_signal_new (I_("process-key-event"),
            G_TYPE_FROM_CLASS (gobject_class),
            G_SIGNAL_RUN_LAST,
            G_STRUCT_OFFSET (IBusPanelServiceClass, process_key_event),
            g_signal_accumulator_true_handled, NULL,
            _ibus_marshal_BOOLEAN__UINT_UINT_UINT,
            G_TYPE_BOOLEAN,
            3,
            G_TYPE_UINT,
            G_TYPE_UINT,
            G_TYPE_UINT);

    /**
     * IBusPanelService::commit-text-received:
     * @panel: An #IBusPanelService
     * @text: A #IBusText
     *
     * Emitted when the client application get the ::commit-text-received.
     * Implement the member function
     * IBusPanelServiceClass::commit_text_received in extended class to
     * receive this signal.
     *
     * <note><para>Argument @user_data is ignored in this function.</para>
     * </note>
     */
    panel_signals[COMMIT_TEXT_RECEIVED] =
        g_signal_new (I_("commit-text-received"),
            G_TYPE_FROM_CLASS (gobject_class),
            G_SIGNAL_RUN_LAST,
            G_STRUCT_OFFSET (IBusPanelServiceClass, commit_text_received),
            NULL, NULL,
            _ibus_marshal_VOID__OBJECT,
            G_TYPE_NONE,
            1,
            IBUS_TYPE_TEXT);

    panel_signals[CANDIDATE_CLICKED_LOOKUP_TABLE] =
        g_signal_new (I_("candidate-clicked-lookup-table"),
            G_TYPE_FROM_CLASS (gobject_class),
            G_SIGNAL_RUN_LAST,
            G_STRUCT_OFFSET (IBusPanelServiceClass,
                             candidate_clicked_lookup_table),
            NULL, NULL,
            _ibus_marshal_VOID__UINT_UINT_UINT,
            G_TYPE_NONE,
            3,
            G_TYPE_UINT,
            G_TYPE_UINT,
            G_TYPE_UINT);
}

static void
ibus_panel_service_init (IBusPanelService *panel)
{
}

static void
ibus_panel_service_set_property (IBusPanelService *panel,
                                 guint             prop_id,
                                 const GValue     *value,
                                 GParamSpec       *pspec)
{
    switch (prop_id) {
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (panel, prop_id, pspec);
    }
}

static void
ibus_panel_service_get_property (IBusPanelService *panel,
                                 guint             prop_id,
                                 GValue           *value,
                                 GParamSpec       *pspec)
{
    switch (prop_id) {
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (panel, prop_id, pspec);
    }
}

static void
ibus_panel_service_real_destroy (IBusPanelService *panel)
{
    IBUS_OBJECT_CLASS(ibus_panel_service_parent_class)->destroy (IBUS_OBJECT (panel));
}


static void
_g_object_unref_if_floating (gpointer instance)
{
    if (g_object_is_floating (instance))
        g_object_unref (instance);
}

static gboolean
ibus_panel_service_service_authorized_method (IBusService     *service,
                                              GDBusConnection *connection)
{
    if (ibus_service_get_connection (service) == connection)
        return TRUE;
    return FALSE;
}

static void
ibus_panel_service_service_method_call (IBusService           *service,
                                        GDBusConnection       *connection,
                                        const gchar           *sender,
                                        const gchar           *object_path,
                                        const gchar           *interface_name,
                                        const gchar           *method_name,
                                        GVariant              *parameters,
                                        GDBusMethodInvocation *invocation)
{
    IBusPanelService *panel = IBUS_PANEL_SERVICE (service);

    if (g_strcmp0 (interface_name, IBUS_INTERFACE_PANEL) != 0) {
        IBUS_SERVICE_CLASS (ibus_panel_service_parent_class)->
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

    if (!ibus_panel_service_service_authorized_method (service, connection))
        return;

    if (g_strcmp0 (method_name, "UpdatePreeditText") == 0) {
        GVariant *variant = NULL;
        guint cursor = 0;
        gboolean visible = FALSE;

        g_variant_get (parameters, "(vub)", &variant, &cursor, &visible);
        IBusText *text = IBUS_TEXT (ibus_serializable_deserialize (variant));
        g_variant_unref (variant);

        g_signal_emit (panel, panel_signals[UPDATE_PREEDIT_TEXT], 0, text, cursor, visible);
        _g_object_unref_if_floating (text);
        g_dbus_method_invocation_return_value (invocation, NULL);
        return;
    }

    if (g_strcmp0 (method_name, "UpdateAuxiliaryText") == 0) {
        GVariant *variant = NULL;
        gboolean visible = FALSE;

        g_variant_get (parameters, "(vb)", &variant, &visible);
        IBusText *text = IBUS_TEXT (ibus_serializable_deserialize (variant));
        g_variant_unref (variant);

        g_signal_emit (panel, panel_signals[UPDATE_AUXILIARY_TEXT], 0, text, visible);
        _g_object_unref_if_floating (text);
        g_dbus_method_invocation_return_value (invocation, NULL);
        return;
    }

    if (g_strcmp0 (method_name, "UpdateLookupTable") == 0) {
        GVariant *variant = NULL;
        gboolean visible = FALSE;

        g_variant_get (parameters, "(vb)", &variant, &visible);
        IBusLookupTable *table = IBUS_LOOKUP_TABLE (ibus_serializable_deserialize (variant));
        g_variant_unref (variant);

        g_signal_emit (panel, panel_signals[UPDATE_LOOKUP_TABLE], 0, table, visible);
        _g_object_unref_if_floating (table);
        g_dbus_method_invocation_return_value (invocation, NULL);
        return;
    }

    if (g_strcmp0 (method_name, "FocusIn") == 0) {
        const gchar *path;
        g_variant_get (parameters, "(&o)", &path);
        g_signal_emit (panel, panel_signals[FOCUS_IN], 0, path);
        g_dbus_method_invocation_return_value (invocation, NULL);
        return;
    }

    if (g_strcmp0 (method_name, "FocusOut") == 0) {
        const gchar *path;
        g_variant_get (parameters, "(&o)", &path);
        g_signal_emit (panel, panel_signals[FOCUS_OUT], 0, path);
        g_dbus_method_invocation_return_value (invocation, NULL);
        return;
    }

    if (g_strcmp0 (method_name, "DestroyContext") == 0) {
        const gchar *path;
        g_variant_get (parameters, "(&o)", &path);
        g_signal_emit (panel, panel_signals[DESTROY_CONTEXT], 0, path);
        g_dbus_method_invocation_return_value (invocation, NULL);
        return;
    }

    if (g_strcmp0 (method_name, "RegisterProperties") == 0) {
        GVariant *variant = g_variant_get_child_value (parameters, 0);
        IBusPropList *prop_list = IBUS_PROP_LIST (ibus_serializable_deserialize (variant));
        g_variant_unref (variant);

        g_signal_emit (panel, panel_signals[REGISTER_PROPERTIES], 0, prop_list);
        _g_object_unref_if_floating (prop_list);
        g_dbus_method_invocation_return_value (invocation, NULL);
        return;
    }

    if (g_strcmp0 (method_name, "UpdateProperty") == 0) {
        GVariant *variant = g_variant_get_child_value (parameters, 0);
        IBusProperty *property = IBUS_PROPERTY (ibus_serializable_deserialize (variant));
        g_variant_unref (variant);

        g_signal_emit (panel, panel_signals[UPDATE_PROPERTY], 0, property);
        _g_object_unref_if_floating (property);
        g_dbus_method_invocation_return_value (invocation, NULL);
        return;
    }

    if (g_strcmp0 (method_name, "SetCursorLocation") == 0) {
        gint x, y, w, h;
        g_variant_get (parameters, "(iiii)", &x, &y, &w, &h);
        g_signal_emit (panel, panel_signals[SET_CURSOR_LOCATION], 0, x, y, w, h);
        g_dbus_method_invocation_return_value (invocation, NULL);
        return;
    }

    if (g_strcmp0 (method_name, "SetCursorLocationRelative") == 0) {
        gint x, y, w, h;
        g_variant_get (parameters, "(iiii)", &x, &y, &w, &h);
        g_signal_emit (panel, panel_signals[SET_CURSOR_LOCATION_RELATIVE],
                       0, x, y, w, h);
        g_dbus_method_invocation_return_value (invocation, NULL);
        return;
    }

    if (g_strcmp0 (method_name, "ContentType") == 0) {
        guint purpose, hints;
        g_variant_get (parameters, "(uu)", &purpose, &hints);
        g_signal_emit (panel, panel_signals[SET_CONTENT_TYPE], 0,
                       purpose, hints);
        g_dbus_method_invocation_return_value (invocation, NULL);
        return;
    }

    if (g_strcmp0 (method_name, "PanelExtensionReceived") == 0) {
        GVariant *arg0 = NULL;
        IBusExtensionEvent *event = NULL;
        g_variant_get (parameters, "(v)", &arg0);
        if (arg0) {
            event = IBUS_EXTENSION_EVENT (ibus_serializable_deserialize (arg0));
            g_variant_unref (arg0);
        }
        if (!event) {
            g_dbus_method_invocation_return_error (
                    invocation,
                    G_DBUS_ERROR,
                    G_DBUS_ERROR_FAILED,
                    "PanelExtensionReceived method gives NULL");
            return;
        }
        g_signal_emit (panel, panel_signals[PANEL_EXTENSION_RECEIVED], 0,
                       event);
        _g_object_unref_if_floating (event);
        g_dbus_method_invocation_return_value (invocation, NULL);
        return;
    }
    if (g_strcmp0 (method_name, "ProcessKeyEvent") == 0) {
        guint keyval, keycode, state;
        gboolean retval = FALSE;

        g_variant_get (parameters, "(uuu)", &keyval, &keycode, &state);
        g_signal_emit (panel,
                       panel_signals[PROCESS_KEY_EVENT],
                       0,
                       keyval,
                       keycode,
                       state,
                       &retval);
        g_dbus_method_invocation_return_value (invocation,
                                               g_variant_new ("(b)", retval));
        return;
    }
    if (g_strcmp0 (method_name, "CommitTextReceived") == 0) {
        GVariant *arg0 = NULL;
        IBusText *text = NULL;

        g_variant_get (parameters, "(v)", &arg0);
        if (arg0) {
            text = (IBusText *) ibus_serializable_deserialize (arg0);
            g_variant_unref (arg0);
        }
        if (!text) {
            g_dbus_method_invocation_return_error (
                    invocation,
                    G_DBUS_ERROR,
                    G_DBUS_ERROR_FAILED,
                    "CommitTextReceived method gives NULL");
            return;
        }
        g_signal_emit (panel,
                       panel_signals[COMMIT_TEXT_RECEIVED],
                       0,
                       text);
        _g_object_unref_if_floating (text);
        return;
    }
    if (g_strcmp0 (method_name, "CandidateClickedLookupTable") == 0) {
        guint index = 0;
        guint button = 0;
        guint state = 0;
        g_variant_get (parameters, "(uuu)", &index, &button, &state);
        g_signal_emit (panel,
                       panel_signals[CANDIDATE_CLICKED_LOOKUP_TABLE],
                       0,
                       index, button, state);
        return;
    }


    const static struct {
        const gchar *name;
        const gint signal_id;
    } no_arg_methods [] = {
        { "CursorUpLookupTable",   CURSOR_UP_LOOKUP_TABLE },
        { "CursorDownLookupTable", CURSOR_DOWN_LOOKUP_TABLE },
        { "HideAuxiliaryText",     HIDE_AUXILIARY_TEXT },
        { "HideLanguageBar",       HIDE_LANGUAGE_BAR },
        { "HideLookupTable",       HIDE_LOOKUP_TABLE },
        { "HidePreeditText",       HIDE_PREEDIT_TEXT },
        { "PageUpLookupTable",     PAGE_UP_LOOKUP_TABLE },
        { "PageDownLookupTable",   PAGE_DOWN_LOOKUP_TABLE },
        { "Reset",                 RESET },
        { "ShowAuxiliaryText",     SHOW_AUXILIARY_TEXT },
        { "ShowLanguageBar",       SHOW_LANGUAGE_BAR },
        { "ShowLookupTable",       SHOW_LOOKUP_TABLE },
        { "ShowPreeditText",       SHOW_PREEDIT_TEXT },
        { "StartSetup",            START_SETUP },
        { "StateChanged",          STATE_CHANGED },
    };

    gint i;
    for (i = 0; i < G_N_ELEMENTS (no_arg_methods); i++) {
        if (g_strcmp0 (method_name, no_arg_methods[i].name) == 0) {
            if (no_arg_methods[i].signal_id >= 0) {
                g_signal_emit (panel, panel_signals[no_arg_methods[i].signal_id], 0);
            }
            g_dbus_method_invocation_return_value (invocation, NULL);
            return;
        }
    }

    /* should not be reached */
    g_return_if_reached ();
}

static GVariant *
ibus_panel_service_service_get_property (IBusService        *service,
                                         GDBusConnection    *connection,
                                         const gchar        *sender,
                                         const gchar        *object_path,
                                         const gchar        *interface_name,
                                         const gchar        *property_name,
                                         GError            **error)
{
    return IBUS_SERVICE_CLASS (ibus_panel_service_parent_class)->
                service_get_property (service,
                                      connection,
                                      sender,
                                      object_path,
                                      interface_name,
                                      property_name,
                                      error);
}

static gboolean
ibus_panel_service_service_set_property (IBusService        *service,
                                         GDBusConnection    *connection,
                                         const gchar        *sender,
                                         const gchar        *object_path,
                                         const gchar        *interface_name,
                                         const gchar        *property_name,
                                         GVariant           *value,
                                         GError            **error)
{
    return IBUS_SERVICE_CLASS (ibus_panel_service_parent_class)->
                service_set_property (service,
                                      connection,
                                      sender,
                                      object_path,
                                      interface_name,
                                      property_name,
                                      value,
                                      error);
}


static void
ibus_panel_service_not_implemented (IBusPanelService *panel)
{
    /* g_debug ("not implemented"); */
}

static void
ibus_panel_service_focus_in (IBusPanelService    *panel,
                             const gchar         *input_context_path)
{
    ibus_panel_service_not_implemented(panel);
}

static void
ibus_panel_service_focus_out (IBusPanelService    *panel,
                              const gchar         *input_context_path)
{
    ibus_panel_service_not_implemented(panel);
}

static void
ibus_panel_service_destroy_context (IBusPanelService    *panel,
                                    const gchar         *input_context_path)
{
    ibus_panel_service_not_implemented(panel);
}

static void
ibus_panel_service_register_properties (IBusPanelService *panel,
                                        IBusPropList     *prop_list)
{
    ibus_panel_service_not_implemented(panel);
}

static void
ibus_panel_service_set_cursor_location (IBusPanelService *panel,
                                        gint              x,
                                        gint              y,
                                        gint              w,
                                        gint              h)
{
    ibus_panel_service_not_implemented(panel);
}

static void
ibus_panel_service_set_cursor_location_relative (IBusPanelService *panel,
                                                 gint              x,
                                                 gint              y,
                                                 gint              w,
                                                 gint              h)
{
    ibus_panel_service_not_implemented(panel);
}

static void
ibus_panel_service_update_auxiliary_text (IBusPanelService *panel,
                                          IBusText         *text,
                                          gboolean          visible)
{
    ibus_panel_service_not_implemented(panel);
}

static void
ibus_panel_service_update_lookup_table (IBusPanelService *panel,
                                        IBusLookupTable  *lookup_table,
                                        gboolean          visible)
{
    ibus_panel_service_not_implemented(panel);
}

static void
ibus_panel_service_update_preedit_text (IBusPanelService *panel,
                                        IBusText         *text,
                                        guint             cursor_pos,
                                        gboolean          visible)
{
    ibus_panel_service_not_implemented(panel);
}

static void
ibus_panel_service_update_property (IBusPanelService *panel,
                                    IBusProperty     *prop)
{
    ibus_panel_service_not_implemented(panel);
}

static void
ibus_panel_service_set_content_type (IBusPanelService *panel,
                                     guint             purpose,
                                     guint             hints)
{
    ibus_panel_service_not_implemented(panel);
}

static void
ibus_panel_service_panel_extension_received (IBusPanelService   *panel,
                                             IBusExtensionEvent *event)
{
    ibus_panel_service_not_implemented(panel);
}

IBusPanelService *
ibus_panel_service_new (GDBusConnection *connection)
{
    g_return_val_if_fail (G_IS_DBUS_CONNECTION (connection), NULL);

    GObject *object = g_object_new (IBUS_TYPE_PANEL_SERVICE,
                                    "object-path", IBUS_PATH_PANEL,
                                    "connection", connection,
                                    NULL);

    return IBUS_PANEL_SERVICE (object);
}

void
ibus_panel_service_candidate_clicked (IBusPanelService *panel,
                                      guint             index,
                                      guint             button,
                                      guint             state)
{
    g_return_if_fail (IBUS_IS_PANEL_SERVICE (panel));
    ibus_service_emit_signal ((IBusService *) panel,
                              NULL,
                              IBUS_INTERFACE_PANEL,
                              "CandidateClicked",
                              g_variant_new ("(uuu)", index, button, state),
                              NULL);
}

void
ibus_panel_service_property_activate (IBusPanelService *panel,
                                      const gchar      *prop_name,
                                      guint             prop_state)
{
    g_return_if_fail (IBUS_IS_PANEL_SERVICE (panel));
    ibus_service_emit_signal ((IBusService *) panel,
                              NULL,
                              IBUS_INTERFACE_PANEL,
                              "PropertyActivate",
                              g_variant_new ("(su)", prop_name, prop_state),
                              NULL);
}

void
ibus_panel_service_property_show (IBusPanelService *panel,
                                  const gchar      *prop_name)
{
    g_return_if_fail (IBUS_IS_PANEL_SERVICE (panel));
    ibus_service_emit_signal ((IBusService *) panel,
                              NULL,
                              IBUS_INTERFACE_PANEL,
                              "PropertyShow",
                              g_variant_new ("(s)", prop_name),
                              NULL);
}

void
ibus_panel_service_property_hide (IBusPanelService *panel,
                                  const gchar      *prop_name)
{
    g_return_if_fail (IBUS_IS_PANEL_SERVICE (panel));
    ibus_service_emit_signal ((IBusService *) panel,
                              NULL,
                              IBUS_INTERFACE_PANEL,
                              "PropertyHide",
                              g_variant_new ("(s)", prop_name),
                              NULL);
}

void
ibus_panel_service_commit_text (IBusPanelService *panel,
                                IBusText         *text)
{
    GVariant *variant;
    g_return_if_fail (IBUS_IS_PANEL_SERVICE (panel));
    g_return_if_fail (IBUS_IS_TEXT (text));

    variant = ibus_serializable_serialize ((IBusSerializable *)text);
    ibus_service_emit_signal ((IBusService *) panel,
                              NULL,
                              IBUS_INTERFACE_PANEL,
                              "CommitText",
                              g_variant_new ("(v)", variant),
                              NULL);

    if (g_object_is_floating (text)) {
        g_object_unref (text);
    }
}

void
ibus_panel_service_panel_extension (IBusPanelService   *panel,
                                    IBusExtensionEvent *event)
{
    GVariant *variant;
    g_return_if_fail (IBUS_IS_PANEL_SERVICE (panel));
    g_return_if_fail (IBUS_IS_EXTENSION_EVENT (event));

    variant = ibus_serializable_serialize ((IBusSerializable *)event);
    ibus_service_emit_signal ((IBusService *) panel,
                              NULL,
                              IBUS_INTERFACE_PANEL,
                              "PanelExtension",
                              g_variant_new ("(v)", variant),
                              NULL);

    if (g_object_is_floating (event)) {
        g_object_unref (event);
    }
}

void
ibus_panel_service_panel_extension_register_keys (IBusPanelService   *panel,
                                                  const gchar
                                                           *first_property_name,
                                                  ...)
{
    GVariantBuilder builder;
    GVariantBuilder child;
    const gchar *name;
    va_list var_args;
    IBusProcessKeyEventData *keys;

    g_return_if_fail (first_property_name);

    g_variant_builder_init (&builder, G_VARIANT_TYPE ("a{sv}"));
    name = first_property_name;

    va_start (var_args, first_property_name);
    do {
        keys = va_arg (var_args, IBusProcessKeyEventData *);
        if (keys == NULL) {
            va_end (var_args);
            g_warning ("Failed to va_arg for IBusProcessKeyEventData");
            return;
        }
        g_variant_builder_init (&child, G_VARIANT_TYPE ("av"));
        for (; keys; keys++) {
            if (keys->keyval == 0 && keys->keycode == 0 && keys->state == 0)
                break;
            g_variant_builder_add (&child, "v",
                                   g_variant_new ("(iii)",
                                                  keys->keyval,
                                                  keys->keycode, 
                                                  keys->state));
        }
        g_variant_builder_add (&builder, "{sv}",
                               g_strdup (name), g_variant_builder_end (&child));
    } while ((name = va_arg (var_args, const gchar *)));
    va_end (var_args);

    ibus_service_emit_signal ((IBusService *) panel,
                              NULL,
                              IBUS_INTERFACE_PANEL,
                              "PanelExtensionRegisterKeys",
                              g_variant_new ("(v)",
                                             g_variant_builder_end (&builder)),
                              NULL);
}

void
ibus_panel_service_update_preedit_text_received (IBusPanelService *panel,
                                                 IBusText         *text,
                                                 guint             cursor_pos,
                                                 gboolean          visible)
{
    GVariant *variant;

    g_return_if_fail (IBUS_IS_PANEL_SERVICE (panel));
    g_return_if_fail (IBUS_IS_TEXT (text));

    variant = ibus_serializable_serialize ((IBusSerializable *)text);
    g_return_if_fail (variant);
    ibus_service_emit_signal ((IBusService *) panel,
                              NULL,
                              IBUS_INTERFACE_PANEL,
                              "UpdatePreeditTextReceived",
                              g_variant_new ("(vub)",
                                             variant, cursor_pos, visible),
                              NULL);

    if (g_object_is_floating (text)) {
        g_object_unref (text);
    }
}

void
ibus_panel_service_update_auxiliary_text_received (IBusPanelService *panel,
                                                   IBusText         *text,
                                                   gboolean          visible)
{
    GVariant *variant;
    g_return_if_fail (IBUS_IS_PANEL_SERVICE (panel));
    g_return_if_fail (IBUS_IS_TEXT (text));

    variant = ibus_serializable_serialize ((IBusSerializable *)text);
    g_return_if_fail (variant);
    ibus_service_emit_signal ((IBusService *) panel,
                              NULL,
                              IBUS_INTERFACE_PANEL,
                              "UpdateAuxiliaryTextReceived",
                              g_variant_new ("(vb)",
                                             variant, visible),
                              NULL);

    if (g_object_is_floating (text)) {
        g_object_unref (text);
    }
}

void
ibus_panel_service_update_lookup_table_received (IBusPanelService *panel,
                                                 IBusLookupTable  *table,
                                                 gboolean          visible)
{
    GVariant *variant;

    g_return_if_fail (IBUS_IS_PANEL_SERVICE (panel));
    g_return_if_fail (IBUS_IS_LOOKUP_TABLE (table));

    variant = ibus_serializable_serialize ((IBusSerializable *)table);
    g_return_if_fail (variant);
    ibus_service_emit_signal ((IBusService *) panel,
                              NULL,
                              IBUS_INTERFACE_PANEL,
                              "UpdateLookupTableReceived",
                              g_variant_new ("(vb)",
                                             variant, visible),
                              NULL);

    if (g_object_is_floating (table)) {
        g_object_unref (table);
    }
}

void
ibus_panel_service_forward_process_key_event (IBusPanelService *panel,
                                              guint32           keyval,
                                              guint32           keycode,
                                              guint32           state)
{
    g_return_if_fail (IBUS_IS_PANEL_SERVICE (panel));
    ibus_service_emit_signal ((IBusService *) panel,
                              NULL,
                              IBUS_INTERFACE_PANEL,
                              "ForwardProcessKeyEvent",
                              g_variant_new ("(uuu)",
                                              keyval, keycode, state),
                              NULL);
}

#define DEFINE_FUNC(name, Name)                             \
    void                                                    \
    ibus_panel_service_##name (IBusPanelService *panel)     \
    {                                                       \
        g_return_if_fail (IBUS_IS_PANEL_SERVICE (panel));   \
        ibus_service_emit_signal ((IBusService *) panel,    \
                                  NULL,                     \
                                  IBUS_INTERFACE_PANEL,     \
                                  #Name,                    \
                                  NULL,                     \
                                  NULL);                    \
    }
DEFINE_FUNC (cursor_down, CursorDown)
DEFINE_FUNC (cursor_up, CursorUp)
DEFINE_FUNC (page_down, PageDown)
DEFINE_FUNC (page_up, PageUp)
#undef DEFINE_FUNC
