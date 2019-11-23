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

#if !defined (__IBUS_H_INSIDE__) && !defined (IBUS_COMPILATION)
#error "Only <ibus.h> can be included directly"
#endif

#ifndef __IBUS_ENGINE_H_
#define __IBUS_ENGINE_H_

/**
 * SECTION: ibusengine
 * @short_description: Input method engine abstract.
 * @title: IBusEngine
 * @stability: Stable
 *
 * An IBusEngine provides infrastructure for input method engine.
 * Developers can "extend" this class for input method engine development.
 *
 * see_also: #IBusComponent, #IBusEngineDesc
 */

#include "ibusservice.h"
#include "ibusattribute.h"
#include "ibuslookuptable.h"
#include "ibusproplist.h"

/*
 * Type macros.
 */

/* define GOBJECT macros */
#define IBUS_TYPE_ENGINE             \
    (ibus_engine_get_type ())
#define IBUS_ENGINE(obj)             \
    (G_TYPE_CHECK_INSTANCE_CAST ((obj), IBUS_TYPE_ENGINE, IBusEngine))
#define IBUS_ENGINE_CLASS(klass)     \
    (G_TYPE_CHECK_CLASS_CAST ((klass), IBUS_TYPE_ENGINE, IBusEngineClass))
#define IBUS_IS_ENGINE(obj)          \
    (G_TYPE_CHECK_INSTANCE_TYPE ((obj), IBUS_TYPE_ENGINE))
#define IBUS_IS_ENGINE_CLASS(klass)  \
    (G_TYPE_CHECK_CLASS_TYPE ((klass), IBUS_TYPE_ENGINE))
#define IBUS_ENGINE_GET_CLASS(obj)   \
    (G_TYPE_INSTANCE_GET_CLASS ((obj), IBUS_TYPE_ENGINE, IBusEngineClass))

G_BEGIN_DECLS

typedef struct _IBusEngine IBusEngine;
typedef struct _IBusEngineClass IBusEngineClass;
typedef struct _IBusEnginePrivate IBusEnginePrivate;

/**
 * IBusEngine:
 * @enabled: Whether the engine is enabled.
 * @has_focus: Whether the engine has focus.
 * @cursor_area: Area of cursor.
 * @client_capabilities: IBusCapabilite (client capabilities) flags.
 *
 * IBusEngine properties.
 */
struct _IBusEngine {
    /*< private >*/
    IBusService parent;
    IBusEnginePrivate *priv;

    /* instance members */
    /*< public >*/
    gboolean enabled;
    gboolean has_focus;

    /* cursor location */
    IBusRectangle cursor_area;
    guint client_capabilities;
};

struct _IBusEngineClass {
    /*< private >*/
    IBusServiceClass parent;

    /* class members */
    /*< public >*/
    /* signals */
    gboolean    (* process_key_event)
                                    (IBusEngine     *engine,
                                     guint           keyval,
                                     guint           keycode,
                                     guint           state);
    void        (* focus_in)        (IBusEngine     *engine);
    void        (* focus_out)       (IBusEngine     *engine);
    void        (* reset)           (IBusEngine     *engine);
    void        (* enable)          (IBusEngine     *engine);
    void        (* disable)         (IBusEngine     *engine);
    void        (* set_cursor_location)
                                    (IBusEngine     *engine,
                                    gint             x,
                                    gint             y,
                                    gint             w,
                                    gint             h);
    void        (* set_capabilities)
                                    (IBusEngine     *engine,
                                     guint           caps);

    void        (* page_up)         (IBusEngine     *engine);
    void        (* page_down)       (IBusEngine     *engine);
    void        (* cursor_up)       (IBusEngine     *engine);
    void        (* cursor_down)     (IBusEngine     *engine);

    void        (* property_activate)
                                    (IBusEngine     *engine,
                                     const gchar    *prop_name,
                                     guint           prop_state);
    void        (* property_show)   (IBusEngine     *engine,
                                     const gchar    *prop_name);
    void        (* property_hide)   (IBusEngine     *engine,
                                     const gchar    *prop_name);
    void        (* candidate_clicked)
                                    (IBusEngine     *engine,
                                     guint           index,
                                     guint           button,
                                     guint           state);
    void        (* set_surrounding_text)
                                    (IBusEngine     *engine,
                                     IBusText       *text,
                                     guint           cursor_index,
                                     guint           anchor_pos);
    void        (* process_hand_writing_event)
                                    (IBusEngine     *engine,
                                     const gdouble  *coordinates,
                                     guint           coordinates_len);
    void        (* cancel_hand_writing)
                                    (IBusEngine     *engine,
                                     guint           n_strokes);
    void        (* set_content_type)
                                    (IBusEngine     *engine,
                                     guint           purpose,
                                     guint           hints);

    /*< private >*/
    /* padding */
    gpointer pdummy[4];
};

GType        ibus_engine_get_type       (void);

/**
 * ibus_engine_new:
 * @engine_name: Name of the IBusObject.
 * @object_path: Path for IBusService.
 * @connection: An opened GDBusConnection.
 *
 * Create a new #IBusEngine.
 *
 * Returns: A newly allocated IBusEngine.
 */
IBusEngine  *ibus_engine_new            (const gchar        *engine_name,
                                         const gchar        *object_path,
                                         GDBusConnection    *connection);
/**
 * ibus_engine_new_with_type:
 * @engine_type: GType of #IBusEngine.
 * @engine_name: Name of the IBusObject.
 * @object_path: Path for IBusService.
 * @connection: An opened GDBusConnection.
 *
 * Create a new #IBusEngine.
 *
 * Returns: A newly allocated IBusEngine.
 */
IBusEngine  *ibus_engine_new_with_type  (GType               engine_type,
                                         const gchar        *engine_name,
                                         const gchar        *object_path,
                                         GDBusConnection    *connection);


/**
 * ibus_engine_commit_text:
 * @engine: An IBusEngine.
 * @text: String commit to IBusEngine.
 *
 * Commit output of input method to IBus client.
 *
 * (Note: The text object will be released, if it is floating.
 *  If caller want to keep the object, caller should make the object
 *  sink by g_object_ref_sink.)
 */
void         ibus_engine_commit_text    (IBusEngine         *engine,
                                         IBusText           *text);

/**
 * ibus_engine_update_preedit_text:
 * @engine: An IBusEngine.
 * @text: Update content.
 * @cursor_pos: Current position of cursor
 * @visible: Whether the pre-edit buffer is visible.
 *
 * Update the pre-edit buffer.
 *
 * (Note: The text object will be released, if it is floating.
 *  If caller want to keep the object, caller should make the object
 *  sink by g_object_ref_sink.)
 */
void         ibus_engine_update_preedit_text
                                        (IBusEngine         *engine,
                                         IBusText           *text,
                                         guint               cursor_pos,
                                         gboolean            visible);

/**
 * ibus_engine_update_preedit_text_with_mode:
 * @engine: An IBusEngine.
 * @text: Update content.
 * @cursor_pos: Current position of cursor
 * @visible: Whether the pre-edit buffer is visible.
 * @mode: Pre-edit commit mode when the focus is lost.
 *
 * Update the pre-edit buffer with commit mode. Similar to
 * ibus_engine_update_preedit_text(), this function allows users to specify
 * the behavior on focus out when the pre-edit buffer is visible.
 *
 * If @mode is IBUS_ENGINE_PREEDIT_COMMIT, contents of the pre-edit buffer
 * will be committed and cleared.
 * If @mode is IBUS_ENGINE_PREEDIT_CLEAR, contents of the pre-edit buffer
 * will be cleared only.
 *
 * (Note: The text object will be released, if it is floating.
 *  If caller want to keep the object, caller should make the object
 *  sink by g_object_ref_sink.)
 */
void         ibus_engine_update_preedit_text_with_mode
                                        (IBusEngine              *engine,
                                         IBusText                *text,
                                         guint                    cursor_pos,
                                         gboolean                 visible,
                                         IBusPreeditFocusMode     mode);

/**
 * ibus_engine_show_preedit_text:
 * @engine: An IBusEngine.
 *
 * Show the pre-edit buffer.
 */
void         ibus_engine_show_preedit_text
                                        (IBusEngine         *engine);

/**
 * ibus_engine_hide_preedit_text:
 * @engine: An IBusEngine.
 *
 * Hide the pre-edit buffer.
 */
void         ibus_engine_hide_preedit_text
                                        (IBusEngine         *engine);

/**
 * ibus_engine_update_auxiliary_text:
 * @engine: An IBusEngine.
 * @text: Update content.
 * @visible: Whether the auxiliary text bar is visible.
 *
 * Update the auxiliary bar.
 *
 * (Note: The text object will be released, if it is floating.
 *  If caller want to keep the object, caller should make the object
 *  sink by g_object_ref_sink.)
 */
void         ibus_engine_update_auxiliary_text
                                        (IBusEngine        *engine,
                                         IBusText          *text,
                                         gboolean           visible);

/**
 * ibus_engine_show_auxiliary_text:
 * @engine: An IBusEngine.
 *
 * Show the auxiliary bar.
 */
void         ibus_engine_show_auxiliary_text
                                        (IBusEngine         *engine);

/**
 * ibus_engine_hide_auxiliary_text:
 * @engine: An IBusEngine.
 *
 * Hide the auxiliary bar.
 */
void         ibus_engine_hide_auxiliary_text
                                        (IBusEngine         *engine);

/**
 * ibus_engine_update_lookup_table:
 * @engine: An IBusEngine.
 * @lookup_table: An lookup_table.
 * @visible: Whether the lookup_table is visible.
 *
 * Update the lookup table.
 *
 * (Note: The table object will be released, if it is floating.
 *  If caller want to keep the object, caller should make the object
 *  sink by g_object_ref_sink.)
 */
void         ibus_engine_update_lookup_table
                                        (IBusEngine         *engine,
                                         IBusLookupTable    *lookup_table,
                                         gboolean            visible);

/**
 * ibus_engine_update_lookup_table_fast:
 * @engine: An IBusEngine.
 * @lookup_table: An lookup_table.
 * @visible: Whether the lookup_table is visible.
 *
 * Fast update for big lookup table.
 *
 * If size of lookup table is not over table page size *4,
 * then it calls ibus_engine_update_lookup_table().
 *
 * (Note: The table object will be released, if it is floating.
 *  If caller want to keep the object, caller should make the object
 *  sink by g_object_ref_sink.)
 */
void         ibus_engine_update_lookup_table_fast
                                        (IBusEngine         *engine,
                                         IBusLookupTable    *lookup_table,
                                         gboolean            visible);

/**
 * ibus_engine_show_lookup_table:
 * @engine: An IBusEngine.
 *
 * Show the lookup table.
 */
void         ibus_engine_show_lookup_table
                                        (IBusEngine         *engine);

/**
 * ibus_engine_hide_lookup_table:
 * @engine: An IBusEngine.
 *
 * Hide the lookup table.
 */
void         ibus_engine_hide_lookup_table
                                        (IBusEngine         *engine);

/**
 * ibus_engine_forward_key_event:
 * @engine: An IBusEngine.
 * @keyval: KeySym.
 * @keycode: keyboard scancode.
 * @state: Key modifier flags.
 *
 * Forward the key event.
 */
void         ibus_engine_forward_key_event
                                        (IBusEngine         *engine,
                                         guint               keyval,
                                         guint               keycode,
                                         guint               state);

/**
 * ibus_engine_register_properties:
 * @engine: An IBusEngine.
 * @prop_list: Property List.
 *
 * Register and show properties in language bar.
 *
 * (Note: The prop_list object will be released, if it is floating.
 *  If caller want to keep the object, caller should make the object
 *  sink by g_object_ref_sink.)
 */
void         ibus_engine_register_properties
                                        (IBusEngine         *engine,
                                         IBusPropList       *prop_list);

/**
 * ibus_engine_update_property:
 * @engine: An IBusEngine.
 * @prop: IBusProperty to be updated.
 *
 * Update the state displayed in language bar.
 *
 * (Note: The prop object will be released, if it is floating.
 *  If caller want to keep the object, caller should make the object
 *  sink by g_object_ref_sink.)
 */
void         ibus_engine_update_property(IBusEngine         *engine,
                                         IBusProperty       *prop);

/**
 * ibus_engine_delete_surrounding_text:
 * @engine: An IBusEngine.
 * @offset: The offset of the first char.
 * @nchars: Number of chars to be deleted.
 *
 * Delete surrounding text.
 */
void ibus_engine_delete_surrounding_text(IBusEngine         *engine,
                                         gint                offset,
                                         guint               nchars);

/**
 * ibus_engine_get_surrounding_text:
 * @engine: An IBusEngine.
 * @text: (out) (transfer none) (allow-none): Location to store surrounding text.
 * @cursor_pos: (out) (allow-none): Cursor position in characters in @text.
 * @anchor_pos: (out) (allow-none): Anchor position of selection in @text.
 *
 * Get surrounding text.
 *
 * It is also used to tell the input-context that the engine will
 * utilize surrounding-text.  In that case, it must be called in
 * #IBusEngine::enable handler, with both @text and @cursor set to
 * %NULL.
 *
 * See also: #IBusEngine::set-surrounding-text
 */
void ibus_engine_get_surrounding_text   (IBusEngine         *engine,
                                         IBusText          **text,
                                         guint              *cursor_pos,
                                         guint              *anchor_pos);


/**
 * ibus_engine_get_content_type:
 * @engine: An #IBusEngine.
 * @purpose: (out) (allow-none): Primary purpose of the input context.
 * @hints: (out) (allow-none): Hints that augument @purpose.
 *
 * Get content-type (primary purpose and hints) of the current input
 * context.
 *
 * See also: #IBusEngine::set-content-type
 */
void ibus_engine_get_content_type       (IBusEngine         *engine,
                                         guint              *purpose,
                                         guint              *hints);

/**
 * ibus_engine_get_name:
 * @engine: An IBusEngine.
 *
 * Return the name of #IBusEngine.
 *
 * Returns: Name of #IBusEngine.
 */
const gchar *ibus_engine_get_name       (IBusEngine         *engine);

G_END_DECLS
#endif
