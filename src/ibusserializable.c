/* -*- mode: C; c-basic-offset: 4; indent-tabs-mode: nil; -*- */
/* vim:set et sts=4: */
/* ibus - The Input Bus
 * Copyright (C) 2008-2010 Peng Huang <shawn.p.huang@gmail.com>
 * Copyright (C) 2018 Takao Fujiwara <takao.fujiwara1@gmail.com>
 * Copyright (C) 2008-2018 Red Hat, Inc.
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
#include "ibusinternal.h"
#include "ibusserializable.h"

#define IBUS_SERIALIZABLE_GET_PRIVATE(o)  \
   (G_TYPE_INSTANCE_GET_PRIVATE ((o), IBUS_TYPE_SERIALIZABLE, IBusSerializablePrivate))

enum {
    LAST_SIGNAL,
};

struct _IBusSerializablePrivate {
    GData   *attachments;
};

// static guint    object_signals[LAST_SIGNAL] = { 0 };

/* functions prototype */
static void      ibus_serializable_base_init        (IBusSerializableClass  *class);
static void      ibus_serializable_base_fini        (IBusSerializableClass  *class);
static void      ibus_serializable_class_init       (IBusSerializableClass  *class);
static void      ibus_serializable_init             (IBusSerializable       *object);
static void      ibus_serializable_destroy          (IBusSerializable       *object);
static gboolean  ibus_serializable_real_serialize   (IBusSerializable       *object,
                                                     GVariantBuilder        *builder);
static gint      ibus_serializable_real_deserialize (IBusSerializable       *object,
                                                     GVariant               *variant);
static gboolean  ibus_serializable_real_copy        (IBusSerializable       *dest,
                                                     const IBusSerializable *src);

static IBusObjectClass *parent_class = NULL;

GType
ibus_serializable_get_type (void)
{
    static GType type = 0;

    static const GTypeInfo type_info = {
        sizeof (IBusSerializableClass),
        (GBaseInitFunc)     ibus_serializable_base_init,
        (GBaseFinalizeFunc) ibus_serializable_base_fini,
        (GClassInitFunc)    ibus_serializable_class_init,
        NULL,               /* class finalize */
        NULL,               /* class data */
        sizeof (IBusSerializable),
        0,
        (GInstanceInitFunc) ibus_serializable_init,
    };

    if (type == 0) {
        type = g_type_register_static (IBUS_TYPE_OBJECT,
                                       "IBusSerializable",
                                       &type_info,
                                       0);
    }

    return type;
}

IBusSerializable *
ibus_serializable_new (void)
{
    return IBUS_SERIALIZABLE (g_object_new (IBUS_TYPE_SERIALIZABLE, NULL));
}

static void
ibus_serializable_base_init     (IBusSerializableClass *class)
{
}

static void
ibus_serializable_base_fini     (IBusSerializableClass *class)
{
}

static void
ibus_serializable_class_init     (IBusSerializableClass *class)
{
    IBusObjectClass *object_class = IBUS_OBJECT_CLASS (class);

    parent_class = (IBusObjectClass *) g_type_class_peek_parent (class);

    g_type_class_add_private (class, sizeof (IBusSerializablePrivate));

    object_class->destroy = (IBusObjectDestroyFunc) ibus_serializable_destroy;

    class->serialize = ibus_serializable_real_serialize;
    class->deserialize = ibus_serializable_real_deserialize;
    class->copy = ibus_serializable_real_copy;
}

static void
ibus_serializable_init (IBusSerializable *serializable)
{
    serializable->priv = IBUS_SERIALIZABLE_GET_PRIVATE (serializable);
    serializable->priv->attachments = NULL;
    g_datalist_init (&serializable->priv->attachments);
}

static void
ibus_serializable_destroy (IBusSerializable *serializable)
{
    g_datalist_clear (&serializable->priv->attachments);
    parent_class->destroy (IBUS_OBJECT (serializable));
}

static void
_serialize_cb (GQuark           key,
               GVariant        *value,
               GVariantBuilder *array)
{
    g_variant_builder_add (array, "{sv}",
                g_quark_to_string (key), g_variant_new_variant (value));
}

static gboolean
ibus_serializable_real_serialize (IBusSerializable *serializable,
                                  GVariantBuilder  *builder)
{
    GVariantBuilder array;
    g_variant_builder_init (&array, G_VARIANT_TYPE ("a{sv}"));

    g_datalist_foreach (&serializable->priv->attachments,
                        (GDataForeachFunc) _serialize_cb,
                        &array);
    g_variant_builder_add (builder, "a{sv}", &array);
    return TRUE;
}

static gint
ibus_serializable_real_deserialize (IBusSerializable *object,
                                    GVariant         *variant)
{
    const gchar *key;
    GVariant *value;
    GVariantIter *iter = NULL;
    g_variant_get_child (variant, 1, "a{sv}", &iter);
    while (g_variant_iter_loop (iter, "{&sv}", &key, &value)) {
        GVariant *attachment = g_variant_get_variant (value);
        ibus_serializable_set_attachment (object,
                                          key,
                                          attachment);
        g_variant_unref (attachment);
    }
    g_variant_iter_free (iter);
    return 2;
}

static void
_copy_cb (GQuark     key,
          GVariant  *value,
          GData    **datalist)
{
    g_datalist_id_set_data_full (datalist,
                                 key,
                                 g_variant_ref (value),
                                 (GDestroyNotify) g_variant_unref);
}

static gboolean
ibus_serializable_real_copy (IBusSerializable *dest,
                             const IBusSerializable *src)
{
    IBusSerializablePrivate *src_priv;
    IBusSerializablePrivate *dest_priv;
    src_priv = IBUS_SERIALIZABLE_GET_PRIVATE (src);
    dest_priv = IBUS_SERIALIZABLE_GET_PRIVATE (dest);

    g_datalist_foreach (&src_priv->attachments,
                        (GDataForeachFunc) _copy_cb,
                        &dest_priv->attachments);
    return TRUE;
}

void
ibus_serializable_set_qattachment (IBusSerializable *serializable,
                                   GQuark            key,
                                   GVariant         *value)
{
    g_return_if_fail (IBUS_IS_SERIALIZABLE (serializable));
    g_return_if_fail (key != 0);

    g_datalist_id_set_data_full (&serializable->priv->attachments,
                                 key,
                                 value ? g_variant_ref_sink (value) : NULL,
                                 (GDestroyNotify) g_variant_unref);
}

GVariant *
ibus_serializable_get_qattachment (IBusSerializable *serializable,
                                   GQuark            key)
{

    g_return_val_if_fail (IBUS_IS_SERIALIZABLE (serializable), NULL);
    g_return_val_if_fail (key != 0, NULL);

    return (GVariant *) g_datalist_id_get_data (
            &serializable->priv->attachments, key);
}

void
ibus_serializable_remove_qattachment (IBusSerializable *serializable,
                                      GQuark            key)
{

    g_return_if_fail (IBUS_IS_SERIALIZABLE (serializable));
    g_return_if_fail (key != 0);

    g_datalist_id_set_data (&serializable->priv->attachments, key, NULL);
}

IBusSerializable *
ibus_serializable_copy (IBusSerializable *object)
{
    g_return_val_if_fail (IBUS_IS_SERIALIZABLE (object), NULL);

    GType type;
    IBusSerializable *new_object;

    type = G_OBJECT_TYPE (object);

    new_object = g_object_new (type, NULL);
    g_return_val_if_fail (new_object != NULL, NULL);

    if (IBUS_SERIALIZABLE_GET_CLASS (new_object)->copy (new_object, object)) {
        return new_object;
    }

    g_object_unref (new_object);
    g_return_val_if_reached (NULL);
}

GVariant *
ibus_serializable_serialize_object (IBusSerializable *object)
{
    g_return_val_if_fail (IBUS_IS_SERIALIZABLE (object), FALSE);
    gboolean retval;

    GVariantBuilder builder;
    g_variant_builder_init (&builder, G_VARIANT_TYPE_TUPLE);

    g_variant_builder_add (&builder, "s", g_type_name (G_OBJECT_TYPE (object)));
    retval = IBUS_SERIALIZABLE_GET_CLASS (object)->serialize (object, &builder);
    g_assert (retval);

    return g_variant_builder_end (&builder);
}

IBusSerializable *
ibus_serializable_deserialize_object (GVariant *variant)
{
    g_return_val_if_fail (variant != NULL, NULL);

    GVariant *var = NULL;
    switch (g_variant_classify (variant)) {
    case G_VARIANT_CLASS_VARIANT:
        var = g_variant_get_variant (variant);
        break;
    case G_VARIANT_CLASS_TUPLE:
        var = g_variant_ref (variant);
        break;
    default:
        g_return_val_if_reached (NULL);
    }

    gchar *type_name = NULL;
    g_variant_get_child (var, 0, "&s", &type_name);
    GType type = g_type_from_name (type_name);

    g_return_val_if_fail (g_type_is_a (type, IBUS_TYPE_SERIALIZABLE), NULL);

    IBusSerializable *object = g_object_new (type, NULL);

    gint retval = IBUS_SERIALIZABLE_GET_CLASS (object)->deserialize (object, var);
    g_variant_unref (var);
    if (retval)
        return object;

    g_object_unref (object);
    g_return_val_if_reached (NULL);
}

