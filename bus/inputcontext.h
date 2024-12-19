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
#ifndef __BUS_INPUT_CONTEXT_H_
#define __BUS_INPUT_CONTEXT_H_

#include <ibus.h>

#include "connection.h"
#include "factoryproxy.h"

#ifndef __BUS_PANEL_PROXY_DEFINED
#define __BUS_PANEL_PROXY_DEFINED
typedef struct _BusPanelProxy BusPanelProxy;
#endif

/*
 * Type macros.
 */

/* define GOBJECT macros */
#define BUS_TYPE_INPUT_CONTEXT                                  \
    (bus_input_context_get_type ())
#define BUS_INPUT_CONTEXT(obj)                                  \
    (G_TYPE_CHECK_INSTANCE_CAST ((obj),                         \
                                 BUS_TYPE_INPUT_CONTEXT,        \
                                 BusInputContext))
#define BUS_INPUT_CONTEXT_CLASS(klass)                          \
    (G_TYPE_CHECK_CLASS_CAST ((klass),                          \
                              BUS_TYPE_INPUT_CONTEXT,           \
                              BusInputContextClass))
#define BUS_IS_INPUT_CONTEXT(obj)                               \
    (G_TYPE_CHECK_INSTANCE_TYPE ((obj), BUS_TYPE_INPUT_CONTEXT))
#define BUS_IS_INPUT_CONTEXT_CLASS(klass)                       \
    (G_TYPE_CHECK_CLASS_TYPE ((klass), BUS_TYPE_INPUT_CONTEXT))
#define BUS_INPUT_CONTEXT_GET_CLASS(obj)                        \
    (G_TYPE_INSTANCE_GET_CLASS ((obj),                          \
                                BUS_TYPE_INPUT_CONTEXT,         \
                                BusInputContextClass))

G_BEGIN_DECLS

typedef struct _BusInputContext BusInputContext;
typedef struct _BusInputContextClass BusInputContextClass;

GType                bus_input_context_get_type (void);
BusInputContext     *bus_input_context_new      (BusConnection      *connection,
                                                 const gchar        *client);

/**
 * bus_input_context_focus_in:
 * @context: A #BusInputContext.
 *
 * Give a focus to the context. Call FocusIn, Enable, SetCapabilities,
 * and SetCursorLocation methods of the engine for the context,
 * and then emit glib signals to the context object. This function does
 * nothing if the context already has a focus.
 */
void                 bus_input_context_focus_in (BusInputContext    *context);

/**
 * bus_input_context_focus_out:
 * @context: A #BusInputContext.
 *
 * Remove a focus from the context. Call FocusOut method of the engine for
 * the context.
 * This function does nothing if the context does not have a focus.
 */
void                 bus_input_context_focus_out
                                                (BusInputContext    *context);

/**
 * bus_input_context_has_focus:
 * @context: A #BusInputContext.
 * @returns: context->has_focus.
 */
gboolean             bus_input_context_has_focus
                                                (BusInputContext    *context);

/**
 * bus_input_context_enable:
 * @context: A #BusInputContext.
 *
 * Enable the current engine for the context. Request an engine (if needed),
 * call FocusIn, Enable, SetCapabilities, and SetCursorLocation methods
 * of the engine for the context, and then emit glib and D-Bus "enabled"
 * signals.
 */
void                 bus_input_context_enable   (BusInputContext    *context);

/**
 * bus_input_context_disable:
 * @context: A #BusInputContext.
 *
 * Disable the current engine for the context. Request an engine (if needed),
 * call FocusIn, Enable, SetCapabilities, and SetCursorLocation methods
 * of the engine for the context, and then emit glib and D-Bus "enabled"
 * signals.
 */
void                 bus_input_context_disable  (BusInputContext    *context);

/**
 * bus_input_context_page_up:
 * @context: A #BusInputContext.
 *
 * Call page_up method of the current engine proxy.
 */
void                 bus_input_context_page_up  (BusInputContext    *context);

/**
 * bus_input_context_page_down:
 * @context: A #BusInputContext.
 *
 * Call page_down method of the current engine proxy.
 */
void                 bus_input_context_page_down
                                                (BusInputContext    *context);

/**
 * bus_input_context_cursor_up:
 * @context: A #BusInputContext.
 *
 * Call cursor_up method of the current engine proxy.
 */
void                 bus_input_context_cursor_up
                                                (BusInputContext    *context);

/**
 * bus_input_context_cursor_down:
 * @context: A #BusInputContext.
 *
 * Call cursor_down method of the current engine proxy.
 */
void                 bus_input_context_cursor_down
                                                (BusInputContext    *context);

/**
 * bus_input_context_candidate_clicked:
 * @context: A #BusInputContext.
 * @index: An index.
 * @button: A button number.
 * @state: A button state.
 *
 * Call candidate_clicked method of the current engine proxy.
 */
void                 bus_input_context_candidate_clicked
                                                (BusInputContext    *context,
                                                 guint               index,
                                                 guint               button,
                                                 guint               state);

/**
 * bus_input_context_set_engine:
 * @context: A #BusInputContext.
 * @engine: A #BusEngineProxy.
 *
 * Use the engine on the context.
 */
void                 bus_input_context_set_engine
                                                (BusInputContext    *context,
                                                 BusEngineProxy     *engine);

/**
 * bus_input_context_set_engine_by_desc:
 * @context: A #BusInputContext.
 * @desc: the engine to use on the context.
 * @timeout: timeout (in ms) for D-Bus calls.
 * @callback: a function to be called when bus_input_context_set_engine_by_desc
 *            is finished. if NULL, the default callback
 *            function, which just calls
 *            bus_input_context_set_engine_by_desc_finish, is used.
 * @user_data: an argument of @callback.
 *
 * Create a new BusEngineProxy object and use it on the context.
 */
void                 bus_input_context_set_engine_by_desc
                                                (BusInputContext    *context,
                                                 IBusEngineDesc     *desc,
                                                 gint                timeout,
                                                 GCancellable
                                                                   *cancellable,
                                                 GAsyncReadyCallback callback,
                                                 gpointer            user_data);

/**
 * bus_input_context_set_engine_by_desc_finish:
 * @context: A #BusInputContext.
 * @res: A #GAsyncResult.
 * @error: A #GError.
 *
 * A function to be called by the GAsyncReadyCallback function for
 * bus_input_context_set_engine_by_desc.
 */
gboolean             bus_input_context_set_engine_by_desc_finish
                                                (BusInputContext    *context,
                                                 GAsyncResult       *res,
                                                 GError            **error);

/**
 * bus_input_context_get_engine:
 * @context: A #BusInputContext.
 *
 * Get a BusEngineProxy object of the current engine.
 */
BusEngineProxy      *bus_input_context_get_engine
                                                (BusInputContext    *context);

/**
 * bus_input_context_get_engine_desc:
 * @context: A #BusInputContext.
 *
 * Get an IBusEngineDesc object of the current engine.
 */
IBusEngineDesc      *bus_input_context_get_engine_desc
                                                (BusInputContext    *context);

/**
 * bus_input_context_property_activate:
 * @context: A #BusInputContext.
 * @prop_name: A property name.
 * @prop_state: A property state.
 *
 * Call property_activate method of the current engine proxy.
 */
void                 bus_input_context_property_activate
                                                (BusInputContext    *context,
                                                 const gchar        *prop_name,
                                                 gint
                                                                    prop_state);

/**
 * bus_input_context_get_capabilities:
 * @context: A #BusInputContext.
 * @returns: context->capabilities.
 */
guint                bus_input_context_get_capabilities
                                                (BusInputContext    *context);

/**
 * bus_input_context_set_capabilities:
 * @context: A #BusInputContext.
 * @capabilities: capabilities.
 *
 * Call set_capabilities method of the current engine proxy.
 */
void                 bus_input_context_set_capabilities
                                                (BusInputContext    *context,
                                                 guint
                                                                  capabilities);

/**
 * bus_input_context_get_client:
 * @context: A #BusInputContext.
 * @returns: context->client.
 */
const gchar         *bus_input_context_get_client
                                                (BusInputContext    *context);

/**
 * bus_input_context_get_content_type:
 * @context: A #BusInputContext.
 * @purpose: Input purpose.
 * @hints: Input hints.
 */
void                 bus_input_context_get_content_type
                                                (BusInputContext    *context,
                                                 guint              *purpose,
                                                 guint              *hints);

/**
 * bus_input_context_set_content_type:
 * @context: A #BusInputContext.
 * @purpose: Input purpose.
 * @hints: Input hints.
 */
void                 bus_input_context_set_content_type
                                                (BusInputContext *context,
                                                 guint            purpose,
                                                 guint            hints);

/**
 * bus_input_context_commit_text:
 * @context: A #BusInputContext.
 * @text: A committed text.
 */
void                 bus_input_context_commit_text
                                                (BusInputContext *context,
                                                 IBusText        *text);

/**
 * bus_input_context_commit_text:
 * @context: A #BusInputContext.
 * @text: A committed text.
 * @use_extension: Use an extension if it's %TRUE and the extension is
 *                 available.
 */
void                 bus_input_context_commit_text_use_extension
                                               (BusInputContext *context,
                                                IBusText        *text,
                                                gboolean         use_extension);

/**
 * bus_input_context_set_emoji_extension:
 * @context: A #BusInputContext.
 * @extension: A #BusPanelProxy.
 */
void                 bus_input_context_set_emoji_extension
                                                (BusInputContext *context,
                                                 BusPanelProxy   *extension);

/**
 * bus_input_context_update_preedit_text:
 * @context: A #BusInputContext.
 * @text: An #IBusText.
 * @cursor_pos: The cursor position.
 * @visible: %TRUE if the preedit is visible. Otherwise %FALSE.
 * @mode: The preedit commit mode.
 * @use_extension: %TRUE if preedit text is sent to the extesion at first.
 *
 * Update a preedit text. Send D-Bus signal to update status of client or
 * send glib signal to the panel, depending on capabilities of the client.
 */
void                 bus_input_context_update_preedit_text
                                                (BusInputContext    *context,
                                                 IBusText           *text,
                                                 guint               cursor_pos,
                                                 gboolean            visible,
                                                 guint               mode,
                                                 gboolean
                                                                 use_extension);

/**
 * bus_input_context_update_lookup_table:
 * @context: A #BusInputContext.
 * @table: An #IBusTable.
 * @visible: %TRUE if the lookup table is visible. Otherwise %FALSE.
 * @is_extension: %TRUE if the lookup table is created by panel extensions.
 *
 * Update contents in the lookup table.
 * Send D-Bus signal to update status of client or send glib signal to the
 * panel, depending on capabilities of the client.
 */
void                 bus_input_context_update_lookup_table
                                                (BusInputContext    *context,
                                                 IBusLookupTable    *table,
                                                 gboolean            visible,
                                                 gboolean
                                                                  is_extension);


/**
 * bus_input_context_panel_extension_received:
 * @context: A #BusInputContext.
 * @event: An #IBusExtensionEvent.
 *
 * Send An #IBusExtensionEvent callback from an extension.
 */
void                 bus_input_context_panel_extension_received
                                                (BusInputContext    *context,
                                                 IBusExtensionEvent *event);

/**
 * bus_input_context_clear_preedit_text:
 *
 * Clear context->preedit_text. If the preedit mode is
 * IBUS_ENGINE_PREEDIT_COMMIT and with_signal is %TRUE, commit it before
 * clearing.
 * If with_signal is %FALSE, this just clears the preedit coditions
 * and the actual preedit is handled in ibus clients.
 */
void                 bus_input_context_clear_preedit_text
                                                (BusInputContext    *context,
                                                 gboolean
                                                                   with_signal);

/**
 * bus_input_context_is_extension_lookup_table:
 *
 * Return if the lookup table is extension.
 */
gboolean             bus_input_context_is_extension_lookup_table
                                                (BusInputContext    *context);

/**
 * bus_input_context_forward_process_key_event:
 *
 * Forward key events to the focused engine.
 */
void bus_input_context_forward_process_key_event (BusInputContext *context,
                                                  guint            keyval,
                                                  guint            keycode,
                                                  guint            modifiers);
G_END_DECLS
#endif
