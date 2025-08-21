/* -*- mode: C; c-basic-offset: 4; indent-tabs-mode: nil; -*- */
/* vim:set et sts=4: */
/* ibus - The Input Bus
 * Copyright (C) 2008-2013 Peng Huang <shawn.p.huang@gmail.com>
 * Copyright (C) 2018-2025 Takao Fujiwara <takao.fujiwara1@gmail.com>
 * Copyright (C) 2008-2025 Red Hat, Inc.
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

#ifndef __IBUS_INPUT_CONTEXT_H_
#define __IBUS_INPUT_CONTEXT_H_

/**
 * SECTION: ibusinputcontext
 * @short_description: IBus input context proxy object.
 * @stability: Stable
 *
 * An IBusInputContext is a proxy object of BusInputContext,
 * which manages the context for input methods that supports
 * text input in various natural languages.
 *
 * Clients call the IBusInputContext to invoke BusInputContext,
 * through which invokes IBusEngine.
 */
#include "ibusproxy.h"
#include "ibusenginedesc.h"
#include "ibustext.h"

/*
 * Type macros.
 */

/* define GOBJECT macros */
#define IBUS_TYPE_INPUT_CONTEXT             \
    (ibus_input_context_get_type ())
#define IBUS_INPUT_CONTEXT(obj)             \
    (G_TYPE_CHECK_INSTANCE_CAST ((obj), IBUS_TYPE_INPUT_CONTEXT, IBusInputContext))
#define IBUS_INPUT_CONTEXT_CLASS(klass)     \
    (G_TYPE_CHECK_CLASS_CAST ((klass), IBUS_TYPE_INPUT_CONTEXT, IBusInputContextClass))
#define IBUS_IS_INPUT_CONTEXT(obj)          \
    (G_TYPE_CHECK_INSTANCE_TYPE ((obj), IBUS_TYPE_INPUT_CONTEXT))
#define IBUS_IS_INPUT_CONTEXT_CLASS(klass)  \
    (G_TYPE_CHECK_CLASS_TYPE ((klass), IBUS_TYPE_INPUT_CONTEXT))
#define IBUS_INPUT_CONTEXT_GET_CLASS(obj)   \
    (G_TYPE_INSTANCE_GET_CLASS ((obj), IBUS_TYPE_INPUT_CONTEXT, IBusInputContextClass))

G_BEGIN_DECLS

typedef struct _IBusInputContext IBusInputContext;
typedef struct _IBusInputContextClass IBusInputContextClass;

/**
 * IBusInputContext:
 *
 * An opaque data type representing an IBusInputContext.
 */
struct _IBusInputContext {
    IBusProxy parent;
    /* instance members */
};

struct _IBusInputContextClass {
    IBusProxyClass parent;
    /* signals */

    /*< private >*/
    /* padding */
    gpointer pdummy[24];
};

GType        ibus_input_context_get_type    (void);

/**
 * ibus_input_context_new:
 * @path: The path to the object that emitting the signal.
 * @connection: A #GDBusConnection.
 * @cancellable: A #GCancellable or %NULL.
 * @error: Return location for error or %NULL.
 *
 * Creates a new #IBusInputContext.
 *
 * Returns: A newly allocated #IBusInputContext.
 */
IBusInputContext *
             ibus_input_context_new         (const gchar        *path,
                                             GDBusConnection    *connection,
                                             GCancellable       *cancellable,
                                             GError            **error);
/**
 * ibus_input_context_new_async:
 * @path: The path to the object that emitting the signal.
 * @connection: A #GDBusConnection.
 * @cancellable: A #GCancellable or %NULL.
 * @callback: A #GAsyncReadyCallback to call when the request is satisfied.
 *      The callback should not be %NULL.
 * @user_data: The data to pass to callback.
 *
 * Creates a new #IBusInputContext asynchronously.
 */
void         ibus_input_context_new_async   (const gchar        *path,
                                             GDBusConnection    *connection,
                                             GCancellable       *cancellable,
                                             GAsyncReadyCallback callback,
                                             gpointer            user_data);

/**
 * ibus_input_context_new_async_finish:
 * @res: A #GAsyncResult obtained from the #GAsyncReadyCallback pass to
 *      ibus_input_context_new_async().
 * @error: Return location for error or %NULL.
 *
 * Finishes an operation started with ibus_input_context_new_async().
 *
 * Returns: A newly allocated #IBusInputContext.
 */
IBusInputContext *
             ibus_input_context_new_async_finish
                                            (GAsyncResult       *res,
                                             GError            **error);
/**
 * ibus_input_context_get_input_context:
 * @path: The path to the object that emitting the signal.
 * @connection: A GDBusConnection.
 *
 * Gets an existing IBusInputContext.
 *
 * Returns: (transfer none): An existing #IBusInputContext.
 */
IBusInputContext *
             ibus_input_context_get_input_context
                                            (const gchar        *path,
                                             GDBusConnection    *connection);
/**
 * ibus_input_context_get_input_context_async:
 * @path: The path to the object that emitting the signal.
 * @connection: A #GDBusConnection.
 * @cancellable: A #GCancellable or %NULL.
 * @callback: A #GAsyncReadyCallback to call when the request is satisfied.
 *      The callback should not be %NULL.
 * @user_data: The data to pass to callback.
 *
 * Gets an existing #IBusInputContext asynchronously.
 */
void         ibus_input_context_get_input_context_async
                                            (const gchar        *path,
                                             GDBusConnection    *connection,
                                             GCancellable       *cancellable,
                                             GAsyncReadyCallback callback,
                                             gpointer            user_data);

/**
 * ibus_input_context_get_input_context_async_finish:
 * @res: A #GAsyncResult obtained from the #GAsyncReadyCallback pass to
 *      ibus_input_context_get_input_context_async().
 * @error: Return location for error or %NULL.
 *
 * Finishes an operation started with
 * ibus_input_context_get_input_context_async().
 *
 * Returns: (transfer none): An existing #IBusInputContext.
 */
IBusInputContext *
             ibus_input_context_get_input_context_async_finish
                                            (GAsyncResult       *res,
                                             GError            **error);

/**
 * ibus_input_context_process_hand_writing_event:
 * @context: An IBusInputContext.
 * @coordinates: An array of gdouble (0.0 to 1.0) which represents a stroke (i.e. [x1, y1, x2, y2, x3, y3, ...]).
 * @coordinates_len: The number of elements in the array. The number should be even and >= 4.
 *
 * Pass a handwriting stroke to an input method engine.
 *
 * In this API, a coordinate (0.0, 0.0) represents the top-left corner of an area for
 * handwriting, and (1.0, 1.0) does the bottom-right. Therefore, for example, if
 * a user writes a character 'L', the array would be something like [0.0, 0.0, 0.0, 1.0, 1.0, 1.0]
 * and coordinates_len would be 6.
 *
 * The function is usually called when a user releases the mouse button in a hand
 * writing area.
 *
 * see_also: #IBusEngine::process-hand-writing-event
 */
void         ibus_input_context_process_hand_writing_event
                                            (IBusInputContext   *context,
                                             const gdouble      *coordinates,
                                             guint               coordinates_len);

/**
 * ibus_input_context_cancel_hand_writing:
 * @context: An IBusInputContext.
 * @n_strokes: The number of strokes to be removed. Pass 0 to remove all.
 *
 * Clear handwriting stroke(s) in the current input method engine.
 *
 * see_also: #IBusEngine::cancel-hand-writing
 */
void         ibus_input_context_cancel_hand_writing
                                            (IBusInputContext   *context,
                                             guint               n_strokes);

/**
 * ibus_input_context_process_key_event_async:
 * @context: An IBusInputContext.
 * @keyval: Key symbol of a key event.
 * @keycode: Keycode of a key event.
 * @state: Key modifier flags.
 * @timeout_msec: The timeout in milliseconds or -1 to use the default timeout.
 * @cancellable: A GCancellable or NULL.
 * @callback: A GAsyncReadyCallback to call when the request is satisfied or NULL
 *      if you don't care about the result of the method invocation.
 * @user_data: The data to pass to callback.
 *
 * Pass the key event to input method engine.
 *
 * Key symbols are characters/symbols produced by key press, for example,
 * pressing "s" generates key symbol "s"; pressing shift-"s" generates key symbol "S".
 * Same key on keyboard may produce different key symbols on different keyboard layout.
 * e.g., "s" key on QWERTY keyboard produces "o" in DVORAK layout.
 *
 * Unlike key symbol, keycode is only determined by the location of the key, and
 * irrelevant of the keyboard layout.
 *
 * Briefly speaking, input methods that expect certain keyboard layout should use
 * keycode; otherwise keyval is sufficient.
 * For example, Chewing, Cangjie, Wubi expect an en-US QWERTY keyboard, these should
 * use keycode; while pinyin can rely on keyval only, as it is less sensitive to
 * the keyboard layout change, DVORAK users can still use DVORAK layout to input pinyin.
 *
 * Use ibus_keymap_lookup_keysym() to convert keycode to keysym in given keyboard layout.
 *
 * see_also: #IBusEngine::process-key-event
 */
void        ibus_input_context_process_key_event_async
                                            (IBusInputContext   *context,
                                             guint32             keyval,
                                             guint32             keycode,
                                             guint32             state,
                                             gint                timeout_msec,
                                             GCancellable       *cancellable,
                                             GAsyncReadyCallback callback,
                                             gpointer            user_data);

/**
 * ibus_input_context_process_key_event_async_finish:
 * @context: An #IBusInputContext.
 * @res: A #GAsyncResult obtained from the #GAsyncReadyCallback passed to
 *      ibus_input_context_process_key_event_async().
 * @error: Return location for error or %NULL.
 *
 * Finishes an operation started with
 *      ibus_input_context_process_key_event_async().
 *
 * Returns: %TRUE if the key event is processed;
 *      %FALSE otherwise or some errors happen and the @error will be set.
 */
gboolean     ibus_input_context_process_key_event_async_finish
                                            (IBusInputContext   *context,
                                             GAsyncResult       *res,
                                             GError            **error);

/**
 * ibus_input_context_process_key_event:
 * @context: An #IBusInputContext.
 * @keyval: Key symbol of a key event.
 * @keycode: Keycode of a key event.
 * @state: Key modifier flags.
 *
 * Pass the key event to input method engine and wait for the reply from
 * ibus (i.e. synchronous IPC).
 *
 * Returns: %TRUE for successfully process the key; %FALSE otherwise.
 *
 * See also: ibus_input_context_process_key_event_async()
 */
gboolean     ibus_input_context_process_key_event
                                            (IBusInputContext   *context,
                                             guint32             keyval,
                                             guint32             keycode,
                                             guint32             state);

/**
 * ibus_input_context_set_cursor_location:
 * @context: An IBusInputContext.
 * @x: X coordinate of the cursor.
 * @y: Y coordinate of the cursor.
 * @w: Width of the cursor.
 * @h: Height of the cursor.
 *
 * Set the cursor location of IBus input context asynchronously.
 *
 * see_also: #IBusEngine::set-cursor-location
 */
void         ibus_input_context_set_cursor_location
                                            (IBusInputContext   *context,
                                             gint32              x,
                                             gint32              y,
                                             gint32              w,
                                             gint32              h);
/**
 * ibus_input_context_set_cursor_location_relative:
 * @context: An IBusInputContext.
 * @x: X coordinate of the cursor.
 * @y: Y coordinate of the cursor.
 * @w: Width of the cursor.
 * @h: Height of the cursor.
 *
 * Set the relative cursor location of IBus input context asynchronously.
 */
void         ibus_input_context_set_cursor_location_relative
                                            (IBusInputContext   *context,
                                             gint32              x,
                                             gint32              y,
                                             gint32              w,
                                             gint32              h);
/**
 * ibus_input_context_set_capabilities:
 * @context: An IBusInputContext.
 * @capabilities: Capabilities flags of IBusEngine, see #IBusCapabilite
 *
 * Set the capabilities flags of client application asynchronously.
 * When IBUS_CAP_FOCUS is not set, IBUS_CAP_PREEDIT_TEXT, IBUS_CAP_AUXILIARY_TEXT, IBUS_CAP_LOOKUP_TABLE, and IBUS_CAP_PROPERTY have to be all set.
 * The panel component does nothing for an application that doesn't support focus.
 *
 * see_also: #IBusEngine::set-capabilities
 */
void         ibus_input_context_set_capabilities
                                            (IBusInputContext   *context,
                                             guint32             capabilities);

/**
 * ibus_input_context_property_activate:
 * @context: An #IBusInputContext.
 * @prop_name: A property name (e.g. "InputMode.WideLatin")
 * @state: A status of the property (e.g. PROP_STATE_CHECKED)
 *
 * Activate the property asynchronously.
 *
 * See also: #IBusEngine::property_activate
 */
void         ibus_input_context_property_activate
                                            (IBusInputContext *context,
                                             const gchar      *prop_name,
                                             guint32           state);

/**
 * ibus_input_context_focus_in:
 * @context: An #IBusInputContext.
 *
 * Invoked when the client application get focus. An asynchronous IPC will
 * be performed.
 *
 * see_also: #IBusEngine::focus_in.
 */
void         ibus_input_context_focus_in    (IBusInputContext   *context);

/**
 * ibus_input_context_focus_out:
 * @context: An #IBusInputContext.
 *
 * Invoked when the client application get focus. An asynchronous IPC will be performed.
 *
 * see_also: #IBusEngine::focus_out.
 */
void         ibus_input_context_focus_out   (IBusInputContext   *context);


/**
 * ibus_input_context_reset:
 * @context: An #IBusInputContext.
 *
 * Invoked when the IME is reset. An asynchronous IPC will be performed.
 *
 * see_also: #IBusEngine::reset
 */
void         ibus_input_context_reset       (IBusInputContext   *context);

/**
 * ibus_input_context_get_engine_async:
 * @context: An #IBusInputContext.
 * @timeout_msec: The timeout in milliseconds or -1 to use the default timeout.
 * @cancellable: A #GCancellable or %NULL.
 * @callback: A #GAsyncReadyCallback to call when the request is satisfied or
 *     %NULL if you don't care about the result of the method invocation.
 * @user_data: The data to pass to callback.
 *
 * An asynchronous IPC will be performed.
 */
void         ibus_input_context_get_engine_async
                                            (IBusInputContext   *context,
                                             gint                timeout_msec,
                                             GCancellable       *cancellable,
                                             GAsyncReadyCallback callback,
                                             gpointer            user_data);

/**
 * ibus_input_context_get_engine_async_finish:
 * @context: An #IBusInputContext.
 * @res: A #GAsyncResult obtained from the #GAsyncReadyCallback passed to
 *   ibus_input_context_get_engine_async().
 * @error: Return location for error or %NULL.
 *
 * Finishes an operation started with ibus_input_context_get_engine_async().
 *
 * Returns: (transfer none): An IME engine description for the context, or
 *     %NULL.
 */
IBusEngineDesc *
             ibus_input_context_get_engine_async_finish
                                            (IBusInputContext   *context,
                                             GAsyncResult       *res,
                                             GError            **error);

/**
 * ibus_input_context_get_engine:
 * @context: An #IBusInputContext.
 *
 * Gets an IME engine description for the context.
 * A synchronous IPC will be performed.
 *
 * Returns: (transfer none): An IME engine description for the context, or NULL.
 */
IBusEngineDesc *
             ibus_input_context_get_engine  (IBusInputContext   *context);

/**
 * ibus_input_context_set_engine:
 * @context: An #IBusInputContext.
 * @name: A name of the engine.
 *
 * Invoked when the IME engine is changed.
 * An asynchronous IPC will be performed.
 */
void         ibus_input_context_set_engine  (IBusInputContext   *context,
                                             const gchar        *name);

/**
 * ibus_input_context_set_surrounding_text:
 * @context: An #IBusInputContext.
 * @text: An #IBusText surrounding the current cursor on the application.
 * @cursor_pos: Current cursor position in characters in @text.
 * @anchor_pos: Anchor position of selection in @text.
*/
void         ibus_input_context_set_surrounding_text
                                            (IBusInputContext   *context,
                                             IBusText           *text,
                                             guint32             cursor_pos,
                                             guint32             anchor_pos);

/**
 * ibus_input_context_needs_surrounding_text:
 * @context: An #IBusInputContext.
 *
 * Check whether the current engine requires surrounding-text.
 *
 * Returns: %TRUE if surrounding-text is needed by the current engine;
 * %FALSE otherwise.
 */
gboolean     ibus_input_context_needs_surrounding_text
                                            (IBusInputContext   *context);

/**
 * ibus_input_context_set_content_type:
 * @context: An #IBusInputContext.
 * @purpose: Primary purpose of the input context, as an #IBusInputPurpose.
 * @hints: Hints that augment @purpose, as an #IBusInputHints.
 *
 * Set content-type (primary purpose and hints) of the context.  This
 * information is particularly useful to implement intelligent
 * behavior in engines, such as automatic input-mode switch and text
 * prediction.  For example, to restrict input to numbers, the client
 * can call this function with @purpose set to
 * #IBUS_INPUT_PURPOSE_NUMBER.
 *
 * See also: #IBusEngine::set-content-type
 */
void         ibus_input_context_set_content_type
                                            (IBusInputContext   *context,
                                             guint               purpose,
                                             guint               hints);

/**
 * ibus_input_context_set_client_commit_preedit:
 * @context: An #IBusInputContext.
 * @client_commit: %TRUE if your input context commits pre-edit texts
 *     with Space or Enter key events or mouse click events. %FALSE if
 *     ibus-daemon commits pre-edit texts with those events.
 *     The default is %FALSE. The behavior is decided with
 *     ibus_engine_update_preedit_text_with_mode() to commit, clear or
 *     keep the pre-edit text and this API is important in ibus-hangul.
 *
 * Set whether #IBusInputContext commits pre-edit texts or not.
 * If %TRUE, 'update-preedit-text-with-mode' signal is emitted
 * instead of 'update-preedit-text' signal.
 * If your client receives the 'update-preedit-text-with-mode' signal,
 * the client needs to implement commit_text() of pre-edit text when
 * GtkIMContextClass.focus_out() is called in case an IME desires that
 * behavior but it depends on each IME.
 *
 * See also ibus_engine_update_preedit_text_with_mode().
 */
void         ibus_input_context_set_client_commit_preedit
                                            (IBusInputContext   *context,
                                             gboolean            client_commit);

/**
 * ibus_input_context_set_post_process_key_event:
 * @context: An #IBusInputContext.
 * @enable: Can use ibus_input_context_post_process_key_event() to retrieve
 * commit-text and forwar-key-event signals during
 * calling ibus_input_context_process_key_event() if it's %TRUE.
 *
 * Since: 1.5.29
 * Stability: Unstable
 */
void         ibus_input_context_set_post_process_key_event
                                            (IBusInputContext   *context,
                                             gboolean            enable);
/**
 * ibus_input_context_post_process_key_event:
 * @context: An #IBusInputContext.
 *
 * Call this API after ibus_input_context_process_key_event() returns
 * to retrieve commit-text and forwar-key-event signals during
 * calling ibus_input_context_process_key_event().
 *
 * See also ibus_input_context_set_post_process_key_event().
 *
 * Since: 1.5.29
 * Stability: Unstable
 */
void         ibus_input_context_post_process_key_event
                                            (IBusInputContext   *context);

/**
 * ibus_input_context_set_preedit_format:
 * @context: An #IBusInputContext.
 * @format: An #IBusPreeditFormat.
 *
 * The pre-edit attributes follows the format and the default is
 * #IBUS_PREEDIT_FORMAT_RGBA and the types of all #IBusAttribute are should be
 * one of  #IBUS_ATTR_TYPE_UNDERLINE, #IBUS_ATTR_TYPE_FOREGROUND,
 * #IBUS_ATTR_TYPE_BACKGROUND.
 * In case that the format is #IBUS_PREEDIT_FORMAT_HINT, the types of all
 * #IBusAttribute are #IBUS_ATTR_TYPE_HINT.
 *
 * See also ibus_text_get_attributes();
 *
 * Since: 1.5.33
 * Stability: Unstable
 */
void         ibus_input_context_set_preedit_format
                                            (IBusInputContext   *context,
                                             IBusPreeditFormat   format);
G_END_DECLS
#endif
