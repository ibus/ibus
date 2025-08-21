/* -*- mode: C; c-basic-offset: 4; indent-tabs-mode: nil; -*- */
/* vim:set et sts=4: */
/* IBus - The Input Bus
 * Copyright (C) 2008-2010 Peng Huang <shawn.p.huang@gmail.com>
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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "ibusattribute.h"

/* functions prototype */
// static void         ibus_attribute_destroy      (IBusAttribute          *attr);
static gboolean     ibus_attribute_serialize    (IBusAttribute          *attr,
                                                 GVariantBuilder        *builder);
static gint         ibus_attribute_deserialize  (IBusAttribute          *attr,
                                                 GVariant               *variant);
static gboolean     ibus_attribute_copy         (IBusAttribute          *dest,
                                                 const IBusAttribute    *src);

G_DEFINE_TYPE (IBusAttribute, ibus_attribute, IBUS_TYPE_SERIALIZABLE)

static void
ibus_attribute_class_init (IBusAttributeClass *class)
{
    IBusSerializableClass *serializable_class = IBUS_SERIALIZABLE_CLASS (class);

    serializable_class->serialize   = (IBusSerializableSerializeFunc) ibus_attribute_serialize;
    serializable_class->deserialize = (IBusSerializableDeserializeFunc) ibus_attribute_deserialize;
    serializable_class->copy        = (IBusSerializableCopyFunc) ibus_attribute_copy;
}

static void
ibus_attribute_init (IBusAttribute *attr)
{
}

static gboolean
ibus_attribute_serialize (IBusAttribute   *attr,
                          GVariantBuilder *builder)
{
    gboolean retval;

    retval = IBUS_SERIALIZABLE_CLASS (ibus_attribute_parent_class)->serialize ((IBusSerializable *) attr, builder);
    g_return_val_if_fail (retval, FALSE);

    g_variant_builder_add (builder, "u", attr->type);
    g_variant_builder_add (builder, "u", attr->value);
    g_variant_builder_add (builder, "u", attr->start_index);
    g_variant_builder_add (builder, "u", attr->end_index);

    return TRUE;
}

static gint
ibus_attribute_deserialize (IBusAttribute *attr,
                            GVariant      *variant)
{
    gint retval;

    retval = IBUS_SERIALIZABLE_CLASS (ibus_attribute_parent_class)->deserialize ((IBusSerializable *) attr, variant);
    g_return_val_if_fail (retval, 0);

    g_variant_get_child (variant, retval++, "u", &attr->type);
    g_variant_get_child (variant, retval++, "u", &attr->value);
    g_variant_get_child (variant, retval++, "u", &attr->start_index);
    g_variant_get_child (variant, retval++, "u", &attr->end_index);

    return retval;

}

static gboolean
ibus_attribute_copy (IBusAttribute       *dest,
                     const IBusAttribute *src)
{
    gboolean retval;

    retval = IBUS_SERIALIZABLE_CLASS (ibus_attribute_parent_class)->copy ((IBusSerializable *)dest,
                                 (IBusSerializable *)src);
    g_return_val_if_fail (retval, FALSE);

    g_return_val_if_fail (IBUS_IS_ATTRIBUTE (dest), FALSE);
    g_return_val_if_fail (IBUS_IS_ATTRIBUTE (src), FALSE);

    dest->type  = src->type;
    dest->value = src->value;
    dest->start_index = src->start_index;
    dest->end_index = src->end_index;

    return TRUE;
}

IBusAttribute *
ibus_attribute_new (guint type,
                    guint value,
                    guint start_index,
                    guint end_index)
{
    g_return_val_if_fail (
        type == IBUS_ATTR_TYPE_UNDERLINE  ||
        type == IBUS_ATTR_TYPE_FOREGROUND ||
        type == IBUS_ATTR_TYPE_BACKGROUND ||
        type == IBUS_ATTR_TYPE_HINT, NULL);

    IBusAttribute *attr = IBUS_ATTRIBUTE (g_object_new (IBUS_TYPE_ATTRIBUTE, NULL));

    attr->type = type;
    attr->value = value;
    attr->start_index = start_index;
    attr->end_index = end_index;

    return attr;
}

guint
ibus_attribute_get_attr_type (IBusAttribute *attr)
{
    return attr->type;
}

guint
ibus_attribute_get_value (IBusAttribute *attr)
{
    return attr->value;
}

guint
ibus_attribute_get_start_index (IBusAttribute *attr)
{
    return attr->start_index;
}

guint
ibus_attribute_get_end_index (IBusAttribute *attr)
{
    return attr->end_index;
}

IBusAttribute *
ibus_attr_underline_new (guint underline_type,
                         guint start_index,
                         guint end_index)
{
    g_return_val_if_fail (
        underline_type == IBUS_ATTR_UNDERLINE_NONE   ||
        underline_type == IBUS_ATTR_UNDERLINE_SINGLE ||
        underline_type == IBUS_ATTR_UNDERLINE_DOUBLE ||
        underline_type == IBUS_ATTR_UNDERLINE_LOW, NULL);

    return ibus_attribute_new (IBUS_ATTR_TYPE_UNDERLINE,
                               underline_type,
                               start_index,
                               end_index);
}

IBusAttribute *
ibus_attr_foreground_new (guint color,
                          guint start_index,
                          guint end_index)
{
    return ibus_attribute_new (IBUS_ATTR_TYPE_FOREGROUND,
                               color,
                               start_index,
                               end_index);
}

IBusAttribute *
ibus_attr_background_new (guint color,
                          guint start_index,
                          guint end_index)
{
    return ibus_attribute_new (IBUS_ATTR_TYPE_BACKGROUND,
                               color,
                               start_index,
                               end_index);
}

IBusAttribute *
ibus_attr_hint_new (guint hint,
                    guint start_index,
                    guint end_index)
{
    g_return_val_if_fail (hint != IBUS_ATTR_PREEDIT_NONE, NULL);
    return ibus_attribute_new (IBUS_ATTR_TYPE_HINT,
                               hint,
                               start_index,
                               end_index);
}
