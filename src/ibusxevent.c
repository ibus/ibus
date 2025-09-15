/* -*- mode: C; c-basic-offset: 4; indent-tabs-mode: nil; -*- */
/* vim:set et sts=4: */
/* ibus - The Input Bus
 * Copyright (C) 2018-2025 Takao Fujiwara <takao.fujiwara1@gmail.com>
 * Copyright (C) 2018-2019 Red Hat, Inc.
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
#include "ibusxevent.h"

#define IBUS_EXTENSION_EVENT_VERSION 1
#define IBUS_EXTENSION_EVENT_GET_PRIVATE(o)                             \
   ((IBusExtensionEventPrivate *)ibus_extension_event_get_instance_private (o))

#define IBUS_X_EVENT_VERSION 1
#define IBUS_X_EVENT_GET_PRIVATE(o)                                     \
   ((IBusXEventPrivate *)ibus_x_event_get_instance_private (o))

enum {
    PROP_0,
    PROP_VERSION,
    PROP_NAME,
    PROP_IS_ENABLED,
    PROP_IS_EXTENSION,
    PROP_PARAMS,
    PROP_EVENT_TYPE,
    PROP_WINDOW,
    PROP_SEND_EVENT,
    PROP_SERIAL,
    PROP_TIME,
    PROP_STATE,
    PROP_KEYVAL,
    PROP_LENGTH,
    PROP_STRING,
    PROP_HARDWARE_KEYCODE,
    PROP_GROUP,
    PROP_IS_MODIFIER,
    PROP_ROOT,
    PROP_SUBWINDOW,
    PROP_X,
    PROP_Y,
    PROP_X_ROOT,
    PROP_Y_ROOT,
    PROP_SAME_SCREEN,
    PROP_PURPOSE
};


struct _IBusExtensionEventPrivate {
    guint     version;
    gchar    *name;
    gboolean  is_enabled;
    gboolean  is_extension;
    gchar    *params;
};

struct _IBusXEventPrivate {
    guint    version;
    guint32  time;
    guint    state;
    guint    keyval;
    gint     length;
    gchar   *string;
    guint16  hardware_keycode;
    guint8   group;
    gboolean is_modifier;
    guint    root;
    guint    subwindow;
    gint     x;
    gint     y;
    gint     x_root;
    gint     y_root;
    gboolean same_screen;
    gchar   *purpose;
};

/* functions prototype */
static void      ibus_extension_event_destroy      (IBusExtensionEvent *event);
static void      ibus_extension_event_set_property (IBusExtensionEvent *event,
                                                    guint               prop_id,
                                                    const GValue       *value,
                                                    GParamSpec         *pspec);
static void      ibus_extension_event_get_property (IBusExtensionEvent *event,
                                                    guint               prop_id,
                                                    GValue             *value,
                                                    GParamSpec         *pspec);
static gboolean  ibus_extension_event_serialize    (IBusExtensionEvent *event,
                                                    GVariantBuilder
                                                                      *builder);
static gint      ibus_extension_event_deserialize  (IBusExtensionEvent *event,
                                                    GVariant
                                                                      *variant);
static gboolean  ibus_extension_event_copy         (IBusExtensionEvent
                                                                          *dest,
                                                    const IBusExtensionEvent
                                                                          *src);
static void      ibus_x_event_destroy              (IBusXEvent         *event);
static void      ibus_x_event_set_property         (IBusXEvent         *event,
                                                    guint               prop_id,
                                                    const GValue       *value,
                                                    GParamSpec         *pspec);
static void      ibus_x_event_get_property         (IBusXEvent         *event,
                                                    guint               prop_id,
                                                    GValue             *value,
                                                    GParamSpec         *pspec);
static gboolean  ibus_x_event_serialize            (IBusXEvent         *event,
                                                    GVariantBuilder
                                                                      *builder);
static gint      ibus_x_event_deserialize          (IBusXEvent         *event,
                                                    GVariant
                                                                      *variant);
static gboolean  ibus_x_event_copy                 (IBusXEvent         *dest,
                                                    const IBusXEvent   *src);

G_DEFINE_TYPE_WITH_PRIVATE (IBusExtensionEvent,
                            ibus_extension_event,
                            IBUS_TYPE_SERIALIZABLE)
G_DEFINE_TYPE_WITH_PRIVATE (IBusXEvent,
                            ibus_x_event,
                            IBUS_TYPE_SERIALIZABLE)

static void
ibus_extension_event_class_init (IBusExtensionEventClass *class)
{
    GObjectClass *gobject_class = G_OBJECT_CLASS (class);
    IBusObjectClass *object_class = IBUS_OBJECT_CLASS (class);
    IBusSerializableClass *serializable_class = IBUS_SERIALIZABLE_CLASS (class);

    gobject_class->set_property =
            (GObjectSetPropertyFunc) ibus_extension_event_set_property;
    gobject_class->get_property =
            (GObjectGetPropertyFunc) ibus_extension_event_get_property;

    object_class->destroy =
            (IBusObjectDestroyFunc) ibus_extension_event_destroy;

    serializable_class->serialize   =
            (IBusSerializableSerializeFunc) ibus_extension_event_serialize;
    serializable_class->deserialize =
            (IBusSerializableDeserializeFunc) ibus_extension_event_deserialize;
    serializable_class->copy        =
            (IBusSerializableCopyFunc) ibus_extension_event_copy;

    /* install properties */
    /**
     * IBusExtensionEvent:version:
     *
     * Version of the #IBusExtensionEvent.
     */
    g_object_class_install_property (gobject_class,
                    PROP_VERSION,
                    g_param_spec_uint ("version",
                        "version",
                        "version",
                        0,
                        G_MAXUINT32,
                        IBUS_EXTENSION_EVENT_VERSION,
                        G_PARAM_READABLE));

    /**
     * IBusExtensionEvent:name:
     *
     * Name of the extension in the #IBusExtensionEvent.
     */
    g_object_class_install_property (gobject_class,
                    PROP_NAME,
                    g_param_spec_string ("name",
                        "name",
                        "name of the extension",
                        "",
                        G_PARAM_READWRITE |
                        G_PARAM_CONSTRUCT_ONLY));

    /**
     * IBusExtensionEvent:is-enabled:
     *
     * %TRUE if the extension is enabled in the #IBusExtensionEvent.
     */
    g_object_class_install_property (gobject_class,
                    PROP_IS_ENABLED,
                    g_param_spec_boolean ("is-enabled",
                        "is enabled",
                        "if the extension is enabled",
                        FALSE,
                        G_PARAM_READWRITE |
                        G_PARAM_CONSTRUCT_ONLY));

    /**
     * IBusExtensionEvent:is-extension:
     *
     * %TRUE if the #IBusExtensionEvent is called by an extension.
     * %FALSE if the #IBusExtensionEvent is called by an active engine or
     * panel.
     * If this value is %TRUE, the event is send to ibus-daemon, an active
     * engine. If it's %FALSE, the event is sned to ibus-daemon, panels.
     */
    g_object_class_install_property (gobject_class,
                    PROP_IS_EXTENSION,
                    g_param_spec_boolean ("is-extension",
                        "is extension",
                        "if the event is called by an extension",
                        FALSE,
                        G_PARAM_READWRITE |
                        G_PARAM_CONSTRUCT_ONLY));

    /**
     * IBusExtensionEvent:params:
     *
     * Parameters to enable the extension in the #IBusExtensionEvent.
     */
    g_object_class_install_property (gobject_class,
                    PROP_PARAMS,
                    g_param_spec_string ("params",
                        "params",
                        "Parameters to enable the extension",
                        "",
                        G_PARAM_READWRITE |
                        G_PARAM_CONSTRUCT_ONLY));
}

static void
ibus_extension_event_init (IBusExtensionEvent *event)
{
    event->priv = IBUS_EXTENSION_EVENT_GET_PRIVATE (event);
    event->priv->version = IBUS_EXTENSION_EVENT_VERSION;
}

static void
ibus_extension_event_destroy (IBusExtensionEvent *event)
{
    g_clear_pointer (&event->priv->name, g_free);
    g_clear_pointer (&event->priv->params, g_free);

    IBUS_OBJECT_CLASS(ibus_extension_event_parent_class)->
            destroy (IBUS_OBJECT (event));
}

static void
ibus_extension_event_set_property (IBusExtensionEvent   *event,
                                   guint         prop_id,
                                   const GValue *value,
                                   GParamSpec   *pspec)
{
    IBusExtensionEventPrivate *priv = event->priv;

    switch (prop_id) {
    case PROP_NAME:
        g_free (priv->name);
        priv->name = g_value_dup_string (value);
        break;
    case PROP_IS_ENABLED:
        priv->is_enabled = g_value_get_boolean (value);
        break;
    case PROP_IS_EXTENSION:
        priv->is_extension = g_value_get_boolean (value);
        break;
    case PROP_PARAMS:
        priv->params = g_value_dup_string (value);
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (event, prop_id, pspec);
    }
}

static void
ibus_extension_event_get_property (IBusExtensionEvent *event,
                                   guint               prop_id,
                                   GValue             *value,
                                   GParamSpec         *pspec)
{
    IBusExtensionEventPrivate *priv = event->priv;
    switch (prop_id) {
    case PROP_VERSION:
        g_value_set_uint (value, priv->version);
        break;
    case PROP_NAME:
        g_value_set_string (value, priv->name);
        break;
    case PROP_IS_ENABLED:
        g_value_set_boolean (value, priv->is_enabled);
        break;
    case PROP_IS_EXTENSION:
        g_value_set_boolean (value, priv->is_extension);
        break;
    case PROP_PARAMS:
        g_value_set_string (value, priv->params);
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (event, prop_id, pspec);
    }
}

static gboolean
ibus_extension_event_serialize (IBusExtensionEvent *event,
                                GVariantBuilder    *builder)
{
    gboolean retval;
    IBusExtensionEventPrivate *priv;

    retval = IBUS_SERIALIZABLE_CLASS (ibus_extension_event_parent_class)->
            serialize ((IBusSerializable *)event, builder);
    g_return_val_if_fail (retval, FALSE);
    /* End dict iter */

    priv = event->priv;
#define NOTNULL(s) ((s) != NULL ? (s) : "")
    /* If you will add a new property, you can append it at the end and
     * you should not change the serialized order of name, longname,
     * description, ... because the order is also used in other applications
     * likes ibus-qt. */
    g_variant_builder_add (builder, "u", priv->version);
    g_variant_builder_add (builder, "s", NOTNULL (priv->name));
    g_variant_builder_add (builder, "b", priv->is_enabled);
    g_variant_builder_add (builder, "b", priv->is_extension);
    g_variant_builder_add (builder, "s", NOTNULL (priv->params));
#undef NOTNULL

    return TRUE;
}

static gint
ibus_extension_event_deserialize (IBusExtensionEvent *event,
                                  GVariant           *variant)
{
    gint retval;
    IBusExtensionEventPrivate *priv;

    retval = IBUS_SERIALIZABLE_CLASS (ibus_extension_event_parent_class)->
            deserialize ((IBusSerializable *)event, variant);
    g_return_val_if_fail (retval, 0);

    priv = event->priv;
    /* If you will add a new property, you can append it at the end and
     * you should not change the serialized order of name, longname,
     * description, ... because the order is also used in other applications
     * likes ibus-qt. */
    g_variant_get_child (variant, retval++, "u", &priv->version);
    ibus_g_variant_get_child_string (variant, retval++,
                                     &priv->name);
    g_variant_get_child (variant, retval++, "b", &priv->is_enabled);
    g_variant_get_child (variant, retval++, "b", &priv->is_extension);
    ibus_g_variant_get_child_string (variant, retval++,
                                     &priv->params);

    return retval;
}

static gboolean
ibus_extension_event_copy (IBusExtensionEvent       *dest,
                           const IBusExtensionEvent *src)
{
    gboolean retval;
    IBusExtensionEventPrivate *dest_priv = dest->priv;
    IBusExtensionEventPrivate *src_priv = src->priv;

    retval = IBUS_SERIALIZABLE_CLASS (ibus_extension_event_parent_class)->
            copy ((IBusSerializable *)dest, (IBusSerializable *)src);
    g_return_val_if_fail (retval, FALSE);

    dest_priv->version           = src_priv->version;
    dest_priv->name              = g_strdup (src_priv->name);
    dest_priv->is_enabled        = src_priv->is_enabled;
    dest_priv->is_extension      = src_priv->is_extension;
    dest_priv->params            = g_strdup (src_priv->params);
    return TRUE;
}

IBusExtensionEvent *
ibus_extension_event_new (const gchar   *first_property_name,
                          ...)
{
    va_list var_args;
    IBusExtensionEvent *event;

    va_start (var_args, first_property_name);
    event = (IBusExtensionEvent *) g_object_new_valist (
            IBUS_TYPE_EXTENSION_EVENT,
            first_property_name,
            var_args);
    va_end (var_args);
    g_assert (event->priv->version != 0);
    return event;
}

guint
ibus_extension_event_get_version (IBusExtensionEvent *event)
{
    g_return_val_if_fail (IBUS_IS_EXTENSION_EVENT (event), 0);
    return event->priv->version;
}

const gchar *
ibus_extension_event_get_name (IBusExtensionEvent *event)
{
    g_return_val_if_fail (IBUS_IS_EXTENSION_EVENT (event), "");
    return event->priv->name;
}

gboolean
ibus_extension_event_is_enabled (IBusExtensionEvent *event)
{
    g_return_val_if_fail (IBUS_IS_EXTENSION_EVENT (event), FALSE);
    return event->priv->is_enabled;
}

gboolean
ibus_extension_event_is_extension (IBusExtensionEvent *event)
{
    g_return_val_if_fail (IBUS_IS_EXTENSION_EVENT (event), FALSE);
    return event->priv->is_extension;
}

const gchar *
ibus_extension_event_get_params (IBusExtensionEvent *event)
{
    g_return_val_if_fail (IBUS_IS_EXTENSION_EVENT (event), "");
    return event->priv->params;
}


static void
ibus_x_event_class_init (IBusXEventClass *class)
{
    GObjectClass *gobject_class = G_OBJECT_CLASS (class);
    IBusObjectClass *object_class = IBUS_OBJECT_CLASS (class);
    IBusSerializableClass *serializable_class = IBUS_SERIALIZABLE_CLASS (class);

    gobject_class->set_property =
            (GObjectSetPropertyFunc) ibus_x_event_set_property;
    gobject_class->get_property =
            (GObjectGetPropertyFunc) ibus_x_event_get_property;

    object_class->destroy = (IBusObjectDestroyFunc) ibus_x_event_destroy;

    serializable_class->serialize   =
            (IBusSerializableSerializeFunc) ibus_x_event_serialize;
    serializable_class->deserialize =
            (IBusSerializableDeserializeFunc) ibus_x_event_deserialize;
    serializable_class->copy        =
            (IBusSerializableCopyFunc) ibus_x_event_copy;

    /* install properties */
    /**
     * IBusXEvent:version:
     *
     * Version of this IBusXEvent.
     */
    g_object_class_install_property (gobject_class,
                    PROP_VERSION,
                    g_param_spec_uint ("version",
                        "version",
                        "version",
                        0,
                        G_MAXUINT32,
                        IBUS_X_EVENT_VERSION,
                        G_PARAM_READABLE));

    /**
     * IBusXEvent:event-type:
     *
     * IBusXEventType of this IBusXEvent.
     */
    g_object_class_install_property (gobject_class,
                    PROP_EVENT_TYPE,
                    g_param_spec_int ("event-type",
                        "event type",
                        "event type",
                        -1,
                        G_MAXINT32,
                        -1,
                        G_PARAM_READWRITE |
                        G_PARAM_CONSTRUCT_ONLY));

    /**
     * IBusXEvent:window:
     *
     * window of this IBusXEvent.
     */
    g_object_class_install_property (gobject_class,
                    PROP_WINDOW,
                    g_param_spec_uint ("window",
                        "window",
                        "window",
                        0,
                        G_MAXUINT32,
                        0,
                        G_PARAM_READWRITE |
                        G_PARAM_CONSTRUCT_ONLY));

    /**
     * IBusXEvent:send-event:
     *
     * send_event of this IBusXEvent.
     */
    g_object_class_install_property (gobject_class,
                    PROP_SEND_EVENT,
                    g_param_spec_int ("send-event",
                        "send event",
                        "send event",
                        0,
                        G_MAXINT8,
                        0,
                        G_PARAM_READWRITE |
                        G_PARAM_CONSTRUCT_ONLY));

    /**
     * IBusXEvent:serial:
     *
     * serial of this IBusXEvent.
     */
    g_object_class_install_property (gobject_class,
                    PROP_SERIAL,
                    g_param_spec_ulong ("serial",
                        "serial",
                        "serial",
                        0,
                        G_MAXUINT64,
                        0,
                        G_PARAM_READWRITE |
                        G_PARAM_CONSTRUCT_ONLY));

    /**
     * IBusXEvent:time:
     *
     * time of this IBusXEvent.
     */
    g_object_class_install_property (gobject_class,
                    PROP_TIME,
                    g_param_spec_uint ("time",
                        "time",
                        "time",
                        0,
                        G_MAXUINT32,
                        0,
                        G_PARAM_READWRITE |
                        G_PARAM_CONSTRUCT_ONLY));

    /**
     * IBusXEvent:state:
     *
     * state of this IBusXEvent.
     */
    g_object_class_install_property (gobject_class,
                    PROP_STATE,
                    g_param_spec_uint ("state",
                        "state",
                        "state",
                        0,
                        G_MAXUINT32,
                        0,
                        G_PARAM_READWRITE |
                        G_PARAM_CONSTRUCT_ONLY));

    /**
     * IBusXEvent:keyval:
     *
     * keyval of this IBusXEvent.
     */
    g_object_class_install_property (gobject_class,
                    PROP_KEYVAL,
                    g_param_spec_uint ("keyval",
                        "keyval",
                        "keyval",
                        0,
                        G_MAXUINT32,
                        0,
                        G_PARAM_READWRITE |
                        G_PARAM_CONSTRUCT_ONLY));

    /**
     * IBusXEvent:length:
     *
     * keyval of this IBusXEvent.
     */
    g_object_class_install_property (gobject_class,
                    PROP_LENGTH,
                    g_param_spec_int ("length",
                        "length",
                        "length",
                        -1,
                        G_MAXINT32,
                        0,
                        G_PARAM_READWRITE |
                        G_PARAM_CONSTRUCT_ONLY));

    /**
     * IBusXEvent:string:
     *
     * string of this IBusXEvent.
     */
    g_object_class_install_property (gobject_class,
                    PROP_STRING,
                    g_param_spec_string ("string",
                        "string",
                        "string",
                        "",
                        G_PARAM_READWRITE |
                        G_PARAM_CONSTRUCT_ONLY));

    /**
     * IBusXEvent:hardware-keycode:
     *
     * hardware keycode of this IBusXEvent.
     */
    g_object_class_install_property (gobject_class,
                    PROP_HARDWARE_KEYCODE,
                    g_param_spec_uint ("hardware-keycode",
                        "hardware keycode",
                        "hardware keycode",
                        0,
                        G_MAXUINT16,
                        0,
                        G_PARAM_READWRITE |
                        G_PARAM_CONSTRUCT_ONLY));

    /**
     * IBusXEvent:group:
     *
     * group of this IBusXEvent.
     */
    g_object_class_install_property (gobject_class,
                    PROP_GROUP,
                    g_param_spec_uint ("group",
                        "group",
                        "group",
                        0,
                        G_MAXUINT8,
                        0,
                        G_PARAM_READWRITE |
                        G_PARAM_CONSTRUCT_ONLY));

    /**
     * IBusXEvent:is-modifier:
     *
     * is_modifier of this IBusXEvent.
     */
    g_object_class_install_property (gobject_class,
                    PROP_IS_MODIFIER,
                    g_param_spec_boolean ("is-modifier",
                        "is modifier",
                        "is modifier",
                        FALSE,
                        G_PARAM_READWRITE |
                        G_PARAM_CONSTRUCT_ONLY));

    /**
     * IBusXEvent:root:
     *
     * root window of this IBusXEvent.
     */
    g_object_class_install_property (gobject_class,
                    PROP_ROOT,
                    g_param_spec_uint ("root",
                        "root",
                        "root",
                        0,
                        G_MAXUINT32,
                        0,
                        G_PARAM_READWRITE |
                        G_PARAM_CONSTRUCT_ONLY));

    /**
     * IBusXEvent:subwindow:
     *
     * subwindow of this IBusXEvent.
     */
    g_object_class_install_property (gobject_class,
                    PROP_SUBWINDOW,
                    g_param_spec_uint ("subwindow",
                        "subwindow",
                        "subwindow",
                        0,
                        G_MAXUINT32,
                        0,
                        G_PARAM_READWRITE |
                        G_PARAM_CONSTRUCT_ONLY));

    /**
     * IBusXEvent:x:
     *
     * x of this IBusXEvent.
     */
    g_object_class_install_property (gobject_class,
                    PROP_X,
                    g_param_spec_int ("x",
                        "x",
                        "x",
                        G_MININT32,
                        G_MAXINT32,
                        0,
                        G_PARAM_READWRITE |
                        G_PARAM_CONSTRUCT_ONLY));

    /**
     * IBusXEvent:y:
     *
     * x of this IBusXEvent.
     */
    g_object_class_install_property (gobject_class,
                    PROP_Y,
                    g_param_spec_int ("y",
                        "y",
                        "y",
                        G_MININT32,
                        G_MAXINT32,
                        0,
                        G_PARAM_READWRITE |
                        G_PARAM_CONSTRUCT_ONLY));

    /**
     * IBusXEvent:x-root:
     *
     * root-x of this IBusXEvent.
     */
    g_object_class_install_property (gobject_class,
                    PROP_X_ROOT,
                    g_param_spec_int ("x-root",
                        "x root",
                        "x root",
                        G_MININT32,
                        G_MAXINT32,
                        0,
                        G_PARAM_READWRITE |
                        G_PARAM_CONSTRUCT_ONLY));

    /**
     * IBusXEvent:y-root:
     *
     * root-y of this IBusXEvent.
     */
    g_object_class_install_property (gobject_class,
                    PROP_Y_ROOT,
                    g_param_spec_int ("y-root",
                        "y root",
                        "y root",
                        G_MININT32,
                        G_MAXINT32,
                        0,
                        G_PARAM_READWRITE |
                        G_PARAM_CONSTRUCT_ONLY));

    /**
     * IBusXEvent:same-screen:
     *
     * same_screen of this IBusXEvent.
     */
    g_object_class_install_property (gobject_class,
                    PROP_SAME_SCREEN,
                    g_param_spec_boolean ("same-screen",
                        "same screen",
                        "same screen",
                        TRUE,
                        G_PARAM_READWRITE |
                        G_PARAM_CONSTRUCT_ONLY));

    /**
     * IBusXEvent:purpose:
     *
     * purpose of this IBusXEvent.
     */
    g_object_class_install_property (gobject_class,
                    PROP_PURPOSE,
                    g_param_spec_string ("purpose",
                        "purpose",
                        "purpose",
                        "",
                        G_PARAM_READWRITE |
                        G_PARAM_CONSTRUCT_ONLY));
}

static void
ibus_x_event_init (IBusXEvent *event)
{
    event->priv = IBUS_X_EVENT_GET_PRIVATE (event);
    event->priv->version = IBUS_X_EVENT_VERSION;
}

static void
ibus_x_event_destroy (IBusXEvent *event)
{
    g_clear_pointer (&event->priv->string, g_free);
    g_clear_pointer (&event->priv->purpose, g_free);

    IBUS_OBJECT_CLASS(ibus_x_event_parent_class)->destroy (IBUS_OBJECT (event));
}

static void
ibus_x_event_set_property (IBusXEvent   *event,
                           guint         prop_id,
                           const GValue *value,
                           GParamSpec   *pspec)
{
    IBusXEventPrivate *priv = event->priv;

    switch (prop_id) {
    case PROP_EVENT_TYPE:
        event->event_type = g_value_get_int (value);
        break;
    case PROP_WINDOW:
        event->window = g_value_get_uint (value);
        break;
    case PROP_SEND_EVENT:
        event->send_event = g_value_get_int (value);
        break;
    case PROP_SERIAL:
        event->serial = g_value_get_ulong (value);
        break;
    case PROP_TIME:
        priv->time = g_value_get_uint (value);
        break;
    case PROP_STATE:
        priv->state = g_value_get_uint (value);
        break;
    case PROP_KEYVAL:
        priv->keyval = g_value_get_uint (value);
        break;
    case PROP_LENGTH:
        priv->length = g_value_get_int (value);
        break;
    case PROP_STRING:
        g_free (priv->string);
        priv->string = g_value_dup_string (value);
        break;
    case PROP_HARDWARE_KEYCODE:
        priv->hardware_keycode = g_value_get_uint (value);
        break;
    case PROP_GROUP:
        priv->group = g_value_get_uint (value);
        break;
    case PROP_IS_MODIFIER:
        priv->is_modifier = g_value_get_boolean (value);
        break;
    case PROP_ROOT:
        priv->root = g_value_get_uint (value);
        break;
    case PROP_SUBWINDOW:
        priv->subwindow = g_value_get_uint (value);
        break;
    case PROP_X:
        priv->x = g_value_get_int (value);
        break;
    case PROP_Y:
        priv->y = g_value_get_int (value);
        break;
    case PROP_X_ROOT:
        priv->x_root = g_value_get_int (value);
        break;
    case PROP_Y_ROOT:
        priv->y_root = g_value_get_int (value);
        break;
    case PROP_SAME_SCREEN:
        priv->same_screen = g_value_get_boolean (value);
        break;
    case PROP_PURPOSE:
        g_free (priv->purpose);
        priv->purpose = g_value_dup_string (value);
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (event, prop_id, pspec);
    }
}

static void
ibus_x_event_get_property (IBusXEvent *event,
                          guint        prop_id,
                          GValue      *value,
                          GParamSpec  *pspec)
{
    IBusXEventPrivate *priv = event->priv;
    switch (prop_id) {
    case PROP_VERSION:
        g_value_set_uint (value, priv->version);
        break;
    case PROP_EVENT_TYPE:
        g_value_set_int (value, event->event_type);
        break;
    case PROP_WINDOW:
        g_value_set_uint (value, event->window);
        break;
    case PROP_SEND_EVENT:
        g_value_set_int (value, event->send_event);
        break;
    case PROP_SERIAL:
        g_value_set_ulong (value, event->serial);
        break;
    case PROP_TIME:
        g_value_set_uint (value, priv->time);
        break;
    case PROP_STATE:
        g_value_set_uint (value, priv->state);
        break;
    case PROP_KEYVAL:
        g_value_set_uint (value, priv->keyval);
        break;
    case PROP_LENGTH:
        g_value_set_int (value, priv->length);
        break;
    case PROP_STRING:
        g_value_set_string (value, priv->string);
        break;
    case PROP_HARDWARE_KEYCODE:
        g_value_set_uint (value, priv->hardware_keycode);
        break;
    case PROP_GROUP:
        g_value_set_uint (value, priv->group);
        break;
    case PROP_IS_MODIFIER:
        g_value_set_boolean (value, priv->is_modifier);
        break;
    case PROP_ROOT:
        g_value_set_uint (value, priv->root);
        break;
    case PROP_SUBWINDOW:
        g_value_set_uint (value, priv->subwindow);
        break;
    case PROP_X:
        g_value_set_int (value, priv->x);
        break;
    case PROP_Y:
        g_value_set_int (value, priv->y);
        break;
    case PROP_X_ROOT:
        g_value_set_int (value, priv->x_root);
        break;
    case PROP_Y_ROOT:
        g_value_set_int (value, priv->y_root);
        break;
    case PROP_SAME_SCREEN:
        g_value_set_boolean (value, priv->same_screen);
        break;
    case PROP_PURPOSE:
        g_value_set_string (value, priv->purpose);
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (event, prop_id, pspec);
    }
}

static gboolean
ibus_x_event_serialize (IBusXEvent      *event,
                        GVariantBuilder *builder)
{
    gboolean retval;
    IBusXEventPrivate *priv;

    retval = IBUS_SERIALIZABLE_CLASS (ibus_x_event_parent_class)->
            serialize ((IBusSerializable *)event, builder);
    g_return_val_if_fail (retval, FALSE);
    /* End dict iter */

    priv = event->priv;
#define NOTNULL(s) ((s) != NULL ? (s) : "")
    /* If you will add a new property, you can append it at the end and
     * you should not change the serialized order of name, longname,
     * description, ... because the order is also used in other applications
     * likes ibus-qt. */
    g_variant_builder_add (builder, "u", priv->version);
    g_variant_builder_add (builder, "u", event->event_type);
    g_variant_builder_add (builder, "u", event->window);
    g_variant_builder_add (builder, "i", event->send_event);
    g_variant_builder_add (builder, "t", event->serial);
    g_variant_builder_add (builder, "u", priv->time);
    g_variant_builder_add (builder, "u", priv->state);
    g_variant_builder_add (builder, "u", priv->keyval);
    g_variant_builder_add (builder, "i", priv->length);
    g_variant_builder_add (builder, "s", NOTNULL (priv->string));
    g_variant_builder_add (builder, "u", priv->hardware_keycode);
    g_variant_builder_add (builder, "u", priv->group);
    g_variant_builder_add (builder, "b", priv->is_modifier);
    g_variant_builder_add (builder, "u", priv->root);
    g_variant_builder_add (builder, "u", priv->subwindow);
    g_variant_builder_add (builder, "i", priv->x);
    g_variant_builder_add (builder, "i", priv->y);
    g_variant_builder_add (builder, "i", priv->x_root);
    g_variant_builder_add (builder, "i", priv->y_root);
    g_variant_builder_add (builder, "b", priv->same_screen);
    g_variant_builder_add (builder, "s", NOTNULL (priv->purpose));
#undef NOTNULL

    return TRUE;
}

static gint
ibus_x_event_deserialize (IBusXEvent *event,
                          GVariant   *variant)
{
    gint retval;
    IBusXEventPrivate *priv;

    retval = IBUS_SERIALIZABLE_CLASS (ibus_x_event_parent_class)->
            deserialize ((IBusSerializable *)event, variant);
    g_return_val_if_fail (retval, 0);

    priv = event->priv;
    /* If you will add a new property, you can append it at the end and
     * you should not change the serialized order of name, longname,
     * description, ... because the order is also used in other applications
     * likes ibus-qt. */
    g_variant_get_child (variant, retval++, "u", &priv->version);
    g_variant_get_child (variant, retval++, "u", &event->event_type);
    g_variant_get_child (variant, retval++, "u", &event->window);
    g_variant_get_child (variant, retval++, "i", &event->send_event);
    g_variant_get_child (variant, retval++, "t", &event->serial);
    g_variant_get_child (variant, retval++, "u", &priv->time);
    g_variant_get_child (variant, retval++, "u", &priv->state);
    g_variant_get_child (variant, retval++, "u", &priv->keyval);
    g_variant_get_child (variant, retval++, "i", &priv->length);
    ibus_g_variant_get_child_string (variant, retval++,
                                     &priv->string);
    g_variant_get_child (variant, retval++, "u", &priv->hardware_keycode);
    g_variant_get_child (variant, retval++, "u", &priv->group);
    g_variant_get_child (variant, retval++, "b", &priv->is_modifier);
    g_variant_get_child (variant, retval++, "u", &priv->root);
    g_variant_get_child (variant, retval++, "u", &priv->subwindow);
    g_variant_get_child (variant, retval++, "i", &priv->x);
    g_variant_get_child (variant, retval++, "i", &priv->y);
    g_variant_get_child (variant, retval++, "i", &priv->x_root);
    g_variant_get_child (variant, retval++, "i", &priv->y_root);
    g_variant_get_child (variant, retval++, "b", &priv->same_screen);
    ibus_g_variant_get_child_string (variant, retval++,
                                     &priv->purpose);

    return retval;
}

static gboolean
ibus_x_event_copy (IBusXEvent       *dest,
                   const IBusXEvent *src)
{
    gboolean retval;
    IBusXEventPrivate *dest_priv = dest->priv;
    IBusXEventPrivate *src_priv = src->priv;

    retval = IBUS_SERIALIZABLE_CLASS (ibus_x_event_parent_class)->
            copy ((IBusSerializable *)dest, (IBusSerializable *)src);
    g_return_val_if_fail (retval, FALSE);

    dest_priv->version           = src_priv->version;
    dest->event_type             = src->event_type;
    dest->window                 = src->window;
    dest->send_event             = src->send_event;
    dest->serial                 = src->serial;
    dest_priv->time              = src_priv->time;
    dest_priv->state             = src_priv->state;
    dest_priv->keyval            = src_priv->keyval;
    dest_priv->length            = src_priv->length;
    dest_priv->string            = g_strdup (src_priv->string);
    dest_priv->hardware_keycode  = src_priv->hardware_keycode;
    dest_priv->group             = src_priv->group;
    dest_priv->is_modifier       = src_priv->is_modifier;
    dest_priv->root              = src_priv->root;
    dest_priv->subwindow         = src_priv->subwindow;
    dest_priv->x                 = src_priv->x;
    dest_priv->y                 = src_priv->y;
    dest_priv->x_root            = src_priv->x_root;
    dest_priv->y_root            = src_priv->y_root;
    dest_priv->same_screen       = src_priv->same_screen;
    dest_priv->purpose           = g_strdup (src_priv->purpose);

    return TRUE;
}

IBusXEvent *
ibus_x_event_new (const gchar   *first_property_name,
                  ...)
{
    va_list var_args;
    IBusXEvent *event;

    va_start (var_args, first_property_name);
    event = (IBusXEvent *) g_object_new_valist (IBUS_TYPE_X_EVENT,
                                                first_property_name,
                                                var_args);
    va_end (var_args);
    g_assert (event->priv->version != 0);
    g_assert (event->event_type != IBUS_X_EVENT_NOTHING);
    return event;
}

guint
ibus_x_event_get_version (IBusXEvent *event)
{
    g_return_val_if_fail (IBUS_IS_X_EVENT (event), 0);
    return event->priv->version;
}

IBusXEventType
ibus_x_event_get_event_type (IBusXEvent *event)
{
    g_return_val_if_fail (IBUS_IS_X_EVENT (event), 0);
    return event->event_type;
}

guint32
ibus_x_event_get_window (IBusXEvent *event)
{
    g_return_val_if_fail (IBUS_IS_X_EVENT (event), 0);
    return event->window;
}

gint8
ibus_x_event_get_send_event (IBusXEvent *event)
{
    g_return_val_if_fail (IBUS_IS_X_EVENT (event), -1);
    return event->send_event;
}

gulong
ibus_x_event_get_serial (IBusXEvent *event)
{
    g_return_val_if_fail (IBUS_IS_X_EVENT (event), 0);
    return event->serial;
}

guint32
ibus_x_event_get_time (IBusXEvent *event)
{
    g_return_val_if_fail (IBUS_IS_X_EVENT (event), 0);
    switch (event->event_type) {
    case IBUS_X_EVENT_KEY_PRESS:
    case IBUS_X_EVENT_KEY_RELEASE:
        break;
    default:
        g_return_val_if_reached (0);
    }
    return event->priv->time;
}

guint
ibus_x_event_get_state (IBusXEvent *event)
{
    g_return_val_if_fail (IBUS_IS_X_EVENT (event), 0);
    switch (event->event_type) {
    case IBUS_X_EVENT_KEY_PRESS:
    case IBUS_X_EVENT_KEY_RELEASE:
        break;
    default:
        g_return_val_if_reached (0);
    }
    return event->priv->state;
}

guint
ibus_x_event_get_keyval (IBusXEvent *event)
{
    g_return_val_if_fail (IBUS_IS_X_EVENT (event), 0);
    switch (event->event_type) {
    case IBUS_X_EVENT_KEY_PRESS:
    case IBUS_X_EVENT_KEY_RELEASE:
        break;
    default:
        g_return_val_if_reached (0);
    }
    return event->priv->keyval;
}

gint
ibus_x_event_get_length (IBusXEvent *event)
{
    g_return_val_if_fail (IBUS_IS_X_EVENT (event), -1);
    switch (event->event_type) {
    case IBUS_X_EVENT_KEY_PRESS:
    case IBUS_X_EVENT_KEY_RELEASE:
        break;
    default:
        g_return_val_if_reached (-1);
    }
    return event->priv->length;
}

const gchar *
ibus_x_event_get_string (IBusXEvent *event)
{
    g_return_val_if_fail (IBUS_IS_X_EVENT (event), "");
    switch (event->event_type) {
    case IBUS_X_EVENT_KEY_PRESS:
    case IBUS_X_EVENT_KEY_RELEASE:
        break;
    default:
        g_return_val_if_reached ("");
    }
    return event->priv->string;
}

guint16
ibus_x_event_get_hardware_keycode (IBusXEvent *event)
{
    g_return_val_if_fail (IBUS_IS_X_EVENT (event), 0);
    switch (event->event_type) {
    case IBUS_X_EVENT_KEY_PRESS:
    case IBUS_X_EVENT_KEY_RELEASE:
        break;
    default:
        g_return_val_if_reached (0);
    }
    return event->priv->hardware_keycode;
}

guint8
ibus_x_event_get_group (IBusXEvent *event)
{
    g_return_val_if_fail (IBUS_IS_X_EVENT (event), 0);
    switch (event->event_type) {
    case IBUS_X_EVENT_KEY_PRESS:
    case IBUS_X_EVENT_KEY_RELEASE:
        break;
    default:
        g_return_val_if_reached (0);
    }
    return event->priv->group;
}

gboolean
ibus_x_event_get_is_modifier (IBusXEvent *event)
{
    g_return_val_if_fail (IBUS_IS_X_EVENT (event), 0);
    switch (event->event_type) {
    case IBUS_X_EVENT_KEY_PRESS:
    case IBUS_X_EVENT_KEY_RELEASE:
        break;
    default:
        g_return_val_if_reached (0);
    }
    return event->priv->is_modifier;
}

guint32
ibus_x_event_get_root (IBusXEvent *event)
{
    g_return_val_if_fail (IBUS_IS_X_EVENT (event), 0);
    switch (event->event_type) {
    case IBUS_X_EVENT_KEY_PRESS:
    case IBUS_X_EVENT_KEY_RELEASE:
        break;
    default:
        g_return_val_if_reached (0);
    }
    return event->priv->root;
}

guint32
ibus_x_event_get_subwindow (IBusXEvent *event)
{
    g_return_val_if_fail (IBUS_IS_X_EVENT (event), 0);
    switch (event->event_type) {
    case IBUS_X_EVENT_KEY_PRESS:
    case IBUS_X_EVENT_KEY_RELEASE:
        break;
    default:
        g_return_val_if_reached (0);
    }
    return event->priv->subwindow;
}

gint
ibus_x_event_get_x (IBusXEvent *event)
{
    g_return_val_if_fail (IBUS_IS_X_EVENT (event), 0);
    switch (event->event_type) {
    case IBUS_X_EVENT_KEY_PRESS:
    case IBUS_X_EVENT_KEY_RELEASE:
        break;
    default:
        g_return_val_if_reached (0);
    }
    return event->priv->x;
}

gint
ibus_x_event_get_y (IBusXEvent *event)
{
    g_return_val_if_fail (IBUS_IS_X_EVENT (event), 0);
    switch (event->event_type) {
    case IBUS_X_EVENT_KEY_PRESS:
    case IBUS_X_EVENT_KEY_RELEASE:
        break;
    default:
        g_return_val_if_reached (0);
    }
    return event->priv->y;
}

gint
ibus_x_event_get_x_root (IBusXEvent *event)
{
    g_return_val_if_fail (IBUS_IS_X_EVENT (event), 0);
    switch (event->event_type) {
    case IBUS_X_EVENT_KEY_PRESS:
    case IBUS_X_EVENT_KEY_RELEASE:
        break;
    default:
        g_return_val_if_reached (0);
    }
    return event->priv->x_root;
}

gint
ibus_x_event_get_y_root (IBusXEvent *event)
{
    g_return_val_if_fail (IBUS_IS_X_EVENT (event), 0);
    switch (event->event_type) {
    case IBUS_X_EVENT_KEY_PRESS:
    case IBUS_X_EVENT_KEY_RELEASE:
        break;
    default:
        g_return_val_if_reached (0);
    }
    return event->priv->y_root;
}

gboolean
ibus_x_event_get_same_screen (IBusXEvent *event)
{
    g_return_val_if_fail (IBUS_IS_X_EVENT (event), TRUE);
    switch (event->event_type) {
    case IBUS_X_EVENT_KEY_PRESS:
    case IBUS_X_EVENT_KEY_RELEASE:
        break;
    default:
        g_return_val_if_reached (TRUE);
    }
    return event->priv->same_screen;
}

const gchar *
ibus_x_event_get_purpose (IBusXEvent *event)
{
    g_return_val_if_fail (IBUS_IS_X_EVENT (event), "");
    return event->priv->purpose;
}
