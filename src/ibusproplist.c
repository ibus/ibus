/* -*- mode: C; c-basic-offset: 4; indent-tabs-mode: nil; -*- */
/* vim:set et sts=4: */
/* IBus - The Input Bus
 * Copyright (C) 2008-2010 Peng Huang <shawn.p.huang@gmail.com>
 * Copyright (C) 2008-2010 Red Hat, Inc.
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
#include "ibusproplist.h"

/* functions prototype */
static void         ibus_prop_list_destroy      (IBusPropList       *prop_list);
static gboolean     ibus_prop_list_serialize    (IBusPropList       *prop_list,
                                                 GVariantBuilder    *builder);
static gint         ibus_prop_list_deserialize  (IBusPropList       *prop_list,
                                                 GVariant           *variant);
static gboolean     ibus_prop_list_copy         (IBusPropList       *dest,
                                                 const IBusPropList *src);

G_DEFINE_TYPE (IBusPropList, ibus_prop_list, IBUS_TYPE_SERIALIZABLE)

static void
ibus_prop_list_class_init (IBusPropListClass *class)
{
    IBusObjectClass *object_class = IBUS_OBJECT_CLASS (class);
    IBusSerializableClass *serializable_class = IBUS_SERIALIZABLE_CLASS (class);

    object_class->destroy = (IBusObjectDestroyFunc) ibus_prop_list_destroy;

    serializable_class->serialize   = (IBusSerializableSerializeFunc) ibus_prop_list_serialize;
    serializable_class->deserialize = (IBusSerializableDeserializeFunc) ibus_prop_list_deserialize;
    serializable_class->copy        = (IBusSerializableCopyFunc) ibus_prop_list_copy;
}

static void
ibus_prop_list_init (IBusPropList *prop_list)
{
    prop_list->properties = g_array_new (TRUE, TRUE, sizeof (IBusProperty *));
}

static void
ibus_prop_list_destroy (IBusPropList *prop_list)
{
    IBusProperty **p;
    gint i;

    p = (IBusProperty **) g_array_free (prop_list->properties, FALSE);

    for (i = 0; p[i] != NULL; i++) {
        g_object_unref (p[i]);
    }
    g_free (p);

    IBUS_OBJECT_CLASS (ibus_prop_list_parent_class)->destroy ((IBusObject *) prop_list);
}

static gboolean
ibus_prop_list_serialize (IBusPropList    *prop_list,
                          GVariantBuilder *builder)
{
    gboolean retval;
    guint i;

    retval = IBUS_SERIALIZABLE_CLASS (ibus_prop_list_parent_class)->serialize ((IBusSerializable *) prop_list, builder);
    g_return_val_if_fail (retval, FALSE);

    GVariantBuilder array;
    g_variant_builder_init (&array, G_VARIANT_TYPE ("av"));
    for (i = 0;; i++) {
        IBusProperty *prop = ibus_prop_list_get (prop_list, i);
        if (prop == NULL)
            break;
        g_variant_builder_open (&array, G_VARIANT_TYPE_VARIANT);
        g_variant_builder_add_value (
                &array,
                ibus_serializable_serialize ((IBusSerializable *)prop));
        g_variant_builder_close (&array);
    }

    g_variant_builder_add (builder, "av", &array);

    return TRUE;
}

gint
ibus_prop_list_deserialize (IBusPropList    *prop_list,
                            GVariant        *variant)
{
    gint retval;

    retval = IBUS_SERIALIZABLE_CLASS (ibus_prop_list_parent_class)->deserialize ((IBusSerializable *) prop_list, variant);
    g_return_val_if_fail (retval, 0);

    g_return_val_if_fail (IBUS_IS_PROP_LIST (prop_list), 0);

    GVariantIter *iter = NULL;
    g_variant_get_child (variant, retval++, "av", &iter);
    g_return_val_if_fail (iter != NULL, retval);
    GVariant *var;
    while (g_variant_iter_loop (iter, "v", &var)) {
        IBusProperty *prop = IBUS_PROPERTY (ibus_serializable_deserialize (var));
        ibus_prop_list_append (prop_list, prop);
    }
    g_variant_iter_free (iter);

    return retval;
}



static gboolean
ibus_prop_list_copy (IBusPropList       *dest,
                     const IBusPropList *src)
{
    gboolean retval;
    IBusProperty *prop;
    guint i;

    retval = IBUS_SERIALIZABLE_CLASS (ibus_prop_list_parent_class)->copy ((IBusSerializable *) dest,
                                 (const IBusSerializable *) src);
    g_return_val_if_fail (retval, FALSE);

    g_return_val_if_fail (IBUS_IS_PROP_LIST (dest), FALSE);
    g_return_val_if_fail (IBUS_IS_PROP_LIST (src), FALSE);

    i = 0;
    while ((prop = ibus_prop_list_get ((IBusPropList *)src, i)) != NULL) {
        prop = (IBusProperty *) ibus_serializable_copy ((IBusSerializable *) prop);
        ibus_prop_list_append (dest, prop);
        i ++;
    }
    return TRUE;
}


IBusPropList *
ibus_prop_list_new ()
{
    IBusPropList *prop_list;

    prop_list = g_object_new (IBUS_TYPE_PROP_LIST, NULL);

    return prop_list;
}

void
ibus_prop_list_append (IBusPropList *prop_list,
                       IBusProperty *prop)
{
    g_assert (IBUS_IS_PROP_LIST (prop_list));
    g_assert (IBUS_IS_PROPERTY (prop));

    g_object_ref_sink (prop);

    g_array_append_val (prop_list->properties, prop);
}

IBusProperty *
ibus_prop_list_get (IBusPropList *prop_list,
                    guint         index)
{
    g_assert (IBUS_IS_PROP_LIST (prop_list));


    if (index >= prop_list->properties->len)
        return NULL;

    return g_array_index (prop_list->properties, IBusProperty *, index);
}



gboolean
ibus_prop_list_update_property (IBusPropList *prop_list,
                                IBusProperty *prop_update)
{
    g_assert (IBUS_IS_PROP_LIST (prop_list));
    g_assert (IBUS_IS_PROPERTY (prop_update));

    gint i;

    for (i = 0; i < prop_list->properties->len; i ++) {
        IBusProperty *prop = g_array_index (prop_list->properties, IBusProperty *, i);
        if (ibus_property_update (prop, prop_update)) {
            return TRUE;
        }
    }

    return FALSE;
}
