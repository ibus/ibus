/* -*- mode: C; c-basic-offset: 4; indent-tabs-mode: nil; -*- */
/* vim:set et sts=4: */
/* IBus - The Input Bus
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

#ifndef __IBUS_TEXT_H_
#define __IBUS_TEXT_H_

/**
 * SECTION: ibustext
 * @short_description: Text with decorating information.
 *
 * An IBusText is the main text object in IBus.
 * The text is decorated according to associated IBusAttribute,
 * e.g. the foreground/background color, underline, and
 * applied scope.
 *
 * see_also: #IBusAttribute
 */

#include "ibusserializable.h"
#include "ibusattrlist.h"

/*
 * Type macros.
 */
/* define IBusText macros */
#define IBUS_TYPE_TEXT             \
    (ibus_text_get_type ())
#define IBUS_TEXT(obj)             \
    (G_TYPE_CHECK_INSTANCE_CAST ((obj), IBUS_TYPE_TEXT, IBusText))
#define IBUS_TEXT_CLASS(klass)     \
    (G_TYPE_CHECK_CLASS_CAST ((klass), IBUS_TYPE_TEXT, IBusTextClass))
#define IBUS_IS_TEXT(obj)          \
    (G_TYPE_CHECK_INSTANCE_TYPE ((obj), IBUS_TYPE_TEXT))
#define IBUS_IS_TEXT_CLASS(klass)  \
    (G_TYPE_CHECK_CLASS_TYPE ((klass), IBUS_TYPE_TEXT))
#define IBUS_TEXT_GET_CLASS(obj)   \
    (G_TYPE_INSTANCE_GET_CLASS ((obj), IBUS_TYPE_TEXT, IBusTextClass))

G_BEGIN_DECLS

typedef struct _IBusText IBusText;
typedef struct _IBusTextClass IBusTextClass;

/**
 * IBusText:
 * @is_static: Whether @text is static, i.e., no need and will not be freed. Only TRUE if IBusText is newed from ibus_text_new_from_static_string().
 * @text: The string content of IBusText in UTF-8.
 * @attrs: Associated IBusAttributes.
 *
 * A text object in IBus.
 */
struct _IBusText {
    IBusSerializable parent;

    /* members */
    /*< public >*/
    gboolean is_static;
    gchar  *text;
    IBusAttrList *attrs;
};

struct _IBusTextClass {
    IBusSerializableClass parent;
};

GType            ibus_text_get_type                 (void);

/**
 * ibus_text_new_from_string:
 * @str: An text string to be set.
 *
 * Creates a new #IBusText from a string.
 * @str will be duplicated in #IBusText, so feel free to free @str after this
 * function.
 *
 * Returns: A newly allocated #IBusText.
 */
IBusText        *ibus_text_new_from_string          (const gchar    *str);

/**
 * ibus_text_new_from_ucs4:
 * @str: An text string to be set.
 *
 * Creates a new #IBusText from an UCS-4 encoded string.
 * @str will be duplicated in IBusText, so feel free to free @str after this
 * function.
 *
 * Returns: A newly allocated #IBusText.
 */
IBusText        *ibus_text_new_from_ucs4            (const gunichar *str);

/**
 * ibus_text_new_from_static_string: (skip)
 * @str: An text string to be set.
 *
 * Creates a new #IBusText from a static string.
 *
 * Since @str is a static string which won't be freed.
 * This function will NOT duplicate @str.
 *
 * Returns: A newly allocated #IBusText.
 */
IBusText        *ibus_text_new_from_static_string   (const gchar    *str);

/**
 * ibus_text_new_from_printf:
 * @fmt: printf format string.
 * @...: arguments for @fmt.
 *
 * Creates a new #IBusText from a printf expression.
 *
 * The result of printf expression is stored in the new IBusText instance.
 *
 * Returns: A newly allocated #IBusText.
 */
IBusText        *ibus_text_new_from_printf          (const gchar    *fmt,
                                                     ...) G_GNUC_PRINTF (1, 2);

/**
 * ibus_text_new_from_unichar:
 * @c: A single UCS4-encoded character.
 *
 * Creates a new #IBusText from a single UCS4-encoded character.
 *
 * Returns: A newly allocated #IBusText.
 */
IBusText        *ibus_text_new_from_unichar         (gunichar        c);

/**
 * ibus_text_append_attribute:
 * @text: an IBusText
 * @type: IBusAttributeType for @text.
 * @value: Value for the type.
 * @start_index: The starting index, inclusive.
 * @end_index: The ending index, exclusive.
 *
 * Append an IBusAttribute for IBusText.
 */
void             ibus_text_append_attribute         (IBusText       *text,
                                                     guint           type,
                                                     guint           value,
                                                     guint           start_index,
                                                     gint            end_index);
/**
 * ibus_text_get_length:
 * @text: An #IBusText.
 *
 * Return number of characters in an #IBusText.
 * This function is based on g_utf8_strlen(), so unlike strlen(),
 * it does not count by bytes but characters instead.
 *
 * Returns: Number of character in @text, not counted by bytes.
 */
guint            ibus_text_get_length               (IBusText       *text);

/**
 * ibus_text_get_is_static: (skip)
 * @text: An #IBusText.
 *
 * Return the is_static in an #IBusText.
 *
 * Returns: the is_static in @text.
 */
gboolean         ibus_text_get_is_static            (IBusText       *text);

/**
 * ibus_text_get_text:
 * @text: An #IBusText.
 *
 * Return the text in an #IBusText. Should not be freed.
 *
 * Returns: the text in @text.
 */
const gchar *    ibus_text_get_text                 (IBusText       *text);

/**
 * ibus_text_get_attributes:
 * @text: An #IBusText.
 *
 * Return the attributes in an #IBusText. Should not be freed.
 *
 * Returns: (transfer none): the attrs in @text.
 */
IBusAttrList *   ibus_text_get_attributes           (IBusText       *text);

/**
 * ibus_text_set_attributes:
 * @text: An IBusText.
 * @attrs: (transfer full): An IBusAttrList
 */
void             ibus_text_set_attributes           (IBusText       *text,
                                                     IBusAttrList   *attrs);


G_END_DECLS
#endif

