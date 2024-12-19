/* -*- mode: C; c-basic-offset: 4; indent-tabs-mode: nil; -*- */
/* vim:set et sts=4: */
/* ibus - The Input Bus
 * Copyright (c) 2009-2014 Google Inc. All rights reserved.
 * Copyright (c) 2017-2024 Takao Fujiwara <takao.fujiwara1@gmail.com>
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

#if !defined (__IBUS_H_INSIDE__) && !defined (IBUS_COMPILATION)
#error "Only <ibus.h> can be included directly"
#endif

#ifndef __IBUS_PANEL_SERVICE_H_
#define __IBUS_PANEL_SERVICE_H_

/**
 * SECTION: ibuspanelservice
 * @short_description: Panel service back-end.
 * @stability: Stable
 *
 * An IBusPanelService is a base class for UI services.
 * Developers can "extend" this class for panel UI development.
 */
#include "ibuslookuptable.h"
#include "ibusservice.h"
#include "ibusproplist.h"
#include "ibusxevent.h"

/*
 * Type macros.
 */

/* define GOBJECT macros */
#define IBUS_TYPE_PANEL_SERVICE                \
    (ibus_panel_service_get_type ())
#define IBUS_PANEL_SERVICE(obj)                        \
    (G_TYPE_CHECK_INSTANCE_CAST ((obj), IBUS_TYPE_PANEL_SERVICE, IBusPanelService))
#define IBUS_PANEL_SERVICE_CLASS(klass)        \
    (G_TYPE_CHECK_CLASS_CAST ((klass), IBUS_TYPE_PANEL_SERVICE, IBusPanelServiceClass))
#define IBUS_IS_PANEL_SERVICE(obj)                     \
    (G_TYPE_CHECK_INSTANCE_TYPE ((obj), IBUS_TYPE_PANEL_SERVICE))
#define IBUS_IS_PANEL_SERVICE_CLASS(klass)             \
    (G_TYPE_CHECK_CLASS_TYPE ((klass), IBUS_TYPE_PANEL_SERVICE))
#define IBUS_PANEL_SERVICE_GET_CLASS(obj)      \
    (G_TYPE_INSTANCE_GET_CLASS ((obj), IBUS_TYPE_PANEL_SERVICE, IBusPanelServiceClass))

G_BEGIN_DECLS

typedef struct _IBusPanelService IBusPanelService;
typedef struct _IBusPanelServiceClass IBusPanelServiceClass;

/**
 * IBusPanelService:
 *
 * An opaque data type representing an IBusPanelService.
 */
struct _IBusPanelService {
    IBusService parent;
    /* instance members */
};

struct _IBusPanelServiceClass {
    IBusServiceClass parent;

    /* class members */
    void     (* focus_in)                  (IBusPanelService       *panel,
                                            const gchar
                                                   *input_context_path);
    void     (* focus_out)                 (IBusPanelService       *panel,
                                            const gchar
                                                   *input_context_path);
    void     (* register_properties)       (IBusPanelService       *panel,
                                            IBusPropList           *prop_list);
    void     (* set_cursor_location)       (IBusPanelService       *panel,
                                            gint                    x,
                                            gint                    y,
                                            gint                    w,
                                            gint                    h);
    void     (* update_auxiliary_text)     (IBusPanelService       *panel,
                                            IBusText               *text,
                                            gboolean                visible);
    void     (* update_lookup_table)       (IBusPanelService       *panel,
                                            IBusLookupTable        *lookup_table,
                                            gboolean                visible);
    void     (* update_preedit_text)       (IBusPanelService       *panel,
                                            IBusText               *text,
                                            guint                  cursor_pos,
                                            gboolean               visible);
    void     (* update_property)           (IBusPanelService       *panel,
                                            IBusProperty           *prop);
    void     (* cursor_down_lookup_table)  (IBusPanelService       *panel);
    void     (* cursor_up_lookup_table)    (IBusPanelService       *panel);
    void     (* hide_auxiliary_text)       (IBusPanelService       *panel);
    void     (* hide_language_bar)         (IBusPanelService       *panel);
    void     (* hide_lookup_table)         (IBusPanelService       *panel);
    void     (* hide_preedit_text)         (IBusPanelService       *panel);
    void     (* page_down_lookup_table)    (IBusPanelService       *panel);
    void     (* page_up_lookup_table)      (IBusPanelService       *panel);
    void     (* reset)                     (IBusPanelService       *panel);
    void     (* show_auxiliary_text)       (IBusPanelService       *panel);
    void     (* show_language_bar)         (IBusPanelService       *panel);
    void     (* show_lookup_table)         (IBusPanelService       *panel);
    void     (* show_preedit_text)         (IBusPanelService       *panel);
    void     (* start_setup)               (IBusPanelService       *panel);
    void     (* state_changed)             (IBusPanelService       *panel);
    void     (* destroy_context)           (IBusPanelService       *panel,
                                            const gchar
                                                   *input_context_path);
    void     (* set_content_type)          (IBusPanelService       *panel,
                                            guint                   purpose,
                                            guint                   hints);
    void     (* set_cursor_location_relative)
                                           (IBusPanelService       *panel,
                                            gint                    x,
                                            gint                    y,
                                            gint                    w,
                                            gint                    h);
    void     (* panel_extension_received)
                                           (IBusPanelService       *panel,
                                            IBusExtensionEvent     *event);
    gboolean (* process_key_event)
                                           (IBusPanelService       *panel,
                                            guint                   keyval,
                                            guint                   keycode,
                                            guint                   state);
    void     (* commit_text_received)
                                           (IBusPanelService       *panel,
                                            IBusText               *text);
    void     (* candidate_clicked_lookup_table)
                                           (IBusPanelService       *panel,
                                            guint                   index,
                                            guint                   button,
                                            guint                   state);

    /*< private >*/
    /* padding */
    gpointer pdummy[2];  // We can add 8 pointers without breaking the ABI.
};

GType            ibus_panel_service_get_type  (void);

/**
 * ibus_panel_service_new:
 * @connection: An GDBusConnection.
 *
 * Creates a new #IBusPanelService from an #GDBusConnection.
 *
 * Returns: A newly allocated #IBusPanelService.
 */
IBusPanelService *ibus_panel_service_new (GDBusConnection    *connection);

/**
 * ibus_panel_service_candidate_clicked:
 * @panel: An IBusPanelService
 * @index: Index in the Lookup table
 * @button: GdkEventButton::button (1: left button, etc.)
 * @state: GdkEventButton::state (key modifier flags)
 *
 * Notify that a candidate is clicked
 * by sending a "CandidateClicked" to IBus service.
 */
void ibus_panel_service_candidate_clicked (IBusPanelService *panel,
                                           guint             index,
                                           guint             button,
                                           guint             state);

/**
 * ibus_panel_service_cursor_down:
 * @panel: An IBusPanelService
 *
 * Notify that the cursor is down
 * by sending a "CursorDown" to IBus service.
 */
void ibus_panel_service_cursor_down       (IBusPanelService *panel);

/**
 * ibus_panel_service_cursor_up:
 * @panel: An IBusPanelService
 *
 * Notify that the cursor is up
 * by sending a "CursorUp" to IBus service.
 */
void ibus_panel_service_cursor_up         (IBusPanelService *panel);

/**
 * ibus_panel_service_page_down:
 * @panel: An IBusPanelService
 *
 * Notify that the page is down
 * by sending a "PageDown" to IBus service.
 */
void ibus_panel_service_page_down         (IBusPanelService *panel);

/**
 * ibus_panel_service_page_up:
 * @panel: An IBusPanelService
 *
 * Notify that the page is up
 * by sending a "PageUp" to IBus service.
 */
void ibus_panel_service_page_up           (IBusPanelService *panel);

/**
 * ibus_panel_service_property_activate:
 * @panel: An IBusPanelService
 * @prop_name: A property name
 * @prop_state: State of the property
 *
 * Notify that a property is active
 * by sending a "PropertyActivate" message to IBus service.
 */
void ibus_panel_service_property_activate (IBusPanelService *panel,
                                           const gchar      *prop_name,
                                           guint             prop_state);
/**
 * ibus_panel_service_property_show:
 * @panel: An IBusPanelService
 * @prop_name: A property name
 *
 * Notify that a property is shown
 * by sending a "ValueChanged" message to IBus service.
 */
void ibus_panel_service_property_show     (IBusPanelService *panel,
                                           const gchar      *prop_name);

/**
 * ibus_panel_service_property_hide:
 * @panel: An IBusPanelService
 * @prop_name: A property name
 *
 * Notify that a property is hidden
 * by sending a "ValueChanged" message to IBus service.
 */
void ibus_panel_service_property_hide     (IBusPanelService *panel,
                                           const gchar      *prop_name);

/**
 * ibus_panel_service_commit_text:
 * @panel: An #IBusPanelService
 * @text: An #IBusText
 *
 * Notify that a text is sent
 * by sending a "CommitText" message to IBus service.
 */
void ibus_panel_service_commit_text       (IBusPanelService *panel,
                                           IBusText         *text);

/**
 * ibus_panel_service_panel_extension:
 * @panel: An #IBusPanelService
 * @event: (transfer full): A #PanelExtensionEvent which is sent to a
 *                          panel extension. 
 *
 * Enable or disable a panel extension with #IBusExtensionEvent.
 * Notify that a data is sent
 * by sending a "PanelExtension" message to IBus panel extension service.
 */
void ibus_panel_service_panel_extension   (IBusPanelService   *panel,
                                           IBusExtensionEvent *event);

/**
 * ibus_panel_service_panel_extension_register_keys:
 * @panel: An #IBusPanelService
 * @first_property_name: the first name of the shortcut keys. This is %NULL
 " terminated.
 *
 * Register shortcut keys to enable panel extensions with #IBusExtensionEvent.
 * Notify that a data is sent
 * by sending a "PanelExtensionRegisterKeys" message to IBus panel extension
 * service. Seems Vala does not support uint[][3] and use
 * IBusProcessKeyEventData[]. E.g.
 * IBusProcessKeyEventData[] keys = {{
 *         IBUS_KEY_e, 0, IBUS_SHIFT_MASK | IBUS_SUPER_MASK }};
 * ibus_panel_service_panel_extension_register_keys(panel, "emoji", keys, NULL);
 */
void ibus_panel_service_panel_extension_register_keys
                                           (IBusPanelService  *panel,
                                            const gchar       *first_property_name,
                                            ...);

/**
 * ibus_panel_service_update_preedit_text_received:
 * @panel: An #IBusPanelService
 * @text: Update content.
 * @cursor_pos: Current position of cursor
 * @visible: Whether the pre-edit buffer is visible.
 *
 * Notify that the preedit is updated by the panel extension
 *
 * (Note: The table object will be released, if it is floating.
 *  If caller want to keep the object, caller should make the object
 *  sink by g_object_ref_sink.)
 */
void ibus_panel_service_update_preedit_text_received
                                          (IBusPanelService *panel,
                                           IBusText         *text,
                                           guint             cursor_pos,
                                           gboolean          visible);

/**
 * ibus_panel_service_show_preedit_text_received:
 * @panel: An IBusPanelService
 *
 * Notify that the preedit is shown by the panel extension
 */
void ibus_panel_service_show_preedit_text_received
                                          (IBusPanelService *panel);

/**
 * ibus_panel_service_hide_preedit_text_received:
 * @panel: An IBusPanelService
 *
 * Notify that the preedit is hidden by the panel extension
 */
void ibus_panel_service_hide_preedit_text_received
                                          (IBusPanelService *panel);

/**
 * ibus_panel_service_update_auxiliary_text_received:
 * @panel: An #IBusPanelService
 * @text: An #IBusText
 * @visible: Whether the auxilirary text is visible.
 *
 * Notify that the auxilirary is updated by the panel extension.
 *
 * (Note: The table object will be released, if it is floating.
 *  If caller want to keep the object, caller should make the object
 *  sink by g_object_ref_sink.)
 */
void ibus_panel_service_update_auxiliary_text_received
                                          (IBusPanelService *panel,
                                           IBusText         *text,
                                           gboolean          visible);

/**
 * ibus_panel_service_update_lookup_table_received:
 * @panel: An #IBusPanelService
 * @table: An #IBusLookupTable
 * @visible: Whether the lookup table is visible.
 *
 * Notify that the lookup table is updated by the panel extension.
 *
 * (Note: The table object will be released, if it is floating.
 *  If caller want to keep the object, caller should make the object
 *  sink by g_object_ref_sink.)
 */
void ibus_panel_service_update_lookup_table_received
                                          (IBusPanelService *panel,
                                           IBusLookupTable  *table,
                                           gboolean          visible);

/**
 * ibus_panel_service_forward_process_key_event:
 * @panel: An #IBusPanelService.
 * @keyval: Key symbol of a key event.
 * @keycode: Keycode of a key event.
 * @state: Key modifier flags.
 *
 * Forward key events when an IBus popup takes the focus and the events
 * needs to be forwared to the target IBus engine.
 */
void ibus_panel_service_forward_process_key_event
                                          (IBusPanelService *panel,
                                           guint32           keyval,
                                           guint32           keycode,
                                           guint32           state);
G_END_DECLS
#endif
