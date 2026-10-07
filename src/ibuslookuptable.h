/* -*- mode: C; c-basic-offset: 4; indent-tabs-mode: nil; -*- */
/* vim:set et sts=4: */
/* IBus - The Input Bus
 * Copyright (C) 2008-2013 Peng Huang <shawn.p.huang@gmail.com>
 * Copyright (C) 2008-2013 Red Hat, Inc.
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

#if !defined (__IBUS_H_INSIDE__) && !defined (IBUS_COMPILATION)
#error "Only <ibus.h> can be included directly"
#endif

#ifndef __IBUS_LOOKUP_TABLE_H_
#define __IBUS_LOOKUP_TABLE_H_

/**
 * SECTION: ibuslookuptable
 * @short_description: Candidate word/phrase lookup table.
 * @stability: Stable
 *
 * An IBusLookuptable stores the candidate words or phrases for users to
 * choose from.
 *
 * Use ibus_engine_update_lookup_table(), ibus_engine_show_lookup_table(),
 * and ibus_engine_hide_lookup_table() to update, show and hide the lookup
 * table.
 *
 * see_also: #IBusEngine
 */

#include "ibusserializable.h"
#include "ibustext.h"

/*
 * Type macros.
 */
/* define IBusLookupTable macros */
#define IBUS_TYPE_LOOKUP_TABLE             \
    (ibus_lookup_table_get_type ())
#define IBUS_LOOKUP_TABLE(obj)             \
    (G_TYPE_CHECK_INSTANCE_CAST ((obj), IBUS_TYPE_LOOKUP_TABLE, IBusLookupTable))
#define IBUS_LOOKUP_TABLE_CLASS(klass)     \
    (G_TYPE_CHECK_CLASS_CAST ((klass), IBUS_TYPE_LOOKUP_TABLE, IBusLookupTableClass))
#define IBUS_IS_LOOKUP_TABLE(obj)          \
    (G_TYPE_CHECK_INSTANCE_TYPE ((obj), IBUS_TYPE_LOOKUP_TABLE))
#define IBUS_IS_LOOKUP_TABLE_CLASS(klass)  \
    (G_TYPE_CHECK_CLASS_TYPE ((klass), IBUS_TYPE_LOOKUP_TABLE))
#define IBUS_LOOKUP_TABLE_GET_CLASS(obj)   \
    (G_TYPE_INSTANCE_GET_CLASS ((obj), IBUS_TYPE_LOOKUP_TABLE, IBusLookupTableClass))


G_BEGIN_DECLS

typedef struct _IBusLookupTable IBusLookupTable;
typedef struct _IBusLookupTableClass IBusLookupTableClass;

/**
 * IBusLookupTable:
 * @page_size: number of candidate shown per page.
 * @cursor_pos: position index of cursor.
 * @cursor_visible: whether the cursor is visible.
 * @round: TRUE for lookup table wrap around.
 * @orientation: orientation of the table.
 * @candidates: Candidate words/phrases.
 * @labels: Candidate labels which identify individual candidates in the same page. Default is 1, 2, 3, 4 ...
 *
 * An IBusLookuptable stores the candidate words or phrases for users to choose from.
 * Note that some input methods allow you to select candidate by pressing non-numeric
 * keys such as "asdfghjkl;".
 * Developers of these input methods should change the labels with
 * ibus_lookup_table_append_label().
 */
struct _IBusLookupTable {
    IBusSerializable parent;

    /*< public >*/
    guint page_size;
    guint cursor_pos;
    gboolean cursor_visible;
    gboolean round;
    gint orientation;

    GArray *candidates;
    GArray *labels;
};

struct _IBusLookupTableClass {
    IBusSerializableClass parent;
};


GType                ibus_lookup_table_get_type (void);

/**
 * ibus_lookup_table_new:
 * @page_size: number of candidate shown per page, the max value is 16.
 * @cursor_pos: position index of cursor.
 * @cursor_visible: whether the cursor is visible.
 * @round: TRUE for lookup table wrap around.
 *
 * Craetes a new #IBusLookupTable.
 *
 * Returns: A newly allocated #IBusLookupTable.
 */
IBusLookupTable     *ibus_lookup_table_new      (guint               page_size,
                                                 guint               cursor_pos,
                                                 gboolean            cursor_visible,
                                                 gboolean            round);

/**
 * ibus_lookup_table_append_candidate:
 * @table: An IBusLookupTable.
 * @text: candidate word/phrase to be appended (in IBusText format).
 *
 * Append a candidate word/phrase to IBusLookupTable, and increase reference.
 */
void                 ibus_lookup_table_append_candidate
                                                (IBusLookupTable    *table,
                                                 IBusText           *text);

/**
 * ibus_lookup_table_get_number_of_candidates:
 * @table: An IBusLookupTable.
 *
 * Return the number of candidate in the table.
 *
 * Returns: The number of candidates in the table
 */
guint               ibus_lookup_table_get_number_of_candidates
                                                (IBusLookupTable    *table);

/**
 * ibus_lookup_table_get_candidate:
 * @table: An IBusLookupTable.
 * @index: Index in the Lookup table.
 *
 * Return #IBusText at the given index. Borrowed reference.
 *
 * Returns: (transfer none): IBusText at the given index; NULL if no such
 *         #IBusText.
 */
IBusText            *ibus_lookup_table_get_candidate
                                                (IBusLookupTable    *table,
                                                 guint               index);

/**
 * ibus_lookup_table_append_label:
 * @table: An IBusLookupTable.
 * @text: A candidate label to be appended (in IBusText format).
 *
 * Append a candidate word/phrase to IBusLookupTable, and increase reference.
 * This function is needed if the input method select candidate with
 * non-numeric keys such as "asdfghjkl;".
 */
void                 ibus_lookup_table_append_label
                                                (IBusLookupTable    *table,
                                                 IBusText           *text);

/**
 * ibus_lookup_table_set_label:
 * @table: An IBusLookupTable.
 * @index: Intex in the Lookup table.
 * @text: A candidate label to be appended (in IBusText format).
 *
 * Append a candidate word/phrase to IBusLookupTable, and increase reference.
 * This function is needed if the input method select candidate with
 * non-numeric keys such as "asdfghjkl;".
 */
void                 ibus_lookup_table_set_label
                                                (IBusLookupTable    *table,
                                                 guint               index,
                                                 IBusText           *text);

/**
 * ibus_lookup_table_get_label:
 * @table: An IBusLookupTable.
 * @index: Index in the Lookup table.
 *
 * Return #IBusText at the given index. Borrowed reference.
 *
 * Returns: (transfer none): #IBusText at the given index; %NULL if no such
 *         #IBusText.
 */
IBusText            *ibus_lookup_table_get_label
                                                (IBusLookupTable    *table,
                                                 guint               index);


/**
 * ibus_lookup_table_set_cursor_pos:
 * @table: An IBusLookupTable.
 * @cursor_pos: The position of cursor.
 *
 * Set the cursor position of IBusLookupTable.
 */
void                 ibus_lookup_table_set_cursor_pos
                                                (IBusLookupTable    *table,
                                                 guint               cursor_pos);

/**
 * ibus_lookup_table_get_cursor_pos:
 * @table: An IBusLookupTable.
 *
 * Gets the cursor position of #IBusLookupTable.
 *
 * Returns: The position of cursor.
 */
guint                ibus_lookup_table_get_cursor_pos
                                                (IBusLookupTable    *table);

/**
 * ibus_lookup_table_set_cursor_visible:
 * @table: An IBusLookupTable.
 * @visible: Whether to make the cursor of @table visible.
 *
 * Set whether to make the cursor of an IBusLookupTable visible or not.
 */
void                 ibus_lookup_table_set_cursor_visible
                                                (IBusLookupTable    *table,
                                                 gboolean            visible);

/**
 * ibus_lookup_table_is_cursor_visible:
 * @table: An #IBusLookupTable.
 *
 * Returns whether the cursor of an #IBusLookupTable is visible.
 *
 * Returns: Whether the cursor of @table is visible.
 */
gboolean             ibus_lookup_table_is_cursor_visible
                                                (IBusLookupTable    *table);

/**
 * ibus_lookup_table_get_cursor_in_page:
 * @table: An IBusLookupTable.
 *
 * Gets the cursor position in current page of #IBusLookupTable.
 *
 * Returns: The position of cursor in current page.
 */
guint                ibus_lookup_table_get_cursor_in_page
                                                (IBusLookupTable    *table);

/**
 * ibus_lookup_table_set_page_size:
 * @table: An IBusLookupTable.
 * @page_size: number of candidate shown per page.
 *
 * Set the number of candidate shown per page.
 */
void                 ibus_lookup_table_set_page_size
                                                (IBusLookupTable    *table,
                                                 guint               page_size);
/**
 * ibus_lookup_table_get_page_size:
 * @table: An IBusLookupTable.
 *
 * Gets the number of candidate shown per page.
 *
 * Returns: Page size, i.e., number of candidate shown per page.
dd
 */
guint                ibus_lookup_table_get_page_size
                                                (IBusLookupTable    *table);

/**
 * ibus_lookup_table_set_round:
 * @table: An IBusLookupTable.
 * @round: Whether to make @table round.
 *
 * Set whether to make the IBusLookupTable round or not.
 */
void                 ibus_lookup_table_set_round
                                                (IBusLookupTable    *table,
                                                 gboolean            round);
/**
 * ibus_lookup_table_is_round:
 * @table: An IBusLookupTable.
 *
 * Returns whether the #IBusLookupTable is round.
 *
 * Returns: Whether the @table is round.
 */
gboolean             ibus_lookup_table_is_round (IBusLookupTable    *table);

/**
 * ibus_lookup_table_set_orientation:
 * @table: An IBusLookupTable.
 * @orientation: .
 *
 * Set the orientation.
 */
void                 ibus_lookup_table_set_orientation
                                                (IBusLookupTable    *table,
                                                 gint                orientation);

/**
 * ibus_lookup_table_get_orientation:
 * @table: An IBusLookupTable.
 *
 * Returns the orientation of the #IBusLookupTable.
 *
 * Returns: The orientation of the @table.
 */
gint                 ibus_lookup_table_get_orientation
                                                (IBusLookupTable    *table);


/**
 * ibus_lookup_table_clear:
 * @table: An IBusLookupTable.
 *
 * Clear and remove all candidate from an IBusLookupTable.
 */
void                 ibus_lookup_table_clear    (IBusLookupTable    *table);

/**
 * ibus_lookup_table_page_up:
 * @table: An IBusLookupTable.
 *
 * Go to previous page of an #IBusLookupTable.
 *
 * It returns FALSE if it is already at the first page,
 * unless  <code>table&gt;-round==TRUE</code>, where it will go
 * to the last page.
 *
 * Returns: %TRUE if succeed.
 */
gboolean             ibus_lookup_table_page_up  (IBusLookupTable    *table);

/**
 * ibus_lookup_table_page_down:
 * @table: An IBusLookupTable.
 *
 * Go to next page of an #IBusLookupTable.
 *
 * It returns FALSE if it is already at the last page,
 * unless  <code>table&gt;-round==TRUE</code>, where it will go
 * to the first page.
 *
 * Returns: %TRUE if succeed.
 */
gboolean             ibus_lookup_table_page_down(IBusLookupTable    *table);

/**
 * ibus_lookup_table_cursor_up:
 * @table: An IBusLookupTable.
 *
 * Go to previous candidate of an #IBusLookupTable.
 *
 * It returns FALSE if it is already at the first candidate,
 * unless  <code>table&gt;-round==TRUE</code>, where it will go
 * to the last candidate.
 *
 * Returns: %TRUE if succeed.
 */
gboolean             ibus_lookup_table_cursor_up(IBusLookupTable    *table);

/**
 * ibus_lookup_table_cursor_down:
 * @table: An IBusLookupTable.
 *
 * Go to next candidate of an #IBusLookupTable.
 *
 * It returns FALSE if it is already at the last candidate,
 * unless  <code>table&gt;-round==TRUE</code>, where it will go
 * to the first candidate.
 *
 * Returns: %TRUE if succeed.
 */
gboolean             ibus_lookup_table_cursor_down
                                                (IBusLookupTable    *table);
G_END_DECLS
#endif

