/* -*- mode: C; c-basic-offset: 4; indent-tabs-mode: nil; -*- */
/* vim:set et sts=4: */
/* bus - The Input Bus
 * Copyright (C) 2008-2015 Peng Huang <shawn.p.huang@gmail.com>
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
#include <stdlib.h>
#include "ibusenginedesc.h"
#include "ibusinternal.h"
#include "ibusxml.h"

enum {
    LAST_SIGNAL,
};

enum {
    PROP_0 = 0,
    PROP_NAME,
    PROP_LONGNAME,
    PROP_DESCRIPTION,
    PROP_LANGUAGE,
    PROP_LICENSE,
    PROP_AUTHOR,
    PROP_ICON,
    PROP_LAYOUT,
    PROP_LAYOUT_VARIANT,
    PROP_LAYOUT_OPTION,
    PROP_RANK,
    PROP_HOTKEYS,
    PROP_SYMBOL,
    PROP_SETUP,
    PROP_VERSION,
    PROP_TEXTDOMAIN,
    PROP_ICON_PROP_KEY
};


/* IBusEngineDescPriv */
struct _IBusEngineDescPrivate {
    gchar      *name;
    gchar      *longname;
    gchar      *description;
    gchar      *language;
    gchar      *license;
    gchar      *author;
    gchar      *icon;
    gchar      *layout;
    gchar      *layout_variant;
    gchar      *layout_option;
    guint       rank;
    gchar      *hotkeys;
    gchar      *symbol;
    gchar      *setup;
    gchar      *version;
    gchar      *textdomain;
    gchar      *icon_prop_key;
};

#define IBUS_ENGINE_DESC_GET_PRIVATE(o)  \
   ((IBusEngineDescPrivate *)ibus_engine_desc_get_instance_private (o))

// static guint            _signals[LAST_SIGNAL] = { 0 };

/* functions prototype */
static void         ibus_engine_desc_set_property   (IBusEngineDesc         *desc,
                                                     guint                   prop_id,
                                                     const GValue           *value,
                                                     GParamSpec             *pspec);
static void         ibus_engine_desc_get_property   (IBusEngineDesc         *desc,
                                                     guint                   prop_id,
                                                     GValue                 *value,
                                                     GParamSpec             *pspec);
static void         ibus_engine_desc_destroy        (IBusEngineDesc         *desc);
static gboolean     ibus_engine_desc_serialize      (IBusEngineDesc         *desc,
                                                     GVariantBuilder        *builder);
static gint         ibus_engine_desc_deserialize    (IBusEngineDesc         *desc,
                                                     GVariant               *variant);
static gboolean     ibus_engine_desc_copy           (IBusEngineDesc         *dest,
                                                     const IBusEngineDesc   *src);
static gboolean     ibus_engine_desc_parse_xml_node (IBusEngineDesc         *desc,
                                                     XMLNode                *node);

G_DEFINE_TYPE_WITH_PRIVATE (IBusEngineDesc,
                            ibus_engine_desc,
                            IBUS_TYPE_SERIALIZABLE)


static void
ibus_engine_desc_class_init (IBusEngineDescClass *class)
{
    GObjectClass *gobject_class = G_OBJECT_CLASS (class);
    IBusObjectClass *object_class = IBUS_OBJECT_CLASS (class);
    IBusSerializableClass *serializable_class = IBUS_SERIALIZABLE_CLASS (class);

    gobject_class->set_property = (GObjectSetPropertyFunc) ibus_engine_desc_set_property;
    gobject_class->get_property = (GObjectGetPropertyFunc) ibus_engine_desc_get_property;
    object_class->destroy = (IBusObjectDestroyFunc) ibus_engine_desc_destroy;

    serializable_class->serialize   = (IBusSerializableSerializeFunc) ibus_engine_desc_serialize;
    serializable_class->deserialize = (IBusSerializableDeserializeFunc) ibus_engine_desc_deserialize;
    serializable_class->copy        = (IBusSerializableCopyFunc) ibus_engine_desc_copy;

    /* install properties */
    /**
     * IBusEngineDesc:name:
     *
     * The name of engine description
     */
    g_object_class_install_property (gobject_class,
                    PROP_NAME,
                    g_param_spec_string ("name",
                        "description name",
                        "The name of engine description",
                        NULL,
                        G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));

    /**
     * IBusEngineDesc:longname:
     *
     * The longname of engine description
     */
    g_object_class_install_property (gobject_class,
                    PROP_LONGNAME,
                    g_param_spec_string ("longname",
                        "description longname",
                        "The longname of engine description",
                        "",
                        G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));

    /**
     * IBusEngineDesc:description:
     *
     * The description of engine description
     */
    g_object_class_install_property (gobject_class,
                    PROP_DESCRIPTION,
                    g_param_spec_string ("description",
                        "description description",
                        "The description of engine description",
                        "",
                        G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));

    /**
     * IBusEngineDesc:language:
     *
     * The language of engine description
     */
    g_object_class_install_property (gobject_class,
                    PROP_LANGUAGE,
                    g_param_spec_string ("language",
                        "description language",
                        "The language of engine description",
                        "",
                        G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));

    /**
     * IBusEngineDesc:license:
     *
     * The license of engine description
     */
    g_object_class_install_property (gobject_class,
                    PROP_LICENSE,
                    g_param_spec_string ("license",
                        "description license",
                        "The license of engine description",
                        "",
                        G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));

    /**
     * IBusEngineDesc:author:
     *
     * The author of engine description
     */
    g_object_class_install_property (gobject_class,
                    PROP_AUTHOR,
                    g_param_spec_string ("author",
                        "description author",
                        "The author of engine description",
                        "",
                        G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));

    /**
     * IBusEngineDesc:icon:
     *
     * The icon of engine description
     */
    g_object_class_install_property (gobject_class,
                    PROP_ICON,
                    g_param_spec_string ("icon",
                        "description icon",
                        "The icon of engine description",
                        "ibus-engine",
                        G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));

    /**
     * IBusEngineDesc:layout:
     *
     * The layout of engine description
     */
    g_object_class_install_property (gobject_class,
                    PROP_LAYOUT,
                    g_param_spec_string ("layout",
                        "description layout",
                        "The layout of engine description",
                        "us",
                        G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));

    /**
     * IBusEngineDesc:layout-variant:
     *
     * The keyboard variant of engine description
     */
    g_object_class_install_property (gobject_class,
                    PROP_LAYOUT_VARIANT,
                    g_param_spec_string ("layout-variant",
                        "description keyboard variant",
                        "The keyboard variant of engine description",
                        "",
                        G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));

    /**
     * IBusEngineDesc:layout-option:
     *
     * The keyboard option of engine description
     */
    g_object_class_install_property (gobject_class,
                    PROP_LAYOUT_OPTION,
                    g_param_spec_string ("layout-option",
                        "description keyboard option",
                        "The keyboard option of engine description",
                        "",
                        G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));

    /**
     * IBusEngineDesc:rank:
     *
     * The rank of engine description
     */
    g_object_class_install_property (gobject_class,
                    PROP_RANK,
                    g_param_spec_uint ("rank",
                        "description rank",
                        "The rank of engine description",
                        0,
                        G_MAXUINT,
                        0,
                        G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));

    /**
     * IBusEngineDesc:hotkeys:
     *
     * The hotkeys of engine description
     */
    g_object_class_install_property (gobject_class,
                    PROP_HOTKEYS,
                    g_param_spec_string ("hotkeys",
                        "description hotkeys",
                        "The hotkeys of engine description",
                        "",
                        G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));

    /**
     * IBusEngineDesc:symbol:
     *
     * The symbol chars of engine description instead of icon image
     */
    g_object_class_install_property (gobject_class,
                    PROP_SYMBOL,
                    g_param_spec_string ("symbol",
                        "description symbol",
                        "The icon symbol chars of engine description",
                        "",
                        G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));

    /**
     * IBusEngineDesc:setup:
     *
     * The exec lists of the engine setup command
     */
    g_object_class_install_property (gobject_class,
                    PROP_SETUP,
                    g_param_spec_string ("setup",
                        "setup args",
                        "The exec lists of the engine setup command",
                        "",
                        G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));

    /**
     * IBusEngineDesc:version:
     *
     * The version number of engine description
     */
    g_object_class_install_property (gobject_class,
                    PROP_VERSION,
                    g_param_spec_string ("version",
                        "version number",
                        "The version number of engine description",
                        "",
                        G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));

    /**
     * IBusEngineDesc:textdomain:
     *
     * The textdomain of engine description
     */
    g_object_class_install_property (gobject_class,
                    PROP_TEXTDOMAIN,
                    g_param_spec_string ("textdomain",
                        "textdomain",
                        "The textdomain of engine description",
                        "",
                        G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));

    /**
     * IBusEngineDesc:icon-prop-key:
     *
     * The key of IBusProperty to change panel icon dynamically.
     */
    g_object_class_install_property (gobject_class,
                    PROP_ICON_PROP_KEY,
                    g_param_spec_string ("icon-prop-key",
                        "icon property key",
                        "The key of IBusProperty for the dynamic panel icon",
                        "",
                        G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));
}

static void
ibus_engine_desc_init (IBusEngineDesc *desc)
{
    desc->priv = IBUS_ENGINE_DESC_GET_PRIVATE (desc);

    desc->priv->name = NULL;
    desc->priv->longname = NULL;
    desc->priv->description = NULL;
    desc->priv->language = NULL;
    desc->priv->license = NULL;
    desc->priv->author = NULL;
    desc->priv->icon = NULL;
    desc->priv->layout = NULL;
    desc->priv->layout_variant = NULL;
    desc->priv->layout_option = NULL;
    desc->priv->rank = 0;
    desc->priv->hotkeys = NULL;
    desc->priv->symbol = NULL;
    desc->priv->setup = NULL;
    desc->priv->version = NULL;
    desc->priv->textdomain = NULL;
    desc->priv->icon_prop_key = NULL;
}

static void
ibus_engine_desc_destroy (IBusEngineDesc *desc)
{
    g_free (desc->priv->name);
    g_free (desc->priv->longname);
    g_free (desc->priv->description);
    g_free (desc->priv->language);
    g_free (desc->priv->license);
    g_free (desc->priv->author);
    g_free (desc->priv->icon);
    g_free (desc->priv->layout);
    g_free (desc->priv->layout_variant);
    g_free (desc->priv->layout_option);
    g_free (desc->priv->hotkeys);
    g_free (desc->priv->symbol);
    g_free (desc->priv->setup);
    g_free (desc->priv->version);
    g_free (desc->priv->textdomain);
    g_free (desc->priv->icon_prop_key);

    IBUS_OBJECT_CLASS (ibus_engine_desc_parent_class)->destroy (IBUS_OBJECT (desc));
}

static void
ibus_engine_desc_set_property (IBusEngineDesc *desc,
                               guint           prop_id,
                               const GValue   *value,
                               GParamSpec     *pspec)
{
    switch (prop_id) {
    case PROP_NAME:
        g_assert (desc->priv->name == NULL);
        desc->priv->name = g_value_dup_string (value);
        break;
    case PROP_LONGNAME:
        g_assert (desc->priv->longname == NULL);
        desc->priv->longname = g_value_dup_string (value);
        break;
    case PROP_DESCRIPTION:
        g_assert (desc->priv->description == NULL);
        desc->priv->description  = g_value_dup_string (value);
        break;
    case PROP_LANGUAGE:
        g_assert (desc->priv->language == NULL);
        desc->priv->language = g_value_dup_string (value);
        break;
    case PROP_LICENSE:
        g_assert (desc->priv->license == NULL);
        desc->priv->license = g_value_dup_string (value);
        break;
    case PROP_AUTHOR:
        g_assert (desc->priv->author == NULL);
        desc->priv->author = g_value_dup_string (value);
        break;
    case PROP_ICON:
        g_assert (desc->priv->icon == NULL);
        desc->priv->icon = g_value_dup_string (value);
        break;
    case PROP_LAYOUT:
        g_assert (desc->priv->layout == NULL);
        desc->priv->layout = g_value_dup_string (value);
        break;
    case PROP_LAYOUT_VARIANT:
        g_assert (desc->priv->layout_variant == NULL);
        desc->priv->layout_variant = g_value_dup_string (value);
        break;
    case PROP_LAYOUT_OPTION:
        g_assert (desc->priv->layout_option == NULL);
        desc->priv->layout_option = g_value_dup_string (value);
        break;
    case PROP_RANK:
        desc->priv->rank = g_value_get_uint (value);
        break;
    case PROP_HOTKEYS:
        g_assert (desc->priv->hotkeys == NULL);
        desc->priv->hotkeys = g_value_dup_string (value);
        break;
    case PROP_SYMBOL:
        g_assert (desc->priv->symbol == NULL);
        desc->priv->symbol = g_value_dup_string (value);
        break;
    case PROP_SETUP:
        g_assert (desc->priv->setup == NULL);
        desc->priv->setup = g_value_dup_string (value);
        break;
    case PROP_VERSION:
        g_assert (desc->priv->version == NULL);
        desc->priv->version = g_value_dup_string (value);
        break;
    case PROP_TEXTDOMAIN:
        g_assert (desc->priv->textdomain == NULL);
        desc->priv->textdomain = g_value_dup_string (value);
        break;
    case PROP_ICON_PROP_KEY:
        g_assert (desc->priv->icon_prop_key == NULL);
        desc->priv->icon_prop_key = g_value_dup_string (value);
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (desc, prop_id, pspec);
    }
}

static void
ibus_engine_desc_get_property (IBusEngineDesc *desc,
                               guint           prop_id,
                               GValue         *value,
                               GParamSpec     *pspec)
{
    switch (prop_id) {
    case PROP_NAME:
        g_value_set_string (value, ibus_engine_desc_get_name (desc));
        break;
    case PROP_LONGNAME:
        g_value_set_string (value, ibus_engine_desc_get_longname (desc));
        break;
    case PROP_DESCRIPTION:
        g_value_set_string (value, ibus_engine_desc_get_description (desc));
        break;
    case PROP_LANGUAGE:
        g_value_set_string (value, ibus_engine_desc_get_language (desc));
        break;
    case PROP_LICENSE:
        g_value_set_string (value, ibus_engine_desc_get_license (desc));
        break;
    case PROP_AUTHOR:
        g_value_set_string (value, ibus_engine_desc_get_author (desc));
        break;
    case PROP_ICON:
        g_value_set_string (value, ibus_engine_desc_get_icon (desc));
        break;
    case PROP_LAYOUT:
        g_value_set_string (value, ibus_engine_desc_get_layout (desc));
        break;
    case PROP_LAYOUT_VARIANT:
        g_value_set_string (value, ibus_engine_desc_get_layout_variant (desc));
        break;
    case PROP_LAYOUT_OPTION:
        g_value_set_string (value, ibus_engine_desc_get_layout_option (desc));
        break;
    case PROP_RANK:
        g_value_set_uint (value, ibus_engine_desc_get_rank (desc));
        break;
    case PROP_HOTKEYS:
        g_value_set_string (value, ibus_engine_desc_get_hotkeys (desc));
        break;
    case PROP_SYMBOL:
        g_value_set_string (value, ibus_engine_desc_get_symbol (desc));
        break;
    case PROP_SETUP:
        g_value_set_string (value, ibus_engine_desc_get_setup (desc));
        break;
    case PROP_VERSION:
        g_value_set_string (value, ibus_engine_desc_get_version (desc));
        break;
    case PROP_TEXTDOMAIN:
        g_value_set_string (value, ibus_engine_desc_get_textdomain (desc));
        break;
    case PROP_ICON_PROP_KEY:
        g_value_set_string (value, ibus_engine_desc_get_icon_prop_key (desc));
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (desc, prop_id, pspec);
    }
}

static gboolean
ibus_engine_desc_serialize (IBusEngineDesc  *desc,
                            GVariantBuilder *builder)
{
    gboolean retval;

    retval = IBUS_SERIALIZABLE_CLASS (ibus_engine_desc_parent_class)->serialize ((IBusSerializable *)desc, builder);
    g_return_val_if_fail (retval, FALSE);
    /* End dict iter */

#define NOTNULL(s) ((s) != NULL ? (s) : "")
    /* If you will add a new property, you can append it at the end and
     * you should not change the serialized order of name, longname,
     * description, ... because the order is also used in other applications
     * likes ibus-qt. */
    g_variant_builder_add (builder, "s", NOTNULL (desc->priv->name));
    g_variant_builder_add (builder, "s", NOTNULL (desc->priv->longname));
    g_variant_builder_add (builder, "s", NOTNULL (desc->priv->description));
    g_variant_builder_add (builder, "s", NOTNULL (desc->priv->language));
    g_variant_builder_add (builder, "s", NOTNULL (desc->priv->license));
    g_variant_builder_add (builder, "s", NOTNULL (desc->priv->author));
    g_variant_builder_add (builder, "s", NOTNULL (desc->priv->icon));
    g_variant_builder_add (builder, "s", NOTNULL (desc->priv->layout));
    g_variant_builder_add (builder, "u", desc->priv->rank);
    /* The serialized order should be kept. */
    g_variant_builder_add (builder, "s", NOTNULL (desc->priv->hotkeys));
    g_variant_builder_add (builder, "s", NOTNULL (desc->priv->symbol));
    g_variant_builder_add (builder, "s", NOTNULL (desc->priv->setup));
    g_variant_builder_add (builder, "s", NOTNULL (desc->priv->layout_variant));
    g_variant_builder_add (builder, "s", NOTNULL (desc->priv->layout_option));
    g_variant_builder_add (builder, "s", NOTNULL (desc->priv->version));
    g_variant_builder_add (builder, "s", NOTNULL (desc->priv->textdomain));
    g_variant_builder_add (builder, "s", NOTNULL (desc->priv->icon_prop_key));
#undef NOTNULL

    return TRUE;
}

static gint
ibus_engine_desc_deserialize (IBusEngineDesc *desc,
                              GVariant       *variant)
{
    gint retval;

    retval = IBUS_SERIALIZABLE_CLASS (ibus_engine_desc_parent_class)->deserialize ((IBusSerializable *)desc, variant);
    g_return_val_if_fail (retval, 0);

    /* If you will add a new property, you can append it at the end and
     * you should not change the serialized order of name, longname,
     * description, ... because the order is also used in other applications
     * likes ibus-qt. */
    ibus_g_variant_get_child_string (variant, retval++,
                                     &desc->priv->name);
    ibus_g_variant_get_child_string (variant, retval++,
                                     &desc->priv->longname);
    ibus_g_variant_get_child_string (variant, retval++,
                                     &desc->priv->description);
    ibus_g_variant_get_child_string (variant, retval++,
                                     &desc->priv->language);
    ibus_g_variant_get_child_string (variant, retval++,
                                     &desc->priv->license);
    ibus_g_variant_get_child_string (variant, retval++,
                                     &desc->priv->author);
    ibus_g_variant_get_child_string (variant, retval++,
                                     &desc->priv->icon);
    ibus_g_variant_get_child_string (variant, retval++,
                                     &desc->priv->layout);
    g_variant_get_child (variant, retval++, "u", &desc->priv->rank);
    /* The serialized order should be kept. */
    ibus_g_variant_get_child_string (variant, retval++,
                                     &desc->priv->hotkeys);
    ibus_g_variant_get_child_string (variant, retval++,
                                     &desc->priv->symbol);
    ibus_g_variant_get_child_string (variant, retval++,
                                     &desc->priv->setup);
    if (g_variant_n_children (variant) < retval + 2)
        return retval;
    ibus_g_variant_get_child_string (variant, retval++,
                                     &desc->priv->layout_variant);
    ibus_g_variant_get_child_string (variant, retval++,
                                     &desc->priv->layout_option);
    if (g_variant_n_children (variant) < retval + 1)
        return retval;
    ibus_g_variant_get_child_string (variant, retval++,
                                     &desc->priv->version);
    if (g_variant_n_children (variant) < retval + 1)
        return retval;
    ibus_g_variant_get_child_string (variant, retval++,
                                     &desc->priv->textdomain);
    if (g_variant_n_children (variant) < retval + 1)
        return retval;
    ibus_g_variant_get_child_string (variant, retval++,
                                     &desc->priv->icon_prop_key);

    return retval;
}

static gboolean
ibus_engine_desc_copy (IBusEngineDesc       *dest,
                       const IBusEngineDesc *src)
{
    gboolean retval;

    retval = IBUS_SERIALIZABLE_CLASS (ibus_engine_desc_parent_class)->copy ((IBusSerializable *)dest,
                                 (IBusSerializable *)src);
    g_return_val_if_fail (retval, FALSE);

    dest->priv->name             = g_strdup (src->priv->name);
    dest->priv->longname         = g_strdup (src->priv->longname);
    dest->priv->description      = g_strdup (src->priv->description);
    dest->priv->language         = g_strdup (src->priv->language);
    dest->priv->license          = g_strdup (src->priv->license);
    dest->priv->author           = g_strdup (src->priv->author);
    dest->priv->icon             = g_strdup (src->priv->icon);
    dest->priv->layout           = g_strdup (src->priv->layout);
    dest->priv->layout_variant   = g_strdup (src->priv->layout_variant);
    dest->priv->layout_option    = g_strdup (src->priv->layout_option);
    dest->priv->rank             = src->priv->rank;
    dest->priv->hotkeys          = g_strdup (src->priv->hotkeys);
    dest->priv->symbol           = g_strdup (src->priv->symbol);
    dest->priv->setup            = g_strdup (src->priv->setup);
    dest->priv->version          = g_strdup (src->priv->version);
    dest->priv->textdomain       = g_strdup (src->priv->textdomain);
    dest->priv->icon_prop_key    = g_strdup (src->priv->icon_prop_key);
    return TRUE;
}

#define g_string_append_indent(string, indent)  \
    {                                           \
        gint i;                                 \
        for (i = 0; i < (indent); i++) {        \
            g_string_append (string, "    ");   \
        }                                       \
    }

void
ibus_engine_desc_output (IBusEngineDesc *desc,
                         GString        *output,
                         gint            indent)
{
    g_string_append_indent (output, indent);
    g_string_append (output, "<engine>\n");
#define OUTPUT_ENTRY(field, element)                                        \
    {                                                                       \
        gchar *escape_text = g_markup_escape_text (                         \
                        desc->priv->field ? desc->priv->field : "", -1);    \
        g_string_append_indent (output, indent + 1);                        \
        g_string_append_printf (output, "<"element">%s</"element">\n",      \
                                escape_text);                               \
        g_free (escape_text);                                               \
    }
#define OUTPUT_ENTRY_1(name) OUTPUT_ENTRY(name, #name)
    OUTPUT_ENTRY_1(name);
    OUTPUT_ENTRY_1(longname);
    OUTPUT_ENTRY_1(description);
    OUTPUT_ENTRY_1(language);
    OUTPUT_ENTRY_1(license);
    OUTPUT_ENTRY_1(author);
    OUTPUT_ENTRY_1(icon);
    OUTPUT_ENTRY_1(layout);
    OUTPUT_ENTRY_1(layout_variant);
    OUTPUT_ENTRY_1(layout_option);
    OUTPUT_ENTRY_1(hotkeys);
    OUTPUT_ENTRY_1(symbol);
    OUTPUT_ENTRY_1(setup);
    OUTPUT_ENTRY_1(version);
    OUTPUT_ENTRY_1(textdomain);
    OUTPUT_ENTRY_1(icon_prop_key);
    g_string_append_indent (output, indent + 1);
    g_string_append_printf (output, "<rank>%u</rank>\n", desc->priv->rank);
#undef OUTPUT_ENTRY
#undef OUTPUT_ENTRY_1
    g_string_append_indent (output, indent);
    g_string_append (output, "</engine>\n");
}

static gboolean
ibus_engine_desc_parse_xml_node (IBusEngineDesc *desc,
                                XMLNode       *node)
{
    GList *p;

    for (p = node->sub_nodes; p != NULL; p = p->next) {
        XMLNode *sub_node = (XMLNode *) p->data;

#define PARSE_ENTRY(field_name, element_name)                   \
        if (g_strcmp0 (sub_node->name, element_name) == 0) {    \
            g_free (desc->priv->field_name);                    \
            desc->priv->field_name = g_strdup (sub_node->text); \
            continue;                                           \
        }
#define PARSE_ENTRY_1(name) PARSE_ENTRY(name, #name)
        PARSE_ENTRY_1(name);
        PARSE_ENTRY_1(longname);
        PARSE_ENTRY_1(description);
        PARSE_ENTRY_1(language);
        PARSE_ENTRY_1(license);
        PARSE_ENTRY_1(author);
        PARSE_ENTRY_1(icon);
        PARSE_ENTRY_1(layout);
        PARSE_ENTRY_1(layout_variant);
        PARSE_ENTRY_1(layout_option);
        PARSE_ENTRY_1(hotkeys);
        PARSE_ENTRY_1(symbol);
        PARSE_ENTRY_1(setup);
        PARSE_ENTRY_1(version);
        PARSE_ENTRY_1(textdomain);
        PARSE_ENTRY_1(icon_prop_key);
#undef PARSE_ENTRY
#undef PARSE_ENTRY_1
        if (g_strcmp0 (sub_node->name , "rank") == 0) {
            desc->priv->rank = atoi (sub_node->text);
            continue;
        }
        g_warning ("<engines> element contains invalidate element <%s>", sub_node->name);
    }
    return TRUE;
}

#define IBUS_ENGINE_DESC_GET_PROPERTY(property, return_type)    \
return_type                                                     \
ibus_engine_desc_get_ ## property (IBusEngineDesc *desc)        \
{                                                               \
    return desc->priv->property;                                \
}

IBUS_ENGINE_DESC_GET_PROPERTY (name, const gchar *)
IBUS_ENGINE_DESC_GET_PROPERTY (longname, const gchar *)
IBUS_ENGINE_DESC_GET_PROPERTY (description, const gchar *)
IBUS_ENGINE_DESC_GET_PROPERTY (language, const gchar *)
IBUS_ENGINE_DESC_GET_PROPERTY (license, const gchar *)
IBUS_ENGINE_DESC_GET_PROPERTY (author, const gchar *)
IBUS_ENGINE_DESC_GET_PROPERTY (icon, const gchar *)
IBUS_ENGINE_DESC_GET_PROPERTY (layout, const gchar *)
IBUS_ENGINE_DESC_GET_PROPERTY (layout_variant, const gchar *)
IBUS_ENGINE_DESC_GET_PROPERTY (layout_option, const gchar *)
IBUS_ENGINE_DESC_GET_PROPERTY (rank, guint)
IBUS_ENGINE_DESC_GET_PROPERTY (hotkeys, const gchar *)
IBUS_ENGINE_DESC_GET_PROPERTY (symbol, const gchar *)
IBUS_ENGINE_DESC_GET_PROPERTY (setup, const gchar *)
IBUS_ENGINE_DESC_GET_PROPERTY (version, const gchar *)
IBUS_ENGINE_DESC_GET_PROPERTY (textdomain, const gchar *)
IBUS_ENGINE_DESC_GET_PROPERTY (icon_prop_key, const gchar *)
#undef IBUS_ENGINE_DESC_GET_PROPERTY

IBusEngineDesc *
ibus_engine_desc_new (const gchar *name,
                      const gchar *longname,
                      const gchar *description,
                      const gchar *language,
                      const gchar *license,
                      const gchar *author,
                      const gchar *icon,
                      const gchar *layout)
{
    return ibus_engine_desc_new_varargs ("name", name,
                                         "longname", longname,
                                         "description", description,
                                         "language", language,
                                         "license", license,
                                         "author", author,
                                         "icon", icon,
                                         "layout", layout,
                                         NULL);
}

IBusEngineDesc *
ibus_engine_desc_new_varargs (const gchar *first_property_name, ...)
{
    va_list var_args;
    IBusEngineDesc *desc;

    g_assert (first_property_name);

    va_start (var_args, first_property_name);
    desc = (IBusEngineDesc *) g_object_new_valist (IBUS_TYPE_ENGINE_DESC,
                                                   first_property_name,
                                                   var_args);
    va_end (var_args);

    /* name is required. Other properties are set in class_init by default. */
    g_assert (desc->priv->name);
    g_assert (desc->priv->longname);
    g_assert (desc->priv->description);
    g_assert (desc->priv->language);
    g_assert (desc->priv->license);
    g_assert (desc->priv->author);
    g_assert (desc->priv->icon);
    g_assert (desc->priv->layout);
    g_assert (desc->priv->layout_variant);
    g_assert (desc->priv->layout_option);
    g_assert (desc->priv->hotkeys);
    g_assert (desc->priv->symbol);
    g_assert (desc->priv->setup);
    g_assert (desc->priv->version);
    g_assert (desc->priv->textdomain);
    g_assert (desc->priv->icon_prop_key);

    return desc;
}

IBusEngineDesc *
ibus_engine_desc_new_from_xml_node (XMLNode      *node)
{
    g_assert (node);

    IBusEngineDesc *desc;

    if (G_UNLIKELY (g_strcmp0 (node->name, "engine") != 0)) {
        return NULL;
    }

    desc = (IBusEngineDesc *)g_object_new (IBUS_TYPE_ENGINE_DESC, NULL);

    if (!ibus_engine_desc_parse_xml_node (desc, node)) {
        g_object_unref (desc);
        desc = NULL;
    }

    return desc;
}
