/* -*- mode: C; c-basic-offset: 4; indent-tabs-mode: nil; -*- */
/* vim:set et sts=4: */
/* IBus - The Input Bus
 * Copyright (C) 2008-2013 Peng Huang <shawn.p.huang@gmail.com>
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
#include "ibusattrlist.h"
#include "ibusattrlistprivate.h"
#include "ibuserror.h"

/* functions prototype */
static void         ibus_attr_list_destroy      (IBusAttrList           *attr_list);
static gboolean     ibus_attr_list_serialize    (IBusAttrList           *attr_list,
                                                 GVariantBuilder        *builder);
static gint         ibus_attr_list_deserialize  (IBusAttrList           *attr_list,
                                                 GVariant               *variant);
static gboolean     ibus_attr_list_copy         (IBusAttrList           *dest,
                                                 const IBusAttrList     *src);

G_DEFINE_TYPE (IBusAttrList, ibus_attr_list, IBUS_TYPE_SERIALIZABLE)

static void
ibus_attr_list_class_init (IBusAttrListClass *class)
{
    IBusObjectClass *object_class = IBUS_OBJECT_CLASS (class);
    IBusSerializableClass *serializable_class = IBUS_SERIALIZABLE_CLASS (class);

    object_class->destroy = (IBusObjectDestroyFunc) ibus_attr_list_destroy;

    serializable_class->serialize   = (IBusSerializableSerializeFunc) ibus_attr_list_serialize;
    serializable_class->deserialize = (IBusSerializableDeserializeFunc) ibus_attr_list_deserialize;
    serializable_class->copy        = (IBusSerializableCopyFunc) ibus_attr_list_copy;
}

static void
ibus_attr_list_init (IBusAttrList *attr_list)
{
    attr_list->attributes = g_array_new (TRUE, TRUE, sizeof (IBusAttribute *));
}

static void
ibus_attr_list_destroy (IBusAttrList *attr_list)
{
    g_assert (IBUS_IS_ATTR_LIST (attr_list));

    gint i;
    for (i = 0;; i++) {
        IBusAttribute *attr;

        attr = ibus_attr_list_get (attr_list, i);
        if (attr == NULL)
            break;

        g_object_unref (attr);
    }

    g_array_free (attr_list->attributes, TRUE);

    IBUS_OBJECT_CLASS (ibus_attr_list_parent_class)->destroy ((IBusObject *)attr_list);
}

static gboolean
ibus_attr_list_serialize (IBusAttrList    *attr_list,
                          GVariantBuilder *builder)
{
    gboolean retval;
    guint i;

    retval = IBUS_SERIALIZABLE_CLASS (ibus_attr_list_parent_class)->serialize ((IBusSerializable *)attr_list, builder);
    g_return_val_if_fail (retval, FALSE);

    g_return_val_if_fail (IBUS_IS_ATTR_LIST (attr_list), FALSE);

    GVariantBuilder array;
    g_variant_builder_init (&array, G_VARIANT_TYPE ("av"));

    for (i = 0;; i++) {
        IBusAttribute *attr;
        attr = ibus_attr_list_get (attr_list, i);
        if (attr == NULL)
            break;
        g_variant_builder_open (&array, G_VARIANT_TYPE_VARIANT);
        g_variant_builder_add_value (
                &array,
                ibus_serializable_serialize ((IBusSerializable *)attr));
        g_variant_builder_close (&array);
    }
    g_variant_builder_add (builder, "av", &array);

    return TRUE;
}

static gint
ibus_attr_list_deserialize (IBusAttrList    *attr_list,
                            GVariant        *variant)
{
    gint retval = IBUS_SERIALIZABLE_CLASS (ibus_attr_list_parent_class)->deserialize ((IBusSerializable *)attr_list, variant);
    g_return_val_if_fail (retval, 0);

    GVariantIter *iter = NULL;
    g_variant_get_child (variant, retval++, "av", &iter);
    GVariant *var;
    while (g_variant_iter_loop (iter, "v", &var)) {
        IBusAttribute *attr = IBUS_ATTRIBUTE (ibus_serializable_deserialize (var));
        ibus_attr_list_append (attr_list, attr);
    }
    g_variant_iter_free (iter);

    return retval;
}



static gboolean
ibus_attr_list_copy (IBusAttrList       *dest,
                     const IBusAttrList *src)
{
    gboolean retval;

    retval = IBUS_SERIALIZABLE_CLASS (ibus_attr_list_parent_class)->copy ((IBusSerializable *)dest,
                                 (IBusSerializable *)src);
    g_return_val_if_fail (retval, FALSE);

    g_return_val_if_fail (IBUS_IS_ATTR_LIST (dest), FALSE);
    g_return_val_if_fail (IBUS_IS_ATTR_LIST (src), FALSE);

    gint i;
    for (i = 0; ; i++) {
        IBusAttribute *attr = ibus_attr_list_get ((IBusAttrList *)src, i);
        if (attr == NULL) {
            break;
        }

        attr = (IBusAttribute *) ibus_serializable_copy ((IBusSerializable *) attr);
        if (attr == NULL) {
            g_warning ("can not copy attribute");
            continue;
        }

        ibus_attr_list_append (dest, attr);
    }
    return TRUE;
}

IBusAttrList *
ibus_attr_list_new ()
{
    IBusAttrList *attr_list;
    attr_list = g_object_new (IBUS_TYPE_ATTR_LIST, NULL);
    return attr_list;
}

void
ibus_attr_list_append (IBusAttrList  *attr_list,
                       IBusAttribute *attr)
{
    g_assert (IBUS_IS_ATTR_LIST (attr_list));
    g_assert (IBUS_IS_ATTRIBUTE (attr));

    g_object_ref_sink (attr);
    g_array_append_val (attr_list->attributes, attr);
}

IBusAttribute *
ibus_attr_list_get (IBusAttrList *attr_list,
                    guint         index)
{
    g_assert (IBUS_IS_ATTR_LIST (attr_list));
    IBusAttribute *attr = NULL;

    if (index < attr_list->attributes->len) {
        attr = g_array_index (attr_list->attributes, IBusAttribute *, index);
    }

    return attr;
}


static gboolean
ibus_attr_list_has_attribution (IBusAttrList  *attr_list,
                                IBusAttribute *attr)
{
    guint i;
    IBusAttribute *_attr;

    g_assert (IBUS_IS_ATTR_LIST (attr_list));
    g_assert (IBUS_IS_ATTRIBUTE (attr));
    for (i = 0; i < attr_list->attributes->len; ++i) {
        _attr = g_array_index (attr_list->attributes, IBusAttribute *, i);
        /* attr->parent would be same in this case? */
        if (attr->type == _attr->type &&
            attr->value == _attr->value &&
            attr->start_index == _attr->start_index &&
            attr->end_index == _attr->end_index) {
            return TRUE;
        }
    }
    return FALSE;
}


static guint
_foreground_rgba_to_hint (guint    rgba,
                          GError **error)
{
    switch (rgba) {
    case 0x7F7F7F: /* typing-booster */
        return IBUS_ATTR_PREEDIT_PREDICTION;
    case 0xF90F0F: /* table */
        return IBUS_ATTR_PREEDIT_PREFIX;
    case 0x1EDC1A: /* table */
        return IBUS_ATTR_PREEDIT_SUFFIX;
    case 0xA40000: /* typing-booster, table */
        return IBUS_ATTR_PREEDIT_ERROR_SPELLING;
    case 0xFF00FF: /* typing-booster */
        return IBUS_ATTR_PREEDIT_ERROR_COMPOSE;
    case 0x0: /* Japanese */
    case 0xFF000000:
        return IBUS_ATTR_PREEDIT_SELECTION;
    case 0xFFFFFF: /* Hangul */
    case 0xFFFFFFFF:
        return IBUS_ATTR_PREEDIT_SELECTION;
    default: /* Custom */
        if (error && *error == NULL) {
            g_set_error (error, IBUS_ERROR, IBUS_ERROR_FAILED,
            "%s does not support foreground RGBA value %X to convert hint",
            G_STRFUNC, rgba);
        }
        return IBUS_ATTR_PREEDIT_NONE;
    }
}


static guint
_background_rgba_to_hint (guint    rgba,
                          GError **error)
{
    switch (rgba) {
    case 0xC8C8F0: /* Japanese */
    case 0xFFC8C8F0:
        return IBUS_ATTR_PREEDIT_SELECTION;
    case 0x0: /* Hangul */
    case 0xFF000000:
        return IBUS_ATTR_PREEDIT_SELECTION;
    default:; /* Custom */
        if (error && *error == NULL) {
            g_set_error (error, IBUS_ERROR, IBUS_ERROR_FAILED,
            "%s does not support background RGBA value %X to convert hint",
            G_STRFUNC, rgba);
        }
        return IBUS_ATTR_PREEDIT_NONE;
    }
}


typedef struct _AttrTypeAndValue {
    guint type;
    guint value;
} AttrTypeAndValue;


static AttrTypeAndValue *
_hint_to_rgba (guint           hint,
               guint          *new_num,
               const IBusRGBA *selected_fg,
               const IBusRGBA *selected_bg,
               GError        **error)
{
    AttrTypeAndValue *values = NULL;

    g_assert (new_num);
    switch (hint) {
    case IBUS_ATTR_PREEDIT_NONE:
        if (error && *error == NULL) {
            g_set_error (error, IBUS_ERROR, IBUS_ERROR_FAILED,
            "%s should not receive the hint IBUS_ATTR_PREEDIT_NONE",
            G_STRFUNC);
        }
        *new_num = 1;
        values = g_new0 (AttrTypeAndValue, *new_num);
        break;
    case IBUS_ATTR_PREEDIT_WHOLE:
        *new_num = 1;
        values = g_new0 (AttrTypeAndValue, *new_num);
        values[0].type = IBUS_ATTR_TYPE_UNDERLINE;
        values[0].value = IBUS_ATTR_UNDERLINE_SINGLE;
        break;
    case IBUS_ATTR_PREEDIT_SELECTION:
        *new_num = 2;
        values = g_new0 (AttrTypeAndValue, *new_num);
        values[0].type = IBUS_ATTR_TYPE_FOREGROUND;
        values[1].type = IBUS_ATTR_TYPE_BACKGROUND;
        if (selected_fg && selected_bg) {
            values[0].value = ((int)(selected_fg->alpha * 0xff) & 0xff) << 24 |\
                              ((int)(selected_fg->red   * 0xff) & 0xff) << 16 |\
                              ((int)(selected_fg->green * 0xff) & 0xff) << 8  |\
                              ((int)(selected_fg->blue  * 0xff) & 0xff);
            values[1].value = ((int)(selected_bg->alpha * 0xff) & 0xff) << 24 |\
                              ((int)(selected_bg->red   * 0xff) & 0xff) << 16 |\
                              ((int)(selected_bg->green * 0xff) & 0xff) << 8  |\
                              ((int)(selected_bg->blue  * 0xff) & 0xff);
        } else {
            /* Hangul */
            values[0].value = 0xFFFFFFFF;
            values[1].value = 0xFF000000;
        }
        break;
    case IBUS_ATTR_PREEDIT_PREDICTION:
        *new_num = 1;
        values = g_new0 (AttrTypeAndValue, *new_num);
        values[0].type = IBUS_ATTR_TYPE_FOREGROUND;
        values[0].value = 0x7F7F7F; /* typing-booster */
        break;
    case IBUS_ATTR_PREEDIT_PREFIX:
        *new_num = 1;
        values = g_new0 (AttrTypeAndValue, *new_num);
        values[0].type = IBUS_ATTR_TYPE_FOREGROUND;
        values[0].value = 0xF90F0F; /* table */
        break;
    case IBUS_ATTR_PREEDIT_SUFFIX:
        *new_num = 1;
        values = g_new0 (AttrTypeAndValue, *new_num);
        values[0].type = IBUS_ATTR_TYPE_FOREGROUND;
        values[0].value = 0x1EDC1A; /* table */
        break;
    case IBUS_ATTR_PREEDIT_ERROR_SPELLING:
        *new_num = 1;
        values = g_new0 (AttrTypeAndValue, *new_num);
        values[0].type = IBUS_ATTR_TYPE_FOREGROUND;
        values[0].value = 0xA40000; /* typing-booster, table */
        break;
    case IBUS_ATTR_PREEDIT_ERROR_COMPOSE:
        *new_num = 1;
        values = g_new0 (AttrTypeAndValue, *new_num);
        values[0].type = IBUS_ATTR_TYPE_FOREGROUND;
        values[0].value = 0xFF00FF; /* typing-booster */
        break;
    default:
        if (error && *error == NULL) {
            g_set_error (error, IBUS_ERROR, IBUS_ERROR_FAILED,
            "%s does not support the hint %u",
            G_STRFUNC, hint);
        }
        *new_num = 1;
        values = g_new0 (AttrTypeAndValue, *new_num);
    }
    return values;
}


IBusAttrList *
ibus_attr_list_copy_format_to_rgba (IBusAttrList   *attr_list,
                                    const IBusRGBA *selected_fg,
                                    const IBusRGBA *selected_bg,
                                    GError         **error)
{
    IBusAttrList *new_attr_list;
    guint i, j;

    if (error)
        *error = NULL;
    g_return_val_if_fail (IBUS_IS_ATTR_LIST (attr_list), NULL);

    if (attr_list->attributes->len == 0)
        return g_object_ref (attr_list);

    new_attr_list = ibus_attr_list_new ();
    g_return_val_if_fail (IBUS_IS_ATTR_LIST (new_attr_list), NULL);
    for (i = 0; i < attr_list->attributes->len; ++i) {
        IBusAttribute *attr = g_array_index (attr_list->attributes,
                                             IBusAttribute *, i);
        guint new_num = 0;
        AttrTypeAndValue *new_values = NULL;

        switch (attr->type) {
        case IBUS_ATTR_TYPE_UNDERLINE:
        case IBUS_ATTR_TYPE_FOREGROUND:
        case IBUS_ATTR_TYPE_BACKGROUND:
            new_num = 1;
            new_values = g_new0 (AttrTypeAndValue, new_num);
            new_values->type = attr->type;
            new_values->value = attr->value;
            break;
        case IBUS_ATTR_TYPE_HINT:
            new_values = _hint_to_rgba (attr->value, &new_num,
                                        selected_fg, selected_bg,
                                        error);
            break;
        default:
            if (error && *error == NULL) {
                g_set_error (error, IBUS_ERROR, IBUS_ERROR_FAILED,
                             "%s does not support attribution type %u at %u",
                             G_STRFUNC, attr->type, i);
            }
        }
        for (j = 0; j < new_num; ++j) {
            IBusAttribute *new_attr;
            new_attr = IBUS_ATTRIBUTE (g_object_new (IBUS_TYPE_ATTRIBUTE,
                                                     NULL));
            if (!IBUS_IS_ATTRIBUTE (new_attr)) {
                g_warning ("Failed to allocate IBusAttribute");
                g_free (new_values);
                return new_attr_list;
            }
            new_attr->type = new_values[j].type;
            new_attr->value = new_values[j].value;
            new_attr->start_index = attr->start_index;
            new_attr->end_index = attr->end_index;
            ibus_attr_list_append (new_attr_list, new_attr);
        }
        g_free (new_values);
    }
    return new_attr_list;
}


IBusAttrList *
ibus_attr_list_copy_format_to_hint (IBusAttrList *attr_list,
                                    GError      **error)
{
    IBusAttrList *new_attr_list;
    guint i;
    IBusAttribute *attr, *new_attr;

    if (error)
        *error = NULL;
    g_return_val_if_fail (IBUS_IS_ATTR_LIST (attr_list), NULL);

    if (attr_list->attributes->len == 0)
        return g_object_ref (attr_list);

    new_attr_list = ibus_attr_list_new ();
    g_return_val_if_fail (IBUS_IS_ATTR_LIST (new_attr_list), NULL);
    for (i = 0; i < attr_list->attributes->len; ++i) {
        attr = g_array_index (attr_list->attributes, IBusAttribute *, i);
        new_attr = IBUS_ATTRIBUTE (g_object_new (IBUS_TYPE_ATTRIBUTE, NULL));
        g_return_val_if_fail (IBUS_IS_ATTRIBUTE (new_attr), new_attr_list);
        new_attr->start_index = attr->start_index;
        new_attr->end_index = attr->end_index;

        switch (attr->type) {
        case IBUS_ATTR_TYPE_UNDERLINE:
            new_attr->type = IBUS_ATTR_TYPE_HINT;
            new_attr->value = IBUS_ATTR_PREEDIT_WHOLE;
            break;
        case IBUS_ATTR_TYPE_FOREGROUND:
            new_attr->type = IBUS_ATTR_TYPE_HINT;
            new_attr->value = _foreground_rgba_to_hint (attr->value, error);
            break;
        case IBUS_ATTR_TYPE_BACKGROUND:
            new_attr->type = IBUS_ATTR_TYPE_HINT;
            new_attr->value = _background_rgba_to_hint (attr->value, error);
        case IBUS_ATTR_TYPE_HINT:
            new_attr->type = attr->type;
            new_attr->value = attr->value;
            break;
        default:
            if (error && *error == NULL) {
                g_set_error (error, IBUS_ERROR, IBUS_ERROR_FAILED,
                             "%s does not support attribution type %u at %u",
                             G_STRFUNC, attr->type, i);
            }
            new_attr->type = attr->type;
            new_attr->value = attr->value;
        }
        if (ibus_attr_list_has_attribution (new_attr_list, new_attr))
            g_object_unref (new_attr);
        else
            ibus_attr_list_append (new_attr_list, new_attr);
    }
    return new_attr_list;
}
