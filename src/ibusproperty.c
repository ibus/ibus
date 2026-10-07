/* -*- mode: C; c-basic-offset: 4; indent-tabs-mode: nil; -*- */
/* vim:set et sts=4: */
/* IBus - The Input Bus
 * Copyright (C) 2008-2010 Peng Huang <shawn.p.huang@gmail.com>
 * Copyright (C) 2008-2019 Red Hat, Inc.
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
#include "ibusinternal.h"
#include "ibusproperty.h"
#include "ibusproplist.h"
#include "ibusenumtypes.h"

enum {
    LAST_SIGNAL,
};

enum {
    PROP_0 = 0,
    PROP_KEY,
    PROP_ICON,
    PROP_LABEL,
    PROP_SYMBOL,
    PROP_TOOLTIP,
    PROP_SENSITIVE,
    PROP_VISIBLE,
    PROP_PROP_TYPE,
    PROP_STATE,
    PROP_SUB_PROPS,
};

/* _IBusPropertyPrivate */
struct _IBusPropertyPrivate {
    gchar    *key;
    gchar    *icon;
    IBusText *label;
    IBusText *symbol;
    IBusText *tooltip;

    gboolean sensitive;
    gboolean visible;
    IBusPropType type;
    IBusPropState state;

    IBusPropList *sub_props;
};

#define IBUS_PROPERTY_GET_PRIVATE(o)  \
   ((IBusPropertyPrivate *)ibus_property_get_instance_private (o))

/* functions prototype */
static void         ibus_property_set_property  (IBusProperty       *prop,
                                                 guint               prop_id,
                                                 const GValue       *value,
                                                 GParamSpec         *pspec);
static void         ibus_property_get_property  (IBusProperty       *prop,
                                                 guint               prop_id,
                                                 GValue             *value,
                                                 GParamSpec         *pspec);
static void         ibus_property_destroy       (IBusProperty       *prop);
static gboolean     ibus_property_serialize     (IBusProperty       *prop,
                                                 GVariantBuilder    *builder);
static gint         ibus_property_deserialize   (IBusProperty       *prop,
                                                 GVariant           *variant);
static gboolean     ibus_property_copy          (IBusProperty       *dest,
                                                 const IBusProperty *src);

G_DEFINE_TYPE_WITH_PRIVATE (IBusProperty, ibus_property, IBUS_TYPE_SERIALIZABLE)

static void
ibus_property_class_init (IBusPropertyClass *class)
{
    GObjectClass *gobject_class = G_OBJECT_CLASS (class);
    IBusObjectClass *object_class = IBUS_OBJECT_CLASS (class);
    IBusSerializableClass *serializable_class = IBUS_SERIALIZABLE_CLASS (class);

    gobject_class->set_property = (GObjectSetPropertyFunc) ibus_property_set_property;
    gobject_class->get_property = (GObjectGetPropertyFunc) ibus_property_get_property;

    object_class->destroy = (IBusObjectDestroyFunc) ibus_property_destroy;

    serializable_class->serialize   = (IBusSerializableSerializeFunc) ibus_property_serialize;
    serializable_class->deserialize = (IBusSerializableDeserializeFunc) ibus_property_deserialize;
    serializable_class->copy        = (IBusSerializableCopyFunc) ibus_property_copy;

    /* install properties */
    /**
     * IBusPropert:key:
     *
     * The key of property
     */
    g_object_class_install_property (gobject_class,
            PROP_KEY,
            g_param_spec_string ("key",
                    "key",
                    "The key of property",
                    "",
                    G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));

    /**
     * IBusPropert:icon:
     *
     * The icon of property
     */
    g_object_class_install_property (gobject_class,
            PROP_ICON,
            g_param_spec_string ("icon",
                    "icon",
                    "The icon of property",
                    "",
                    G_PARAM_READWRITE));

    /**
     * IBusPropert:label:
     *
     * The label of property
     */
    g_object_class_install_property (gobject_class,
            PROP_LABEL,
            g_param_spec_object("label",
                    "label",
                    "The label of property",
                    IBUS_TYPE_TEXT,
                    G_PARAM_READWRITE | G_PARAM_CONSTRUCT));

    /**
     * IBusPropert:symbol:
     *
     * The symbol of property
     */
    g_object_class_install_property (gobject_class,
            PROP_SYMBOL,
            g_param_spec_object("symbol",
                    "symbol",
                    "The symbol of property",
                    IBUS_TYPE_TEXT,
                    G_PARAM_READWRITE | G_PARAM_CONSTRUCT));

    /**
     * IBusPropert:tooltip:
     *
     * The tooltip of property
     */
    g_object_class_install_property (gobject_class,
            PROP_TOOLTIP,
            g_param_spec_object("tooltip",
                    "tooltip",
                    "The tooltip of property",
                    IBUS_TYPE_TEXT,
                    G_PARAM_READWRITE));

    /**
     * IBusPropert:sensitive:
     *
     * The sensitive of property
     */
    g_object_class_install_property (gobject_class,
            PROP_SENSITIVE,
            g_param_spec_boolean("sensitive",
                    "sensitive",
                    "The sensitive of property",
                    TRUE,
                    G_PARAM_READWRITE));

    /**
     * IBusPropert:visible:
     *
     * The visible of property
     */
    g_object_class_install_property (gobject_class,
            PROP_VISIBLE,
            g_param_spec_boolean("visible",
                    "visible",
                    "The visible of property",
                    TRUE,
                    G_PARAM_READWRITE));

    /**
     * IBusPropert:type:
     *
     * The type of property
     */
    g_object_class_install_property (gobject_class,
            PROP_PROP_TYPE,
            g_param_spec_enum("prop-type",
                    "prop-type",
                    "The type of property",
                    IBUS_TYPE_PROP_TYPE,
                    PROP_TYPE_NORMAL,
                    G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));
    /**
     * IBusPropert:state:
     *
     * The state of property
     */
    g_object_class_install_property (gobject_class,
            PROP_STATE,
            g_param_spec_enum("state",
                    "state",
                    "The state of property",
                    IBUS_TYPE_PROP_STATE,
                    PROP_STATE_UNCHECKED,
                    G_PARAM_READWRITE));

    /**
     * IBusPropert:sub-props:
     *
     * The sub properties of property
     */
    g_object_class_install_property (gobject_class,
            PROP_SUB_PROPS,
            g_param_spec_object("sub-props",
                    "sub properties",
                    "The sub properties of property",
                    IBUS_TYPE_PROP_LIST,
                    G_PARAM_READWRITE));
}

static void
ibus_property_init (IBusProperty *prop)
{
    prop->priv = IBUS_PROPERTY_GET_PRIVATE (prop);

    ibus_property_set_label (prop, NULL);
    ibus_property_set_symbol (prop, NULL);
    ibus_property_set_tooltip (prop, NULL);
    ibus_property_set_sub_props (prop, NULL);

}
static void
ibus_property_set_property (IBusProperty *prop,
                            guint         prop_id,
                            const GValue *value,
                            GParamSpec   *pspec)
{
    switch (prop_id) {
    case PROP_KEY:
        g_assert (prop->priv->key == NULL);
        prop->priv->key = g_value_dup_string (value);
        break;
    case PROP_ICON:
        ibus_property_set_icon (prop, g_value_get_string (value));
        break;
    case PROP_LABEL:
        ibus_property_set_label (prop, g_value_get_object (value));
        break;
    case PROP_SYMBOL:
        ibus_property_set_symbol (prop, g_value_get_object (value));
        break;
    case PROP_TOOLTIP:
        ibus_property_set_tooltip (prop, g_value_get_object (value));
        break;
    case PROP_SENSITIVE:
        ibus_property_set_sensitive (prop, g_value_get_boolean (value));
        break;
    case PROP_VISIBLE:
        ibus_property_set_visible (prop, g_value_get_boolean (value));
        break;
    case PROP_PROP_TYPE:
        prop->priv->type = g_value_get_enum (value);
        break;
    case PROP_STATE:
        ibus_property_set_state (prop, g_value_get_enum (value));
        break;
    case PROP_SUB_PROPS:
        ibus_property_set_sub_props (prop,
                (IBusPropList *)g_value_get_object (value));
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (prop, prop_id, pspec);
    }
}

static void
ibus_property_get_property (IBusProperty *prop,
                            guint         prop_id,
                            GValue       *value,
                            GParamSpec   *pspec)
{
    switch (prop_id) {
    case PROP_KEY:
        g_value_set_string (value, ibus_property_get_key (prop));
        break;
    case PROP_ICON:
        g_value_set_string (value, ibus_property_get_icon (prop));
        break;
    case PROP_LABEL:
        g_value_set_object (value, ibus_property_get_label (prop));
        break;
    case PROP_SYMBOL:
        g_value_set_object (value, ibus_property_get_symbol (prop));
        break;
    case PROP_TOOLTIP:
        g_value_set_object (value, ibus_property_get_tooltip (prop));
        break;
    case PROP_SENSITIVE:
        g_value_set_boolean (value, ibus_property_get_sensitive (prop));
        break;
    case PROP_VISIBLE:
        g_value_set_boolean (value, ibus_property_get_visible (prop));
        break;
    case PROP_PROP_TYPE:
        g_value_set_enum (value, ibus_property_get_prop_type (prop));
        break;
    case PROP_STATE:
        g_value_set_enum (value, ibus_property_get_state (prop));
        break;
    case PROP_SUB_PROPS:
        g_value_set_object (value, ibus_property_get_sub_props (prop));
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (prop, prop_id, pspec);
    }
}

static void
ibus_property_destroy (IBusProperty *prop)
{
    g_free (prop->priv->key);
    prop->priv->key = NULL;

    g_free (prop->priv->icon);
    prop->priv->icon = NULL;

    if (prop->priv->label) {
        g_object_unref (prop->priv->label);
        prop->priv->label = NULL;
    }

    if (prop->priv->symbol) {
        g_object_unref (prop->priv->symbol);
        prop->priv->symbol = NULL;
    }

    if (prop->priv->tooltip) {
        g_object_unref (prop->priv->tooltip);
        prop->priv->tooltip = NULL;
    }

    if (prop->priv->sub_props) {
        g_object_unref (prop->priv->sub_props);
        prop->priv->sub_props = NULL;
    }

    IBUS_OBJECT_CLASS (ibus_property_parent_class)->destroy ((IBusObject *)prop);
}

gboolean
ibus_property_serialize (IBusProperty    *prop,
                         GVariantBuilder *builder)
{
    gboolean retval;

    retval = IBUS_SERIALIZABLE_CLASS (ibus_property_parent_class)->serialize ((IBusSerializable *) prop, builder);
    g_return_val_if_fail (retval, FALSE);

    g_return_val_if_fail (IBUS_IS_PROPERTY (prop), FALSE);

    g_variant_builder_add (builder, "s", prop->priv->key);
    g_variant_builder_add (builder, "u", prop->priv->type);
    g_variant_builder_open (builder, G_VARIANT_TYPE_VARIANT);
    g_variant_builder_add_value (
            builder,
            ibus_serializable_serialize (
            (IBusSerializable *)prop->priv->label));
    g_variant_builder_close (builder);
    g_variant_builder_add (builder, "s", prop->priv->icon);
    g_variant_builder_open (builder, G_VARIANT_TYPE_VARIANT);
    g_variant_builder_add_value (
            builder,
            ibus_serializable_serialize (
            (IBusSerializable *)prop->priv->tooltip));
    g_variant_builder_close (builder);
    g_variant_builder_add (builder, "b", prop->priv->sensitive);
    g_variant_builder_add (builder, "b", prop->priv->visible);
    g_variant_builder_add (builder, "u", prop->priv->state);
    g_variant_builder_open (builder, G_VARIANT_TYPE_VARIANT);
    g_variant_builder_add_value (
            builder,
            ibus_serializable_serialize (
            (IBusSerializable *)prop->priv->sub_props));
    g_variant_builder_close (builder);
    /* Keep the serialized order for the compatibility when add new members. */
    g_variant_builder_open (builder, G_VARIANT_TYPE_VARIANT);
    g_variant_builder_add_value (
            builder,
            ibus_serializable_serialize (
            (IBusSerializable *)prop->priv->symbol));
    g_variant_builder_close (builder);

    return TRUE;
}

static gint
ibus_property_deserialize (IBusProperty *prop,
                           GVariant     *variant)
{
    gint retval;

    retval = IBUS_SERIALIZABLE_CLASS (ibus_property_parent_class)->deserialize ((IBusSerializable *) prop, variant);
    g_return_val_if_fail (retval, 0);

    ibus_g_variant_get_child_string (variant, retval++, &prop->priv->key);
    g_variant_get_child (variant, retval++, "u", &prop->priv->type);

    GVariant *subvar = g_variant_get_child_value (variant, retval++);
    if (prop->priv->label) {
        g_object_unref (prop->priv->label);
    }
    prop->priv->label = IBUS_TEXT (ibus_serializable_deserialize (subvar));
    g_object_ref_sink (prop->priv->label);
    g_variant_unref (subvar);

    ibus_g_variant_get_child_string (variant, retval++, &prop->priv->icon);

    subvar = g_variant_get_child_value (variant, retval++);
    if (prop->priv->tooltip) {
        g_object_unref (prop->priv->tooltip);
    }
    prop->priv->tooltip = IBUS_TEXT (ibus_serializable_deserialize (subvar));
    g_object_ref_sink (prop->priv->tooltip);
    g_variant_unref (subvar);

    g_variant_get_child (variant, retval++, "b", &prop->priv->sensitive);
    g_variant_get_child (variant, retval++, "b", &prop->priv->visible);
    g_variant_get_child (variant, retval++, "u", &prop->priv->state);

    subvar = g_variant_get_child_value (variant, retval++);
    if (prop->priv->sub_props != NULL) {
        g_object_unref (prop->priv->sub_props);
    }
    prop->priv->sub_props = IBUS_PROP_LIST (ibus_serializable_deserialize (subvar));
    g_object_ref_sink (prop->priv->sub_props);
    g_variant_unref (subvar);

    /* Keep the serialized order for the compatibility when add new members. */
    subvar = g_variant_get_child_value (variant, retval++);
    if (prop->priv->symbol) {
        g_object_unref (prop->priv->symbol);
    }
    prop->priv->symbol = IBUS_TEXT (ibus_serializable_deserialize (subvar));
    g_object_ref_sink (prop->priv->symbol);
    g_variant_unref (subvar);

    return retval;
}

static gboolean
ibus_property_copy (IBusProperty       *dest,
                    const IBusProperty *src)
{
    gboolean retval;

    retval = IBUS_SERIALIZABLE_CLASS (ibus_property_parent_class)->copy ((IBusSerializable *) dest, (IBusSerializable *) src);
    g_return_val_if_fail (retval, FALSE);

    g_return_val_if_fail (IBUS_IS_PROPERTY (dest), FALSE);
    g_return_val_if_fail (IBUS_IS_PROPERTY (src), FALSE);

    dest->priv->key = g_strdup (src->priv->key);
    dest->priv->icon = g_strdup (src->priv->icon);
    if (src->priv->label) {
        dest->priv->label = (IBusText *) ibus_serializable_copy ((IBusSerializable *) src->priv->label);
    }
    else
        dest->priv->label = ibus_text_new_from_static_string ("");
    if (src->priv->symbol) {
        dest->priv->symbol = (IBusText *) ibus_serializable_copy ((IBusSerializable *) src->priv->symbol);
    }
    else
        dest->priv->symbol = ibus_text_new_from_static_string ("");
    if (src->priv->tooltip) {
        dest->priv->tooltip = (IBusText *) ibus_serializable_copy ((IBusSerializable *) src->priv->tooltip);
    }
    else
        dest->priv->tooltip = ibus_text_new_from_static_string ("");

    dest->priv->sensitive = src->priv->sensitive;
    dest->priv->visible = src->priv->visible;
    dest->priv->type = src->priv->type;
    dest->priv->state = src->priv->state;

    dest->priv->sub_props = (IBusPropList *) ibus_serializable_copy ((IBusSerializable *) src->priv->sub_props);

    return TRUE;
}

IBusProperty *
ibus_property_new (const gchar   *key,
                   IBusPropType   type,
                   IBusText      *label,
                   const gchar   *icon,
                   IBusText      *tooltip,
                   gboolean       sensitive,
                   gboolean       visible,
                   IBusPropState  state,
                   IBusPropList  *props)
{
    g_return_val_if_fail (key != NULL, NULL);
    g_return_val_if_fail (type >= PROP_TYPE_NORMAL &&
                          type <= PROP_TYPE_SEPARATOR,
                          NULL);

    return ibus_property_new_varargs ("key", key,
                                      "prop-type", type,
                                      "label", label,
                                      "icon", icon,
                                      "tooltip", tooltip,
                                      "sensitive", sensitive,
                                      "visible", visible,
                                      "state", state,
                                      "sub-props", props,
                                      NULL);
}

IBusProperty *
ibus_property_new_varargs (const gchar *first_property_name, ...)
{
    va_list var_args;
    IBusProperty *prop;

    g_assert (first_property_name);

    va_start (var_args, first_property_name);
    prop = (IBusProperty *) g_object_new_valist (IBUS_TYPE_PROPERTY,
                                                 first_property_name,
                                                 var_args);
    va_end (var_args);

    g_assert (prop->priv->key);
    g_assert (prop->priv->type >= PROP_TYPE_NORMAL &&
              prop->priv->type <= PROP_TYPE_SEPARATOR);

    return prop;
}

#define IBUS_PROPERTY_GET_FIELD(field, return_type) \
return_type                                                             \
ibus_property_get_ ## field (IBusProperty *prop)                        \
{                                                                       \
    return prop->priv->field;                                           \
}

IBUS_PROPERTY_GET_FIELD (key, const gchar *)
IBUS_PROPERTY_GET_FIELD (icon, const gchar *)
IBUS_PROPERTY_GET_FIELD (label, IBusText *)
IBUS_PROPERTY_GET_FIELD (symbol, IBusText *)
IBUS_PROPERTY_GET_FIELD (tooltip, IBusText *)
IBUS_PROPERTY_GET_FIELD (sensitive, gboolean)
IBUS_PROPERTY_GET_FIELD (visible, gboolean)
IBUS_PROPERTY_GET_FIELD (state, IBusPropState)
IBUS_PROPERTY_GET_FIELD (sub_props, IBusPropList *)
#undef IBUS_PROPERTY_GET_FIELD

/* ibus_property_get_type() exists */
IBusPropType
ibus_property_get_prop_type (IBusProperty *prop)
{
    g_assert (IBUS_IS_PROPERTY (prop));
    return prop->priv->type;
}

void
ibus_property_set_label (IBusProperty *prop,
                         IBusText     *label)
{
    g_assert (IBUS_IS_PROPERTY (prop));
    g_return_if_fail (label == NULL || IBUS_IS_TEXT (label));

    if (prop->priv->label) {
        g_object_unref (prop->priv->label);
    }

    if (label == NULL) {
        prop->priv->label = ibus_text_new_from_static_string ("");
    }
    else {
        prop->priv->label = label;
    }

    g_object_ref_sink (prop->priv->label);
}

void
ibus_property_set_symbol (IBusProperty *prop,
                          IBusText     *symbol)
{
    g_assert (IBUS_IS_PROPERTY (prop));
    g_return_if_fail (symbol == NULL || IBUS_IS_TEXT (symbol));

    if (prop->priv->symbol) {
        g_object_unref (prop->priv->symbol);
    }

    if (symbol == NULL) {
        prop->priv->symbol = ibus_text_new_from_static_string ("");
    }
    else {
        prop->priv->symbol = symbol;
    }

    g_object_ref_sink (prop->priv->symbol);
}

void
ibus_property_set_icon (IBusProperty *prop,
                        const gchar  *icon)
{
    g_assert (IBUS_IS_PROPERTY (prop));

    g_free (prop->priv->icon);
    prop->priv->icon = g_strdup (icon != NULL ? icon : "");
}

void
ibus_property_set_tooltip (IBusProperty *prop,
                           IBusText     *tooltip)
{
    g_assert (IBUS_IS_PROPERTY (prop));
    g_assert (tooltip == NULL || IBUS_IS_TEXT (tooltip));

    if (prop->priv->tooltip) {
        g_object_unref (prop->priv->tooltip);
    }

    if (tooltip == NULL) {
        prop->priv->tooltip = ibus_text_new_from_static_string ("");
    }
    else {
        prop->priv->tooltip = tooltip;
    }

    g_object_ref_sink (prop->priv->tooltip);
}

void
ibus_property_set_sensitive (IBusProperty *prop,
                             gboolean      sensitive)
{
    g_assert (IBUS_IS_PROPERTY (prop));
    prop->priv->sensitive = sensitive;
}

void
ibus_property_set_visible (IBusProperty *prop,
                           gboolean      visible)
{
    g_assert (IBUS_IS_PROPERTY (prop));
    prop->priv->visible = visible;
}

void
ibus_property_set_state (IBusProperty  *prop,
                         IBusPropState  state)
{
    g_assert (IBUS_IS_PROPERTY (prop));
    g_assert (state == PROP_STATE_UNCHECKED ||
              state == PROP_STATE_CHECKED ||
              state == PROP_STATE_INCONSISTENT);

    prop->priv->state = state;
}

void
ibus_property_set_sub_props (IBusProperty *prop,
                             IBusPropList *prop_list)
{
    g_assert (IBUS_IS_PROPERTY (prop));
    g_assert (IBUS_IS_PROP_LIST (prop_list) || prop_list == NULL);

    IBusPropertyPrivate *priv = prop->priv;

    if (priv->sub_props) {
        g_object_unref (priv->sub_props);
    }

    if (prop_list) {
        priv->sub_props = prop_list;
        g_object_ref_sink (prop_list);
    }
    else {
        priv->sub_props = ibus_prop_list_new ();
        g_object_ref_sink (priv->sub_props);
    }
}

gboolean
ibus_property_update (IBusProperty *prop,
                      IBusProperty *prop_update)
{
    g_assert (IBUS_IS_PROPERTY (prop));
    g_assert (IBUS_IS_PROPERTY (prop_update));

    IBusPropertyPrivate *priv = prop->priv;
    IBusPropertyPrivate *priv_update = prop_update->priv;

    if (g_strcmp0 (priv->key, priv_update->key) != 0) {
        return ibus_prop_list_update_property (priv->sub_props, prop_update);
    }

    /* Do not support update prop type */
    g_assert (priv->type == priv_update->type);

    ibus_property_set_icon (prop, ibus_property_get_icon (prop_update));
    ibus_property_set_label (prop, ibus_property_get_label (prop_update));
    ibus_property_set_symbol (prop, ibus_property_get_symbol (prop_update));
    ibus_property_set_tooltip (prop, ibus_property_get_tooltip (prop_update));
    ibus_property_set_visible (prop, ibus_property_get_visible (prop_update));
    ibus_property_set_state (prop, ibus_property_get_state (prop_update));
    ibus_property_set_sensitive (prop, ibus_property_get_sensitive (prop_update));

    /* Do not support update sub props */

    return TRUE;
}


