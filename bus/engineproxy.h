/* -*- mode: C; c-basic-offset: 4; indent-tabs-mode: nil; -*- */
/* vim:set et sts=4: */
/* ibus - The Input Bus
 * Copyright (C) 2008-2013 Peng Huang <shawn.p.huang@gmail.com>
 * Copyright (C) 2018-2022 Takao Fujiwara <takao.fujiwara@gmail.com>
 * Copyright (C) 2008-2018 Red Hat, Inc.
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
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
 * License along with this library; if not, see <https://www.gnu.org/licenses/>.
 */
#ifndef __BUS_ENGINE_PROXY_H_
#define __BUS_ENGINE_PROXY_H_

#include <gio/gio.h>
#include <ibus.h>
#include "connection.h"

/*
 * Type macros.
 */

/* define GOBJECT macros */
#define BUS_TYPE_ENGINE_PROXY             \
    (bus_engine_proxy_get_type ())
#define BUS_ENGINE_PROXY(obj)             \
    (G_TYPE_CHECK_INSTANCE_CAST ((obj), BUS_TYPE_ENGINE_PROXY, BusEngineProxy))
#define BUS_ENGINE_PROXY_CLASS(klass)     \
    (G_TYPE_CHECK_CLASS_CAST ((klass), BUS_TYPE_ENGINE_PROXY, BusEngineProxyClass))
#define BUS_IS_ENGINE_PROXY(obj)          \
    (G_TYPE_CHECK_INSTANCE_TYPE ((obj), BUS_TYPE_ENGINE_PROXY))
#define BUS_IS_ENGINE_PROXY_CLASS(klass)  \
    (G_TYPE_CHECK_CLASS_TYPE ((klass), BUS_TYPE_ENGINE_PROXY))
#define BUS_ENGINE_PROXY_GET_CLASS(obj)   \
    (G_TYPE_INSTANCE_GET_CLASS ((obj), BUS_TYPE_ENGINE_PROXY, BusEngineProxyClass))

G_BEGIN_DECLS

typedef struct _BusEngineProxy BusEngineProxy;
typedef struct _BusEngineProxyClass BusEngineProxyClass;

GType           bus_engine_proxy_get_type    (void);

/**
 * bus_engine_proxy_new:
 * @desc: the engine to create.
 * @timeout: timeout in msec, or -1 to use the default timeout value.
 * @cancellable: a object that could be used to cancel the operation.
 * @callback: a function to be called when the method invocation is done.
 * @user_data: a pointer that will be passed to the callback.
 */
void            bus_engine_proxy_new         (IBusEngineDesc     *desc,
                                              gint                timeout,
                                              GCancellable       *cancellable,
                                              GAsyncReadyCallback callback,
                                              gpointer            user_data);

/**
 * bus_engine_proxy_new_finish:
 * @res: A #GAsyncResult.
 * @error: Return location for error or %NULL.
 * @returns: On success, return an engine object. On error, return %NULL.
 *
 * Get the result of bus_engine_proxy_new() call. You have to call this
 * function in the #GAsyncReadyCallback function.
 */
BusEngineProxy *bus_engine_proxy_new_finish  (GAsyncResult       *res,
                                              GError            **error);

/**
 * bus_engine_proxy_get_desc:
 * @engine: A #BusEngineProxy.
 *
 * Get an #IBusEngineDesc object associated with the engine.
 */
IBusEngineDesc *bus_engine_proxy_get_desc    (BusEngineProxy     *engine);

/**
 * bus_engine_proxy_process_key_event:
 * @engine: A #BusEngineProxy.
 * @keyval: Key symbol of the key press.
 * @keycode: KeyCode of the key press.
 * @state: Key modifier flags.
 * @callback: A function to be called when the method invocation is done.
 * @user_data: Data supplied to @callback.
 *
 * Call "ProcessKeyEvent" method of an engine asynchronously.
 */
void            bus_engine_proxy_process_key_event
                                             (BusEngineProxy     *engine,
                                              guint               keyval,
                                              guint               keycode,
                                              guint               state,
                                              GAsyncReadyCallback callback,
                                              gpointer            user_data);
/**
 * bus_engine_proxy_set_cursor_location:
 * @engine: A #BusEngineProxy.
 * @x: X coordinate of the cursor.
 * @y: Y coordinate of the cursor.
 * @w: Width of the cursor.
 * @h: Height of the cursor.
 *
 * Call "SetCursorLocation" method of an engine asynchronously. Unlike
 * bus_engine_proxy_process_key_event(), there's no way to know the
 * result of the method invocation. If the same coordinate is given
 * twice or more, the function does nothing from the second time.
 */
void            bus_engine_proxy_set_cursor_location
                                             (BusEngineProxy     *engine,
                                              gint                x,
                                              gint                y,
                                              gint                w,
                                              gint                h);
/**
 * bus_engine_proxy_focus_in:
 * @engine: A #BusEngineProxy.
 * @object_path: An object path.
 * @client: A client name.
 *
 * Call "FocusIn" method of an engine asynchronously. Do nothing if
 * the engine already has a focus.
 */
void            bus_engine_proxy_focus_in    (BusEngineProxy     *engine,
                                              const gchar        *object_path,
                                              const gchar        *client);

/**
 * bus_engine_proxy_focus_out:
 * @engine: A #BusEngineProxy.
 * @object_path: An object path.
 *
 * Call "FocusOut" method of an engine asynchronously. Do nothing if
 * the engine does not have a focus.
 */
void            bus_engine_proxy_focus_out   (BusEngineProxy     *engine,
                                              const gchar        *object_path);

/**
 * bus_engine_proxy_reset:
 * @engine: A #BusEngineProxy.
 *
 * Call "Reset" method of an engine asynchronously.
 */
void            bus_engine_proxy_reset       (BusEngineProxy     *engine);

/**
 * bus_engine_proxy_set_capabilities:
 * @engine: A #BusEngineProxy.
 * @caps: Capabilities flags of IBusEngine, see #IBusCapabilite.
 *
 * Call "SetCapabilities" method of an engine asynchronously.
 */
void            bus_engine_proxy_set_capabilities
                                             (BusEngineProxy     *engine,
                                              guint               caps);
/**
 * bus_engine_proxy_page_up:
 * @engine: A #BusEngineProxy.
 *
 * Call "PageUp" method of an engine asynchronously.
 */
void            bus_engine_proxy_page_up     (BusEngineProxy     *engine);

/**
 * bus_engine_proxy_page_down:
 * @engine: A #BusEngineProxy.
 *
 * Call "PageDown" method of an engine asynchronously.
 */
void            bus_engine_proxy_page_down   (BusEngineProxy     *engine);

/**
 * bus_engine_proxy_cursor_up:
 * @engine: A #BusEngineProxy.
 *
 * Call "CursorUp" method of an engine asynchronously.
 */
void            bus_engine_proxy_cursor_up   (BusEngineProxy     *engine);

/**
 * bus_engine_proxy_cursor_down:
 * @engine: A #BusEngineProxy.
 *
 * Call "CursorDown" method of an engine asynchronously.
 */
void            bus_engine_proxy_cursor_down (BusEngineProxy     *engine);

/**
 * bus_engine_proxy_candidate_clicked:
 * @engine: A #BusEngineProxy.
 * @index:  Index of candidate be clicked.
 * @button: Mouse button.
 * @state:  Keyboard state.
 *
 * Call "CandidateClicked" method of an engine asynchronously.
 */
void            bus_engine_proxy_candidate_clicked
                                             (BusEngineProxy     *engine,
                                              guint               index,
                                              guint               button,
                                              guint               state);
/**
 * bus_engine_proxy_enable:
 * @engine: A #BusEngineProxy.
 *
 * Call "Enable" method of an engine asynchronously. Do nothing if the
 * engine is already enabled.
 */
void            bus_engine_proxy_enable      (BusEngineProxy     *engine);

/**
 * bus_engine_proxy_disable:
 * @engine: A #BusEngineProxy.
 *
 * Call "Disable" method of an engine asynchronously. Do nothing if
 * the engine is already disabled.
 */
void            bus_engine_proxy_disable     (BusEngineProxy     *engine);

/**
 * bus_engine_proxy_property_activate:
 * @engine: A #BusEngineProxy.
 * @name: Property name.
 * @state: Property state.
 *
 * Call "PropertyActivate" method of an engine asynchronously.
 */
void            bus_engine_proxy_property_activate
                                             (BusEngineProxy     *engine,
                                              const gchar        *prop_name,
                                              guint               state);
/**
 * bus_engine_proxy_property_show:
 * @engine: A #BusEngineProxy.
 * @prop_name: Property name.
 *
 * Call "PropertyShow" method of an engine asynchronously.
 */
void            bus_engine_proxy_property_show
                                             (BusEngineProxy     *engine,
                                              const gchar        *prop_name);
/**
 * bus_engine_proxy_property_hide:
 * @engine: A #BusEngineProxy.
 * @prop_name: Property name.
 *
 * Call "PropertyHide" method of an engine asynchronously.
 */
void            bus_engine_proxy_property_hide
                                             (BusEngineProxy     *engine,
                                              const gchar        *prop_name);
/**
 * bus_engine_proxy_is_enabled:
 * @engine: A #BusEngineProxy.
 * @returns: %TRUE if the engine is enabled.
 */
gboolean        bus_engine_proxy_is_enabled  (BusEngineProxy     *engine);

/**
 * bus_engine_proxy_set_surrounding_text:
 * @engine: A #BusEngineProxy.
 * @text: The surrounding text.
 * @cursor_pos: The cursor position on surrounding text.
 * @anchor_pos: The anchor position on selection area.
 *
 * Call "SetSurroundingText" method of an engine asynchronously.
 */
void            bus_engine_proxy_set_surrounding_text
                                             (BusEngineProxy     *engine,
                                              IBusText           *text,
                                              guint               cursor_pos,
                                              guint               anchor_pos);

/**
 * bus_engine_proxy_process_hand_writing_event:
 * @engine: A #BusEngineProxy.
 * @coordinates: A #GVariant containing an array of coordinates.
 *
 * Call "ProcessHandWritingEvent" method of an engine
 * asynchronously. The type of the GVariant should be "(ad)".  See
 * ibus_input_context_process_hand_writing_event() for details.
 */
void            bus_engine_proxy_process_hand_writing_event
                                             (BusEngineProxy     *engine,
                                              GVariant           *coordinates);

/**
 * bus_engine_proxy_cancel_hand_writing:
 * @engine: A #BusEngineProxy.
 * @n_strokes: The number of strokes to be removed. 0 means "remove all".
 *
 * Call "CancelHandWriting" method of an engine asynchronously.
 * See ibus_input_context_cancel_hand_writing() for details.
 */
void            bus_engine_proxy_cancel_hand_writing
                                             (BusEngineProxy     *engine,
                                              guint               n_strokes);

/**
 * bus_engine_proxy_set_content_type:
 * @engine: A #BusEngineProxy.
 * @purpose: Primary purpose of the input context, as an #IBusInputPurpose.
 * @hints: Hints that augment @purpose, as an #IBusInputHints.
 *
 * Call "SetContentType" method of an engine asynchronously.
 * See ibus_input_context_set_content_type() for details.
 */
void            bus_engine_proxy_set_content_type
                                             (BusEngineProxy     *engine,
                                              guint               purpose,
                                              guint               hints);

/**
 * bus_engine_proxy_get_properties:
 * @engine: A #BusEngineProxy.
 * @returns: An #IBusPropList.
 *
 * Get an #IBusPropList object associated with the engine.
 */
IBusPropList   *bus_engine_proxy_get_properties
                                             (BusEngineProxy     *engine);

/**
 * bus_engine_proxy_panel_extension_received:
 * @engine: A #BusEngineProxy.
 * @event: An #IBusExtensionEvent.
 *
 * Send an #IBusExtensionEvent to the engine.
 */
void            bus_engine_proxy_panel_extension_received
                                             (BusEngineProxy     *engine,
                                              IBusExtensionEvent *event);

/**
 * bus_engine_proxy_panel_extension_register_keys:
 * @engine: A #BusEngineProxy.
 * @parameters: A #GVariant array which includes the name and shortcut keys.
 *
 * Send shortcut keys to the engine to enable the extension.
 */
void            bus_engine_proxy_panel_extension_register_keys
                                             (BusEngineProxy     *engine,
                                              GVariant           *parameters);

G_END_DECLS
#endif
