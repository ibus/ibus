/* -*- mode: C; c-basic-offset: 4; indent-tabs-mode: nil; -*- */
/* vim:set et sts=4: */
/* bus - The Input Bus
 * Copyright (C) 2025 Takao Fujiwara <takao.fujiwara1@gmail.com>
 * Copyright (C) 2025 Red Hat, Inc.
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
#include "ibusmessage.h"
#include "ibusinternal.h"

enum {
    LAST_SIGNAL,
};

enum {
    PROP_0 = 0,
    PROP_DOMAIN,
    PROP_CODE,
    PROP_TITLE,
    PROP_DESCRIPTION,
    PROP_TIMEOUT,
    PROP_PROGRESS,
    PROP_SERIAL
};


/* IBusMessagePriv */
struct _IBusMessagePrivate {
    guchar      domain;
    guchar      code;
    gchar      *title;
    gchar      *description;
    int         timeout;
    int         progress;
    guint       serial;
};


#define ibus_message_get_const_instance_private(self) \
    G_STRUCT_MEMBER_P ((self), IBusMessage_private_offset)

#define IBUS_MESSAGE_GET_PRIVATE(o)  \
    ((IBusMessagePrivate *)ibus_message_get_instance_private (o))
#define IBUS_MESSAGE_GET_CONST_PRIVATE(o)  \
    ((const IBusMessagePrivate *)ibus_message_get_const_instance_private (o))

#define DEFAULT_TIMEOUT -1
#define DEFAULT_PROGRESS -1

// static guint            _signals[LAST_SIGNAL] = { 0 };

/* functions prototype */
static void     ibus_message_set_property      (IBusMessage            *msg,
                                                guint                  prop_id,
                                                const GValue           *value,
                                                GParamSpec             *pspec);
static void     ibus_message_get_property      (IBusMessage            *msg,
                                                guint                   prop_id,
                                                GValue                 *value,
                                                GParamSpec             *pspec);
static void     ibus_message_destroy           (IBusMessage            *msg);
static gboolean ibus_message_serialize         (IBusMessage            *msg,
                                                GVariantBuilder
                                                                      *builder);
static gint     ibus_message_deserialize       (IBusMessage            *msg,
                                                GVariant
                                                                      *variant);
static gboolean ibus_message_copy              (IBusMessage            *dest,
                                                const IBusMessage      *src);

G_DEFINE_TYPE_WITH_PRIVATE (IBusMessage,
                            ibus_message,
                            IBUS_TYPE_SERIALIZABLE)


static void
ibus_message_class_init (IBusMessageClass *class)
{
    GObjectClass *gobject_class = G_OBJECT_CLASS (class);
    IBusObjectClass *object_class = IBUS_OBJECT_CLASS (class);
    IBusSerializableClass *serializable_class = IBUS_SERIALIZABLE_CLASS (class);

    gobject_class->set_property =
            (GObjectSetPropertyFunc)ibus_message_set_property;
    gobject_class->get_property =
            (GObjectGetPropertyFunc)ibus_message_get_property;
    object_class->destroy = (IBusObjectDestroyFunc)ibus_message_destroy;

    serializable_class->serialize   =
            (IBusSerializableSerializeFunc)ibus_message_serialize;
    serializable_class->deserialize =
            (IBusSerializableDeserializeFunc)ibus_message_deserialize;
    serializable_class->copy        =
            (IBusSerializableCopyFunc)ibus_message_copy;

    /* install properties */
    /**
     * IBusMessage:domain:
     *
     * The domain of message
     */
    g_object_class_install_property (gobject_class,
                    PROP_DOMAIN,
                    g_param_spec_uchar ("domain",
                        "message domain",
                        "The domain of message",
                        0,
                        G_MAXUINT8,
                        0,
                        G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));

    /**
     * IBusMessage:code:
     *
     * The code of message
     */
    g_object_class_install_property (gobject_class,
                    PROP_CODE,
                    g_param_spec_uchar ("code",
                        "message code",
                        "The code of message",
                        0,
                        G_MAXUINT8,
                        0,
                        G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));

    /**
     * IBusMessage:title:
     *
     * The title of message
     */
    g_object_class_install_property (gobject_class,
                    PROP_TITLE,
                    g_param_spec_string ("title",
                        "message title",
                        "The title of message",
                        NULL,
                        G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));

    /**
     * IBusMessage:description:
     *
     * The description of message
     */
    g_object_class_install_property (gobject_class,
                    PROP_DESCRIPTION,
                    g_param_spec_string ("description",
                        "message description",
                        "The description of message",
                        "",
                        G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));

    /**
     * IBusMessage:timeout:
     *
     * The timeout of message
     */
    g_object_class_install_property (gobject_class,
                    PROP_TIMEOUT,
                    g_param_spec_int ("timeout",
                        "message timeout",
                        "The timeout of message",
                        G_MININT,
                        G_MAXINT,
                        DEFAULT_TIMEOUT,
                        G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));

    /**
     * IBusMessage:progress:
     *
     * The progress of message
     */
    g_object_class_install_property (gobject_class,
                    PROP_PROGRESS,
                    g_param_spec_int ("progress",
                        "message progress",
                        "The progress of message",
                        G_MININT,
                        G_MAXINT,
                        DEFAULT_PROGRESS,
                        G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));

    /**
     * IBusMessage:serial:
     *
     * The serial of message
     */
    g_object_class_install_property (gobject_class,
                    PROP_SERIAL,
                    g_param_spec_uint ("serial",
                        "message serial",
                        "The serial of message",
                        0,
                        G_MAXUINT,
                        0,
                        G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));
}


static void
ibus_message_init (IBusMessage *msg)
{
    IBusMessagePrivate *priv = IBUS_MESSAGE_GET_PRIVATE (msg);
    priv->timeout = DEFAULT_TIMEOUT;
    priv->progress = DEFAULT_PROGRESS;
}


static void
ibus_message_destroy (IBusMessage *msg)
{
    IBusMessagePrivate *priv = IBUS_MESSAGE_GET_PRIVATE (msg);
    g_free (priv->title);
    g_free (priv->description);

    IBUS_OBJECT_CLASS (ibus_message_parent_class)->destroy (IBUS_OBJECT (msg));
}


static void
ibus_message_set_property (IBusMessage  *msg,
                           guint         prop_id,
                           const GValue *value,
                           GParamSpec   *pspec)
{
    IBusMessagePrivate *priv;

    g_return_if_fail (IBUS_IS_MESSAGE (msg));
    priv = IBUS_MESSAGE_GET_PRIVATE (msg);
    switch (prop_id) {
    case PROP_DOMAIN:
        g_assert (priv->domain == 0);
        priv->domain = g_value_get_uchar (value);
        break;
    case PROP_CODE:
        g_assert (priv->code == 0);
        priv->code = g_value_get_uchar (value);
        break;
    case PROP_TITLE:
        g_assert (priv->title == NULL);
        priv->title = g_value_dup_string (value);
        break;
    case PROP_DESCRIPTION:
        g_assert (priv->description == NULL);
        priv->description  = g_value_dup_string (value);
        break;
    case PROP_TIMEOUT:
        g_assert (priv->timeout == DEFAULT_TIMEOUT);
        priv->timeout = g_value_get_int (value);
        break;
    case PROP_PROGRESS:
        g_assert (priv->progress == DEFAULT_PROGRESS);
        priv->progress = g_value_get_int (value);
        break;
    case PROP_SERIAL:
        g_assert (priv->serial == 0);
        priv->serial = g_value_get_uint (value);
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (msg, prop_id, pspec);
    }
}


static void
ibus_message_get_property (IBusMessage *msg,
                           guint        prop_id,
                           GValue      *value,
                           GParamSpec  *pspec)
{
    switch (prop_id) {
    case PROP_DOMAIN: {
            int domain = ibus_message_get_domain (msg);
            g_assert (domain > 0 && domain <= G_MAXUINT8);
            g_value_set_uchar (value, (guchar)domain);
        }
        break;
    case PROP_CODE: {
            int code = ibus_message_get_code (msg);
            g_assert (code >= 0 && code <= G_MAXUINT8);
            g_value_set_uchar (value, (guchar)code);
        }
        break;
    case PROP_TITLE:
        g_value_set_string (value, ibus_message_get_title (msg));
        break;
    case PROP_DESCRIPTION:
        g_value_set_string (value, ibus_message_get_description (msg));
        break;
    case PROP_TIMEOUT:
        g_value_set_int (value, ibus_message_get_timeout (msg));
        break;
    case PROP_PROGRESS:
        g_value_set_int (value, ibus_message_get_progress (msg));
        break;
    case PROP_SERIAL:
        g_value_set_uint (value, ibus_message_get_serial (msg));
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (msg, prop_id, pspec);
    }
}


static gboolean
ibus_message_serialize (IBusMessage     *msg,
                        GVariantBuilder *builder)
{
    gboolean retval;
    IBusMessagePrivate *priv;

    retval = IBUS_SERIALIZABLE_CLASS (ibus_message_parent_class)->
            serialize ((IBusSerializable *)msg, builder);
    g_return_val_if_fail (retval, FALSE);
    /* End dict iter */

    g_return_val_if_fail (IBUS_IS_MESSAGE (msg), FALSE);
    priv = IBUS_MESSAGE_GET_PRIVATE (msg);

#define NOTNULL(s) ((s) != NULL ? (s) : "")
    /* If you will add a new property, you can append it at the end and
     * you should not change the serialized order of domain, code,
     * title, .... */
    g_variant_builder_add (builder, "y", priv->domain);
    g_variant_builder_add (builder, "y", priv->code);
    g_variant_builder_add (builder, "s", NOTNULL (priv->title));
    g_variant_builder_add (builder, "s", NOTNULL (priv->description));
    g_variant_builder_add (builder, "i", priv->timeout);
    g_variant_builder_add (builder, "i", priv->progress);
    g_variant_builder_add (builder, "u", priv->serial);
    /* The serialized order should be kept. */
#undef NOTNULL

    return TRUE;
}


static gint
ibus_message_deserialize (IBusMessage *msg,
                          GVariant    *variant)
{
    gint retval;
    IBusMessagePrivate *priv;

    retval = IBUS_SERIALIZABLE_CLASS (ibus_message_parent_class)->
            deserialize ((IBusSerializable *)msg, variant);
    g_return_val_if_fail (retval, 0);

    g_return_val_if_fail (IBUS_IS_MESSAGE (msg), retval);
    priv = IBUS_MESSAGE_GET_PRIVATE (msg);

    /* If you will add a new property, you can append it at the end and
     * you should not change the serialized order of domain, code,
     * title, .... */
    g_variant_get_child (variant, retval++, "y", &priv->domain);
    g_variant_get_child (variant, retval++, "y", &priv->code);
    ibus_g_variant_get_child_string (variant, retval++,
                                     &priv->title);
    ibus_g_variant_get_child_string (variant, retval++,
                                     &priv->description);
    g_variant_get_child (variant, retval++, "i", &priv->timeout);
    g_variant_get_child (variant, retval++, "i", &priv->progress);
    g_variant_get_child (variant, retval++, "u", &priv->serial);
    /* The serialized order should be kept. */
    return retval;
}


static gboolean
ibus_message_copy (IBusMessage       *dest,
                   const IBusMessage *src)
{
    gboolean retval;
    IBusMessagePrivate *priv_dest;
    const IBusMessagePrivate *priv_src;

    retval = IBUS_SERIALIZABLE_CLASS (ibus_message_parent_class)->
            copy ((IBusSerializable *)dest, (IBusSerializable *)src);
    g_return_val_if_fail (retval, FALSE);

    g_return_val_if_fail (IBUS_IS_MESSAGE (dest), FALSE);
    g_return_val_if_fail (IBUS_IS_MESSAGE (src), FALSE);
    priv_dest  = IBUS_MESSAGE_GET_PRIVATE (dest);
    priv_src   = IBUS_MESSAGE_GET_CONST_PRIVATE (src);

    priv_dest->domain            = priv_src->domain;
    priv_dest->code              = priv_src->code;
    priv_dest->title             = g_strdup (priv_src->title);
    priv_dest->description       = g_strdup (priv_src->description);
    priv_dest->timeout           = priv_src->timeout;
    priv_dest->progress          = priv_src->progress;
    priv_dest->serial            = priv_src->serial;
    return TRUE;
}


guint
ibus_message_get_domain (IBusMessage *msg)
{
    IBusMessagePrivate *priv;
    g_return_val_if_fail (IBUS_IS_MESSAGE (msg), 0);
    priv = IBUS_MESSAGE_GET_PRIVATE (msg);
    return (guint)priv->domain;
}


guint
ibus_message_get_code (IBusMessage *msg)
{
    IBusMessagePrivate *priv;
    g_return_val_if_fail (IBUS_IS_MESSAGE (msg), 0);
    priv = IBUS_MESSAGE_GET_PRIVATE (msg);
    return (guint)priv->code;
}


#define IBUS_MESSAGE_GET_PROPERTY(property, return_type, defval)        \
return_type                                                             \
ibus_message_get_ ## property (IBusMessage *msg)                        \
{                                                                       \
    IBusMessagePrivate *priv;                                           \
    g_return_val_if_fail (IBUS_IS_MESSAGE (msg), (defval));             \
    priv = IBUS_MESSAGE_GET_PRIVATE (msg);                              \
    return priv->property;                                              \
}

IBUS_MESSAGE_GET_PROPERTY (title, const gchar *, NULL)
IBUS_MESSAGE_GET_PROPERTY (description, const gchar *, NULL)
IBUS_MESSAGE_GET_PROPERTY (timeout, int, DEFAULT_TIMEOUT)
IBUS_MESSAGE_GET_PROPERTY (progress, int, DEFAULT_PROGRESS)
IBUS_MESSAGE_GET_PROPERTY (serial, guint32, 0)
#undef IBUS_MESSAGE_GET_PROPERTY


IBusMessage *
ibus_message_new (guint domain,
                  guint code,
                  const gchar *title,
                  const gchar *description,
                  ...)
{
    va_list var_args;
    gchar *prop;
    gchar **names, **names_tmp;
    GValue *values, *values_tmp;
    guint n_properties = 4, i;
    IBusMessage *msg;
    G_GNUC_UNUSED IBusMessagePrivate *priv;

    g_return_val_if_fail (domain > 0 && domain <= G_MAXUINT8, NULL);
    g_return_val_if_fail (code <= G_MAXUINT8, NULL);
    g_return_val_if_fail (description, NULL);
    g_return_val_if_fail (*description != '\0', NULL);

    if (!(names = g_new0 (gchar *, n_properties))) {
        g_warning ("allocation error in %s", G_STRFUNC);
        return NULL;
    }
    if (!(values = g_new0 (GValue, n_properties))) {
        g_warning ("allocation error in %s", G_STRFUNC);
        return NULL;
    }

    names[0] = "domain";
    g_value_init (&values[0], G_TYPE_UCHAR);
    g_value_set_uchar (&values[0], domain);
    names[1] = "code";
    g_value_init (&values[1], G_TYPE_UCHAR);
    g_value_set_uchar (&values[1], code);
    names[2] = "title";
    g_value_init (&values[2], G_TYPE_STRING);
    g_value_set_string (&values[2], title);
    names[3] = "description";
    g_value_init (&values[3], G_TYPE_STRING);
    g_value_set_string (&values[3], description);

#define IF_PROPERTY_IS_MATCHED(property, gtype, ctype) \
    if (!g_strcmp0 (prop, #property)) {                                       \
        if (!(names_tmp = g_renew (gchar *, names, n_properties + 1))) {      \
            g_warning ("allocation error in %s", G_STRFUNC);                  \
            va_end (var_args);                                                \
            g_free (names);                                                   \
            for (i = 0; i <= n_properties; i++)                               \
                g_value_unset (&values[i]);                                   \
            g_free (values);                                                  \
            return NULL;                                                      \
        }                                                                     \
        names = names_tmp;                                                    \
        names[n_properties] = prop;                                           \
        if (!(values_tmp = g_renew (GValue, values, n_properties + 1))) {     \
            g_warning ("allocation error in %s", G_STRFUNC);                  \
            va_end (var_args);                                                \
            g_free (names);                                                   \
            for (i = 0; i < n_properties; i++)                               \
                g_value_unset (&values[i]);                                   \
            g_free (values);                                                  \
            return NULL;                                                      \
        }                                                                     \
        values = values_tmp;                                                  \
        memset (&values[n_properties], 0, sizeof (GValue));                   \
        g_value_init (&values[n_properties], (gtype));                        \
        g_value_set_## ctype (&values[n_properties],                          \
                              va_arg (var_args, g ## ctype));                 \
    }

    va_start (var_args, description);
    while ((prop =  va_arg (var_args, gchar *))) {
        IF_PROPERTY_IS_MATCHED (timeout, G_TYPE_INT, int)
        else IF_PROPERTY_IS_MATCHED (progress, G_TYPE_INT, int)
        else IF_PROPERTY_IS_MATCHED (serial, G_TYPE_UINT, uint)
        else {
            g_warning ("wrong parameter %s in %s", prop, G_STRFUNC);
            for (i = 0; i < n_properties; i++)
                g_value_unset (&values[i]);
            g_free (values);
            g_free (names);
            va_end (var_args);
            return NULL;
        }
        n_properties++;
    }
    va_end (var_args);

#undef IF_PROPERTY_IS_MATCHED

    msg = (IBusMessage *)g_object_new_with_properties (IBUS_TYPE_MESSAGE,
                                                       n_properties,
                                                       (const gchar **)names,
                                                       (const GValue*)values);


    for (i = 0; i < n_properties; i++)
        g_value_unset (&values[i]);
    g_free (values);
    g_free (names);
    if (!IBUS_IS_MESSAGE (msg)) {
        g_warning ("msg is not IBusMessage in %s", G_STRFUNC);
        return NULL;
    }
    priv = IBUS_MESSAGE_GET_PRIVATE (msg);
    /* name is required. Other properties are set in class_init by default. */
    g_assert (priv->domain > 0 && priv->domain <= G_MAXUINT8);
    g_assert (priv->code <= G_MAXUINT8);
    g_assert (priv->description);
    g_assert (*(priv->description) != '\0');

    return msg;
}
