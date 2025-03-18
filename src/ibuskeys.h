/* -*- mode: C; c-basic-offset: 4; indent-tabs-mode: nil; -*- */
/* vim:set et sts=4: */
/* ibus - The Input Bus
 * Copyright (C) 2011 Peng Huang <shawn.p.huang@gmail.com>
 * Copyright (C) 2011 Google, Inc.
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

#ifndef __IBUS_KEYS_H_
#define __IBUS_KEYS_H_

#include <glib.h>

G_BEGIN_DECLS
/**
 * ibus_keyval_name:
 * @keyval: Key symbol.
 *
 * Return the name of a key symbol.
 *
 * Note that the returned string is used internally, so don't free it.
 *
 * Returns: Corresponding key name. %NULL if no such key symbol.
 */
const gchar     *ibus_keyval_name       (guint           keyval);

/**
 * ibus_keyval_from_name:
 * @keyval_name: Key name in #gdk_keys_by_name.
 *
 * Return the key symbol that associate with the key name.
 *
 * Returns: Corresponding key symbol.
 */
guint            ibus_keyval_from_name  (const gchar    *keyval_name);

/**
 * ibus_unicode_to_keyval:
 * @wc: a ISO10646 encoded character
 * 
 * Convert from a ISO10646 character to a key symbol.
 * 
 * Returns: the corresponding IBus key symbol, if one exists.
 *          or, if there is no corresponding symbol,
 *          `wc | 0x01000000`
 **/
guint            ibus_unicode_to_keyval (gunichar        wc);

/**
 * ibus_keyval_to_unicode:
 * @keyval: an IBus key symbol
 * 
 * Convert from an IBus key symbol to the corresponding ISO10646 (Unicode)
 * character.
 * 
 * Returns: the corresponding unicode character, or 0 if there
 *          is no corresponding character.
 **/
gunichar         ibus_keyval_to_unicode (guint           keyval);

/**
 * ibus_keyval_to_upper:
 * @keyval: a key value.
 *
 * Converts a key value to upper case, if applicable.
 *
 * Returns: the upper case form of @keyval, or @keyval itself if it is already
 *   in upper case or it is not subject to case conversion.
 */
guint            ibus_keyval_to_upper (guint keyval);

/**
 * ibus_keyval_to_lower:
 * @keyval: a key value.
 *
 * Converts a key value to lower case, if applicable.
 *
 * Returns: the lower case form of @keyval, or @keyval itself if it is already
 *  in lower case or it is not subject to case conversion.
 */
guint            ibus_keyval_to_lower (guint keyval);

/**
 * ibus_keyval_convert_case:
 * @symbol: a keyval
 * @lower: (out): return location for lowercase version of @symbol
 * @upper: (out): return location for uppercase version of @symbol
 *
 * Obtains the upper- and lower-case versions of the keyval @symbol.
 * Examples of keyvals are #IBUS_KEY_a, #IBUS_KEY_Return, #IBUS_KEY_F1, etc.
 */
void ibus_keyval_convert_case (guint symbol, guint *lower, guint *upper);

G_END_DECLS
#endif // __IBUS_KEYS_H_
