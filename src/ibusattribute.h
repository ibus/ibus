/* -*- mode: C; c-basic-offset: 4; indent-tabs-mode: nil; -*- */
/* vim:set et sts=4: */
/* IBus - The Input Bus
 * Copyright (C) 2008-2013 Peng Huang <shawn.p.huang@gmail.com>
 * Copyright (C) 2011-2023 Takao Fujiwara <takao.fujiwara1@gmail.com>
 * Copyright (C) 2008-2023 Red Hat, Inc.
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

#ifndef __IBUS_ATTRIBUTE_H_
#define __IBUS_ATTRIBUTE_H_

/**
 * SECTION: ibusattribute
 * @short_description: Attributes of IBusText.
 * @see_also: #IBusText
 * @stability: Stable
 *
 * An IBusAttribute represents an attribute that associate to IBusText.
 * It decorates preedit buffer and auxiliary text with underline, foreground
 * and background colors.
 */

#include "ibusserializable.h"

G_BEGIN_DECLS

/*
 * Type macros.
 */
/* define IBusAttribute macros */
#define IBUS_TYPE_ATTRIBUTE             \
    (ibus_attribute_get_type ())
#define IBUS_ATTRIBUTE(obj)             \
    (G_TYPE_CHECK_INSTANCE_CAST ((obj), IBUS_TYPE_ATTRIBUTE, IBusAttribute))
#define IBUS_ATTRIBUTE_CLASS(klass)     \
    (G_TYPE_CHECK_CLASS_CAST ((klass), IBUS_TYPE_ATTRIBUTE, IBusAttributeClass))
#define IBUS_IS_ATTRIBUTE(obj)          \
    (G_TYPE_CHECK_INSTANCE_TYPE ((obj), IBUS_TYPE_ATTRIBUTE))
#define IBUS_IS_ATTRIBUTE_CLASS(klass)  \
    (G_TYPE_CHECK_CLASS_TYPE ((klass), IBUS_TYPE_ATTRIBUTE))
#define IBUS_ATTRIBUTE_GET_CLASS(obj)   \
    (G_TYPE_INSTANCE_GET_CLASS ((obj), IBUS_TYPE_ATTRIBUTE, IBusAttributeClass))

/**
 * IBusAttrType:
 * @IBUS_ATTR_TYPE_UNDERLINE: Decorate with underline.
 * @IBUS_ATTR_TYPE_FOREGROUND: Foreground color.
 * @IBUS_ATTR_TYPE_BACKGROUND: Background color.
 *
 * Type enumeration of IBusText attribute.
 */
typedef enum {
    IBUS_ATTR_TYPE_UNDERLINE    = 1,
    IBUS_ATTR_TYPE_FOREGROUND   = 2,
    IBUS_ATTR_TYPE_BACKGROUND   = 3,
} IBusAttrType;

/**
 * IBusAttrUnderline:
 * @IBUS_ATTR_UNDERLINE_NONE: No underline.
 * @IBUS_ATTR_UNDERLINE_SINGLE: Single underline.
 * @IBUS_ATTR_UNDERLINE_DOUBLE: Double underline.
 * @IBUS_ATTR_UNDERLINE_LOW: Low underline ? FIXME
 * @IBUS_ATTR_UNDERLINE_ERROR: Error underline
 *
 * Type of IBusText attribute.
 */
typedef enum {
    IBUS_ATTR_UNDERLINE_NONE    = 0,
    IBUS_ATTR_UNDERLINE_SINGLE  = 1,
    IBUS_ATTR_UNDERLINE_DOUBLE  = 2,
    IBUS_ATTR_UNDERLINE_LOW     = 3,
    IBUS_ATTR_UNDERLINE_ERROR   = 4,
} IBusAttrUnderline;


/**
 * IBusAttrPreedit:
 * @IBUS_ATTR_PREEDIT_DEFAULT: Default style for composing text.
 * @IBUS_ATTR_PREEDIT_NONE: Style should be the same as in non-composing text.
 * @IBUS_ATTR_PREEDIT_WHOLE: Most language engines wish to draw underline in
 *                           the typed whole preedit string except for the
 *                           prediction string. (Chinese, Japanese,
 *                           Typing-booster)
 * @IBUS_ATTR_PREEDIT_SELECTION: Modifying an active segment is distinguished
 *                               against whole the preedit text. (Hangul,
 *                               Japanese)
 * @IBUS_ATTR_PREEDIT_PREDICTION: A prediction string can be appended after the
 *                                typed string. (Typing-booster)
 * @IBUS_ATTR_PREEDIT_PREFIX: A prefix string can be an informative color.
 *                            (Table)
 * @IBUS_ATTR_PREEDIT_SUFFIX: A suffix string can be an informative color.
 *                            (Table)
 * @IBUS_ATTR_PREEDIT_ERROR_SPELLING: An detected typo could be an error color
 *                                    with a spelling check or the word could
 *                                    not be found in a dictionary. The
 *                                    underline color also might be more
 *                                    visible. (Typing-booster, Table)
 * @IBUS_ATTR_PREEDIT_ERROR_COMPOSE: A wrong compose key could be an error
 *                                   color. (Typing-booster)
 *
 * Type of Pre-edit style as the semantic name.
 * The Wayland specs prefers to express the semantic values rather than RGB
 * values and text-input protocol version 1 defines some values:
 * https://gitlab.freedesktop.org/wayland/wayland-protocols/-/blob/main/unstable/text-input/text-input-unstable-v1.xml?ref_type=heads#L251
 *
 * IBus compiled the values for major input method engines:
 * https://github.com/ibus/ibus/wiki/Wayland-Colors
 *
 * Since: 1.5.29
 * Stability: Unstable
 */
typedef enum {
    IBUS_ATTR_PREEDIT_DEFAULT = 0,
    IBUS_ATTR_PREEDIT_NONE,
    IBUS_ATTR_PREEDIT_WHOLE,
    IBUS_ATTR_PREEDIT_SELECTION,
    IBUS_ATTR_PREEDIT_PREDICTION,
    IBUS_ATTR_PREEDIT_PREFIX,
    IBUS_ATTR_PREEDIT_SUFFIX,
    IBUS_ATTR_PREEDIT_ERROR_SPELLING,
    IBUS_ATTR_PREEDIT_ERROR_COMPOSE,
} IBusAttrPreedit;

typedef struct _IBusAttribute IBusAttribute;
typedef struct _IBusAttributeClass IBusAttributeClass;

/**
 * IBusAttribute:
 * @type: IBusAttributeType
 * @value: Value for the type.
 * @start_index: The starting index, inclusive.
 * @end_index: The ending index, exclusive.
 *
 * Signify the type, value and scope of the attribute.
 * The scope starts from @start_index till the @end_index-1.
 */
struct _IBusAttribute {
    IBusSerializable parent;

    /*< public >*/
    guint type;
    guint value;
    guint start_index;
    guint end_index;
};

struct _IBusAttributeClass {
    IBusSerializableClass parent;
};

/**
 * ibus_attribute_get_type:
 * @returns: GType of IBusAttribute.
 *
 * Returns GType of IBusAttribute.
 */
GType                ibus_attribute_get_type    ();

/**
 * ibus_attribute_new:
 * @type: Type of the attribute.
 * @value: Value of the attribute.
 * @start_index: Where attribute starts.
 * @end_index: Where attribute ends.
 *
 * Creates a new IBusAttribute.
 *
 * Returns: (transfer none): A newly allocated IBusAttribute.
 */
IBusAttribute       *ibus_attribute_new         (guint           type,
                                                 guint           value,
                                                 guint           start_index,
                                                 guint           end_index);

/**
 * ibus_attribute_get_attr_type:
 * @attr: An #IBusAttribute
 *
 * Gets an enum of #IBusAttrType.
 *
 * Returns: An enum of #IBusAttrType.
 */
guint                ibus_attribute_get_attr_type
                                                (IBusAttribute *attr);

/**
 * ibus_attribute_get_value:
 * @attr: An #IBusAttribute
 *
 * Gets an unsigned int value relative with #IBusAttrType.
 * If the type is %IBUS_ATTR_TYPE_UNDERLINE, the return value is
 * #IBusAttrUnderline. If the type is %IBUS_ATTR_TYPE_FOREGROUND,
 * the return value is the color RGB.
 *
 * Returns: An unsigned int value relative with #IBusAttrType.
 */
guint                ibus_attribute_get_value   (IBusAttribute *attr);

/**
 * ibus_attribute_get_start_index:
 * @attr: An #IBusAttribute
 *
 * Gets a start unsigned index
 *
 * Returns: A start unsigned index
 */
guint                ibus_attribute_get_start_index
                                                (IBusAttribute *attr);

/**
 * ibus_attribute_get_end_index:
 * @attr: An #IBusAttribute
 *
 * Gets an end unsigned index
 *
 * Returns: A end unsigned index
 */
guint                ibus_attribute_get_end_index
                                                (IBusAttribute *attr);

/**
 * ibus_attr_underline_new:
 * @underline_type: Type of underline.
 * @start_index: Where attribute starts.
 * @end_index: Where attribute ends.
 *
 * Creates a new underline #IBusAttribute.
 *
 * Returns: (transfer none): A newly allocated #IBusAttribute.
 */
IBusAttribute       *ibus_attr_underline_new    (guint           underline_type,
                                                 guint           start_index,
                                                 guint           end_index);
/**
 * ibus_attr_foreground_new:
 * @color: Color in RGB.
 * @start_index: Where attribute starts.
 * @end_index: Where attribute ends.
 *
 * Creates a new foreground #IBusAttribute.
 *
 * Returns: (transfer none): A newly allocated #IBusAttribute.
 */
IBusAttribute       *ibus_attr_foreground_new   (guint           color,
                                                 guint           start_index,
                                                 guint           end_index);
/**
 * ibus_attr_background_new:
 * @color: Color in RGB.
 * @start_index: Where attribute starts.
 * @end_index: Where attribute ends.
 *
 * Creates a new background #IBusAttribute.
 *
 * Returns: (transfer none): A newly allocated #IBusAttribute.
 */
IBusAttribute       *ibus_attr_background_new   (guint           color,
                                                 guint           start_index,
                                                 guint           end_index);

G_END_DECLS
#endif

