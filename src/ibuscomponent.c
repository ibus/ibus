/* -*- mode: C; c-basic-offset: 4; indent-tabs-mode: nil; -*- */
/* vim:set et sts=4: */
/* bus - The Input Bus
 * Copyright (C) 2008-2010 Peng Huang <shawn.p.huang@gmail.com>
 * Copyright (C) 2020-2025 Takao Fujiwara <takao.fujiwara1@gmail.com>
 * Copyright (C) 2008-2020 Red Hat, Inc.
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
#include <glib/gstdio.h>
#include "ibuscomponent.h"
#include "ibusinternal.h"

enum {
    LAST_SIGNAL,
};

enum {
    PROP_0 = 0,
    PROP_NAME,
    PROP_DESCRIPTION,
    PROP_VERSION,
    PROP_LICENSE,
    PROP_AUTHOR,
    PROP_HOMEPAGE,
    PROP_COMMAND_LINE,
    PROP_TEXTDOMAIN,
};

/* IBusComponentPriv */
struct _IBusComponentPrivate {
    gchar *name;
    gchar *description;
    gchar *version;
    gchar *license;
    gchar *author;
    gchar *homepage;
    gchar *exec;
    gchar *textdomain;

    /* engines */
    GList *engines;

    /* observed paths */
    GList *observed_paths;
};

#define IBUS_COMPONENT_GET_PRIVATE(o)  \
   ((IBusComponentPrivate *)ibus_component_get_instance_private (o))

// static guint            _signals[LAST_SIGNAL] = { 0 };

/* functions prototype */
static void         ibus_component_set_property (IBusComponent          *component,
                                                 guint                   prop_id,
                                                 const GValue           *value,
                                                 GParamSpec             *pspec);
static void         ibus_component_get_property (IBusComponent          *component,
                                                 guint                   prop_id,
                                                 GValue                 *value,
                                                 GParamSpec             *pspec);
static void         ibus_component_destroy      (IBusComponent          *component);
static gboolean     ibus_component_serialize    (IBusComponent          *component,
                                                 GVariantBuilder        *builder);
static gint         ibus_component_deserialize  (IBusComponent          *component,
                                                 GVariant               *variant);
static gboolean     ibus_component_copy         (IBusComponent          *dest,
                                                 const IBusComponent    *src);
static gboolean     ibus_component_parse_xml_node
                                                (IBusComponent          *component,
                                                 XMLNode                *node,
                                                 gboolean                access_fs);

static void         ibus_component_parse_engines(IBusComponent          *component,
                                                 XMLNode                *node);
static void         ibus_component_parse_observed_paths
                                                (IBusComponent          *component,
                                                 XMLNode                *node,
                                                 gboolean                access_fs);

G_DEFINE_TYPE_WITH_PRIVATE (IBusComponent,
                            ibus_component,
                            IBUS_TYPE_SERIALIZABLE)

static void
ibus_component_class_init (IBusComponentClass *class)
{
    GObjectClass *gobject_class = G_OBJECT_CLASS (class);
    IBusObjectClass *object_class = IBUS_OBJECT_CLASS (class);
    IBusSerializableClass *serializable_class = IBUS_SERIALIZABLE_CLASS (class);

    gobject_class->set_property = (GObjectSetPropertyFunc) ibus_component_set_property;
    gobject_class->get_property = (GObjectGetPropertyFunc) ibus_component_get_property;
    object_class->destroy = (IBusObjectDestroyFunc) ibus_component_destroy;

    serializable_class->serialize   = (IBusSerializableSerializeFunc) ibus_component_serialize;
    serializable_class->deserialize = (IBusSerializableDeserializeFunc) ibus_component_deserialize;
    serializable_class->copy        = (IBusSerializableCopyFunc) ibus_component_copy;

    /* install properties */
    /**
     * IBusComponent:name:
     *
     * The name of component
     */
    g_object_class_install_property (gobject_class,
                    PROP_NAME,
                    g_param_spec_string ("name",
                        "component name",
                        "The name of component",
                        NULL,
                        G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));

    /**
     * IBusComponent:description:
     *
     * The description of component
     */
    g_object_class_install_property (gobject_class,
                    PROP_DESCRIPTION,
                    g_param_spec_string ("description",
                        "component description",
                        "The description of component",
                        NULL,
                        G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));

    /**
     * IBusComponent:version:
     *
     * The version of component
     */
    g_object_class_install_property (gobject_class,
                    PROP_VERSION,
                    g_param_spec_string ("version",
                        "component version",
                        "The version of component",
                        NULL,
                        G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));

    /**
     * IBusComponent:license:
     *
     * The license of component
     */
    g_object_class_install_property (gobject_class,
                    PROP_LICENSE,
                    g_param_spec_string ("license",
                        "component license",
                        "The license of component",
                        NULL,
                        G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));

    /**
     * IBusComponent:author:
     *
     * The author of component
     */
    g_object_class_install_property (gobject_class,
                    PROP_AUTHOR,
                    g_param_spec_string ("author",
                        "component author",
                        "The author of component",
                        NULL,
                        G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));

    /**
     * IBusComponent:homepage:
     *
     * The homepage of component
     */
    g_object_class_install_property (gobject_class,
                    PROP_HOMEPAGE,
                    g_param_spec_string ("homepage",
                        "component homepage",
                        "The homepage of component",
                        NULL,
                        G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));

    /**
     * IBusComponent:command-line:
     *
     * The exec path of component
     */
    g_object_class_install_property (gobject_class,
                    PROP_COMMAND_LINE,
                    g_param_spec_string ("command-line",
                        "component command-line",
                        "The command line of component",
                        NULL,
                        G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));

    /**
     * IBusComponent:textdomain:
     *
     * The textdomain of component
     */
    g_object_class_install_property (gobject_class,
                    PROP_TEXTDOMAIN,
                    g_param_spec_string ("textdomain",
                        "component textdomain",
                        "The textdomain path of component",
                        NULL,
                        G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));
}


static void
ibus_component_init (IBusComponent *component)
{
    component->priv = IBUS_COMPONENT_GET_PRIVATE (component);
}

static void
ibus_component_destroy (IBusComponent *component)
{
    GList *p;

    g_free (component->priv->name);
    g_free (component->priv->description);
    g_free (component->priv->version);
    g_free (component->priv->license);
    g_free (component->priv->author);
    g_free (component->priv->homepage);
    g_free (component->priv->exec);
    g_free (component->priv->textdomain);

    component->priv->name = NULL;
    component->priv->description = NULL;
    component->priv->version = NULL;
    component->priv->license = NULL;
    component->priv->author = NULL;
    component->priv->homepage = NULL;
    component->priv->exec = NULL;
    component->priv->textdomain = NULL;

    g_list_free_full (component->priv->observed_paths, g_object_unref);
    component->priv->observed_paths = NULL;

    for (p = component->priv->engines; p != NULL; p = p->next) {
        g_object_steal_data ((GObject *)p->data, "component");
        ibus_object_destroy ((IBusObject *)p->data);
        g_object_unref (p->data);
    }
    g_list_free (component->priv->engines);
    component->priv->engines = NULL;

    IBUS_OBJECT_CLASS (ibus_component_parent_class)->destroy (IBUS_OBJECT (component));
}

static void
ibus_component_set_property (IBusComponent *component,
                             guint          prop_id,
                             const GValue  *value,
                             GParamSpec    *pspec)
{
    switch (prop_id) {
    case PROP_NAME:
        g_assert (component->priv->name == NULL);
        component->priv->name = g_value_dup_string (value);
        break;
    case PROP_DESCRIPTION:
        g_assert (component->priv->description == NULL);
        component->priv->description = g_value_dup_string (value);
        break;
    case PROP_VERSION:
        g_assert (component->priv->version == NULL);
        component->priv->version = g_value_dup_string (value);
        break;
    case PROP_LICENSE:
        g_assert (component->priv->license == NULL);
        component->priv->license = g_value_dup_string (value);
        break;
    case PROP_AUTHOR:
        g_assert (component->priv->author == NULL);
        component->priv->author = g_value_dup_string (value);
        break;
    case PROP_HOMEPAGE:
        g_assert (component->priv->homepage == NULL);
        component->priv->homepage = g_value_dup_string (value);
        break;
    case PROP_COMMAND_LINE:
        g_assert (component->priv->exec == NULL);
        component->priv->exec = g_value_dup_string (value);
        break;
    case PROP_TEXTDOMAIN:
        g_assert (component->priv->textdomain == NULL);
        component->priv->textdomain = g_value_dup_string (value);
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (component, prop_id, pspec);
    }
}

static void
ibus_component_get_property (IBusComponent *component,
                             guint          prop_id,
                             GValue        *value,
                             GParamSpec    *pspec)
{
    switch (prop_id) {
    case PROP_NAME:
        g_value_set_string (value, ibus_component_get_name (component));
        break;
    case PROP_DESCRIPTION:
        g_value_set_string (value, ibus_component_get_description (component));
        break;
    case PROP_VERSION:
        g_value_set_string (value, ibus_component_get_version (component));
        break;
    case PROP_LICENSE:
        g_value_set_string (value, ibus_component_get_license (component));
        break;
    case PROP_AUTHOR:
        g_value_set_string (value, ibus_component_get_author (component));
        break;
    case PROP_HOMEPAGE:
        g_value_set_string (value, ibus_component_get_homepage (component));
        break;
    case PROP_COMMAND_LINE:
        g_value_set_string (value, ibus_component_get_exec (component));
        break;
    case PROP_TEXTDOMAIN:
        g_value_set_string (value, ibus_component_get_textdomain (component));
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (component, prop_id, pspec);
    }
}

static gboolean
ibus_component_serialize (IBusComponent   *component,
                          GVariantBuilder *builder)
{
    gboolean retval;

    retval = IBUS_SERIALIZABLE_CLASS (ibus_component_parent_class)->serialize ((IBusSerializable *)component, builder);
    g_return_val_if_fail (retval, FALSE);

    g_variant_builder_add (builder, "s", component->priv->name);
    g_variant_builder_add (builder, "s", component->priv->description);
    g_variant_builder_add (builder, "s", component->priv->version);
    g_variant_builder_add (builder, "s", component->priv->license);
    g_variant_builder_add (builder, "s", component->priv->author);
    g_variant_builder_add (builder, "s", component->priv->homepage);
    g_variant_builder_add (builder, "s", component->priv->exec);
    g_variant_builder_add (builder, "s", component->priv->textdomain);

    GList *p;
    GVariantBuilder *array;
    /* serialize observed paths */
    array = g_variant_builder_new (G_VARIANT_TYPE ("av"));
    for (p = component->priv->observed_paths; p != NULL; p = p->next) {
        g_variant_builder_open (array, G_VARIANT_TYPE_VARIANT);
        g_variant_builder_add_value (
                array,
                ibus_serializable_serialize ((IBusSerializable *)p->data));
        g_variant_builder_close (array);
    }
    g_variant_builder_add (builder, "av", array);
    g_variant_builder_unref (array);

    /* serialize engine desc list */
    array = g_variant_builder_new (G_VARIANT_TYPE ("av"));
    for (p = component->priv->engines; p != NULL; p = p->next) {
        g_variant_builder_open (array, G_VARIANT_TYPE_VARIANT);
        g_variant_builder_add_value (
                array,
                ibus_serializable_serialize ((IBusSerializable *)p->data));
        g_variant_builder_close (array);
    }
    g_variant_builder_add (builder, "av", array);
    g_variant_builder_unref (array);

    return TRUE;
}

static gint
ibus_component_deserialize (IBusComponent   *component,
                            GVariant        *variant)
{
    gboolean retval;

    retval = IBUS_SERIALIZABLE_CLASS (ibus_component_parent_class)->deserialize ((IBusSerializable *)component, variant);
    g_return_val_if_fail (retval, 0);

    ibus_g_variant_get_child_string (variant, retval++,
                                     &component->priv->name);
    ibus_g_variant_get_child_string (variant, retval++,
                                     &component->priv->description);
    ibus_g_variant_get_child_string (variant, retval++,
                                     &component->priv->version);
    ibus_g_variant_get_child_string (variant, retval++,
                                     &component->priv->license);
    ibus_g_variant_get_child_string (variant, retval++,
                                     &component->priv->author);
    ibus_g_variant_get_child_string (variant, retval++,
                                     &component->priv->homepage);
    ibus_g_variant_get_child_string (variant, retval++,
                                     &component->priv->exec);
    ibus_g_variant_get_child_string (variant, retval++,
                                     &component->priv->textdomain);

    GVariant *var;
    GVariantIter *iter = NULL;
    g_variant_get_child (variant, retval++, "av", &iter);
    while (g_variant_iter_loop (iter, "v", &var)) {
        component->priv->observed_paths = g_list_append (component->priv->observed_paths,
                        IBUS_OBSERVED_PATH (ibus_serializable_deserialize (var)));
    }
    g_variant_iter_free (iter);

    g_variant_get_child (variant, retval++, "av", &iter);
    while (g_variant_iter_loop (iter, "v", &var)) {
        ibus_component_add_engine (component,
                                   IBUS_ENGINE_DESC (ibus_serializable_deserialize (var)));
    }
    g_variant_iter_free (iter);

    return retval;
}

static gboolean
ibus_component_copy (IBusComponent       *dest,
                     const IBusComponent *src)
{
    gboolean retval;

    retval = IBUS_SERIALIZABLE_CLASS (ibus_component_parent_class)->copy ((IBusSerializable *)dest,
                                 (IBusSerializable *)src);
    g_return_val_if_fail (retval, FALSE);

    dest->priv->name          = g_strdup (src->priv->name);
    dest->priv->description   = g_strdup (src->priv->description);
    dest->priv->version       = g_strdup (src->priv->version);
    dest->priv->license       = g_strdup (src->priv->license);
    dest->priv->author        = g_strdup (src->priv->author);
    dest->priv->homepage      = g_strdup (src->priv->homepage);
    dest->priv->exec          = g_strdup (src->priv->exec);
    dest->priv->textdomain    = g_strdup (src->priv->textdomain);

    dest->priv->observed_paths = g_list_copy (src->priv->observed_paths);
    g_list_foreach (dest->priv->observed_paths, (GFunc) g_object_ref, NULL);

    dest->priv->engines = g_list_copy (src->priv->engines);
    g_list_foreach (dest->priv->engines, (GFunc) g_object_ref, NULL);

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
ibus_component_output (IBusComponent *component,
                      GString      *output,
                      gint          indent)
{
    g_assert (IBUS_IS_COMPONENT (component));
    GList *p;

    g_string_append_indent (output, indent);
    g_string_append (output, "<component>\n");

#define OUTPUT_ENTRY(field, element)                                        \
    {                                                                       \
        gchar *escape_text =                                                \
            g_markup_escape_text (component->priv->field ?                  \
                                  component->priv->field : "", -1);         \
        g_string_append_indent (output, indent + 1);                        \
        g_string_append_printf (output, "<"element">%s</"element">\n",      \
                                escape_text);                               \
        g_free (escape_text);                                               \
    }
#define OUTPUT_ENTRY_1(name) OUTPUT_ENTRY(name, #name)
    OUTPUT_ENTRY_1 (name);
    OUTPUT_ENTRY_1 (description);
    OUTPUT_ENTRY_1 (version);
    OUTPUT_ENTRY_1 (license);
    OUTPUT_ENTRY_1 (author);
    OUTPUT_ENTRY_1 (homepage);
    OUTPUT_ENTRY_1 (exec);
    OUTPUT_ENTRY_1 (textdomain);
#undef OUTPUT_ENTRY
#undef OUTPUT_ENTRY_1

    if (component->priv->observed_paths) {
        g_string_append_indent (output, indent + 1);
        g_string_append (output, "<observed-paths>\n");

        for (p = component->priv->observed_paths; p != NULL; p = p->next ) {
            IBusObservedPath *path = (IBusObservedPath *) p->data;
            ibus_observed_path_output (path, output, indent + 2);
        }

        g_string_append_indent (output, indent + 1);
        g_string_append (output, "</observed-paths>\n");
    }

    ibus_component_output_engines (component, output, indent + 1);

    g_string_append_indent (output, indent);
    g_string_append (output, "</component>\n");
}

void
ibus_component_output_engines (IBusComponent  *component,
                               GString        *output,
                               gint            indent)
{
    g_assert (IBUS_IS_COMPONENT (component));
    g_assert (output);

    GList *p;

    g_string_append_indent (output, indent);
    g_string_append (output, "<engines>\n");

    for (p = component->priv->engines; p != NULL; p = p->next) {
        ibus_engine_desc_output ((IBusEngineDesc *)p->data, output, indent + 2);
    }

    g_string_append_indent (output, indent);
    g_string_append (output, "</engines>\n");
}

static gboolean
ibus_component_parse_xml_node (IBusComponent   *component,
                              XMLNode          *node,
                              gboolean          access_fs)
{
    g_assert (component);
    g_assert (node);

    if (G_UNLIKELY (g_strcmp0 (node->name, "component") != 0)) {
        return FALSE;
    }

    GList *p;
    for (p = node->sub_nodes; p != NULL; p = p->next) {
        XMLNode *sub_node = (XMLNode *)p->data;

#define PARSE_ENTRY(field_name, element_name)                           \
        if (g_strcmp0 (sub_node->name, element_name) == 0) {            \
            if (component->priv->field_name != NULL) {                  \
                g_free (component->priv->field_name);                   \
            }                                                           \
            component->priv->field_name = g_strdup (sub_node->text);    \
            continue;                                                   \
        }
#define PARSE_ENTRY_1(name) PARSE_ENTRY (name, #name)
        PARSE_ENTRY_1 (name);
        PARSE_ENTRY_1 (description);
        PARSE_ENTRY_1 (version);
        PARSE_ENTRY_1 (license);
        PARSE_ENTRY_1 (author);
        PARSE_ENTRY_1 (homepage);
        PARSE_ENTRY_1 (exec);
        PARSE_ENTRY_1 (textdomain);
#undef PARSE_ENTRY
#undef PARSE_ENTRY_1

        if (g_strcmp0 (sub_node->name, "engines") == 0) {
            ibus_component_parse_engines (component, sub_node);
            continue;
        }

        if (g_strcmp0 (sub_node->name, "observed-paths") == 0) {
            ibus_component_parse_observed_paths (component, sub_node, access_fs);
            continue;
        }

        g_warning ("<component> element contains invalidate element <%s>", sub_node->name);
    }

    return TRUE;
}


static void
ibus_component_parse_engines (IBusComponent *component,
                              XMLNode       *node)
{
    g_assert (IBUS_IS_COMPONENT (component));
    g_assert (node);

    gchar *exec = NULL;
    gchar **p;
    XMLNode *engines_node = NULL;

    if (g_strcmp0 (node->name, "engines") != 0) {
        return;
    }

    for (p = node->attributes; *p != NULL; p += 2) {
        if (g_strcmp0 (*p, "exec") == 0) {
            exec = *(p + 1);
            break;
        }
    }

    if (exec != NULL) {
        gchar *output = NULL;
        gchar *errput = NULL;
        GError *error = NULL;
        if (g_spawn_command_line_sync (exec, &output, &errput, NULL, &error)) {
            engines_node = ibus_xml_parse_buffer (output);
            g_free (output);

            if (engines_node) {
                if (g_strcmp0 (engines_node->name, "engines") == 0) {
                    node = engines_node;
                }
            }
            if (errput) {
                g_warning ("Engines exec:%s is failed: %s", exec, errput);
                g_free (errput);
            }
        } else {
            g_warning ("Engines exec:%s is failed: %s: %s",
                       exec, errput ? errput : "(null)", error->message);
            g_error_free (error);
            g_free (errput);
        }
    }

    GList *pl;
    for (pl = node->sub_nodes; pl != NULL; pl = pl->next) {
        IBusEngineDesc *engine;
        engine = ibus_engine_desc_new_from_xml_node ((XMLNode *)pl->data);

        if (G_UNLIKELY (engine == NULL))
            continue;
        ibus_component_add_engine (component, engine);
    }

    if (engines_node) {
        ibus_xml_free (engines_node);
    }
}

static void
ibus_component_parse_observed_paths (IBusComponent *component,
                                     XMLNode       *node,
                                     gboolean       access_fs)
{
    g_assert (IBUS_IS_COMPONENT (component));
    g_assert (node);

    if (g_strcmp0 (node->name, "observed-paths") != 0) {
        return;
    }

    GList *p;
    for (p = node->sub_nodes; p != NULL; p = p->next) {
        IBusObservedPath *path;

        path = ibus_observed_path_new_from_xml_node ((XMLNode *)p->data, access_fs);
        g_object_ref_sink (path);
        component->priv->observed_paths = g_list_append (component->priv->observed_paths, path);

        if (access_fs && path->is_dir && path->is_exist) {
            component->priv->observed_paths =
                    g_list_concat (component->priv->observed_paths,
                                   ibus_observed_path_traverse (path, TRUE));
        }
    }
}

#define IBUS_COMPONENT_GET_PROPERTY(property, return_type)  \
return_type                                                 \
ibus_component_get_ ## property (IBusComponent *component)  \
{                                                           \
    return component->priv->property;                       \
}

IBUS_COMPONENT_GET_PROPERTY (name, const gchar *)
IBUS_COMPONENT_GET_PROPERTY (description, const gchar *)
IBUS_COMPONENT_GET_PROPERTY (version, const gchar *)
IBUS_COMPONENT_GET_PROPERTY (license, const gchar *)
IBUS_COMPONENT_GET_PROPERTY (author, const gchar *)
IBUS_COMPONENT_GET_PROPERTY (homepage, const gchar *)
IBUS_COMPONENT_GET_PROPERTY (exec, const gchar *)
IBUS_COMPONENT_GET_PROPERTY (textdomain, const gchar *)
#undef IBUS_COMPONENT_GET_PROPERTY

IBusComponent *
ibus_component_new (const gchar *name,
                    const gchar *description,
                    const gchar *version,
                    const gchar *license,
                    const gchar *author,
                    const gchar *homepage,
                    const gchar *command_line,
                    const gchar *textdomain)
{
    return ibus_component_new_varargs ("name", name,
                                       "description", description,
                                       "version", version,
                                       "license", license,
                                       "author", author,
                                       "homepage", homepage,
                                       "command-line", command_line,
                                       "textdomain", textdomain,
                                       NULL);
}


IBusComponent *
ibus_component_new_varargs (const gchar *first_property_name, ...)
{
    va_list var_args;
    IBusComponent *component;
    G_GNUC_UNUSED IBusComponentPrivate *priv;

    g_assert (first_property_name);

    va_start (var_args, first_property_name);
    component = (IBusComponent *) g_object_new_valist (IBUS_TYPE_COMPONENT,
                                                       first_property_name,
                                                       var_args);
    va_end (var_args);

    priv = IBUS_COMPONENT_GET_PRIVATE (component);

    /* name is required. Other properties are set in class_init by default. */
    g_assert (priv->name);

    return component;
}


IBusComponent *
ibus_component_new_from_xml_node (XMLNode  *node)
{
    g_assert (node);

    IBusComponent *component;

    component = (IBusComponent *)g_object_new (IBUS_TYPE_COMPONENT, NULL);
    if (!ibus_component_parse_xml_node (component, node, FALSE)) {
        g_object_unref (component);
        component = NULL;
    }

    return component;
}

IBusComponent *
ibus_component_new_from_file (const gchar *filename)
{
    g_assert (filename);

    XMLNode *node;
    struct stat buf;
    IBusComponent *component;
    gboolean retval;

    if (g_stat (filename, &buf) != 0) {
        g_warning ("Can not get stat of file %s", filename);
        return NULL;
    }

    node = ibus_xml_parse_file (filename);

    if (!node) {
        return NULL;
    }

    component = (IBusComponent *)g_object_new (IBUS_TYPE_COMPONENT, NULL);
    retval = ibus_component_parse_xml_node (component, node, TRUE);
    ibus_xml_free (node);

    if (!retval) {
        g_object_unref (component);
        component = NULL;
    }
    else {
        IBusObservedPath *path;
        path = ibus_observed_path_new (filename, TRUE);
        component->priv->observed_paths =
                g_list_prepend(component->priv->observed_paths, path);
    }

    return component;
}

void
ibus_component_add_observed_path (IBusComponent *component,
                                  const gchar   *path,
                                  gboolean       access_fs)
{
    IBusObservedPath *p;

    p = ibus_observed_path_new (path, access_fs);
    g_object_ref_sink (p);
    component->priv->observed_paths =
            g_list_append (component->priv->observed_paths, p);

    if (access_fs && p->is_dir && p->is_exist) {
        component->priv->observed_paths =
                g_list_concat (component->priv->observed_paths,
                               ibus_observed_path_traverse (p, TRUE));
    }
}

void
ibus_component_add_engine (IBusComponent  *component,
                           IBusEngineDesc *engine)
{
    g_assert (IBUS_IS_COMPONENT (component));
    g_assert (IBUS_IS_ENGINE_DESC (engine));

    g_object_ref_sink (engine);
    component->priv->engines =
            g_list_append (component->priv->engines, engine);
}

GList *
ibus_component_get_engines (IBusComponent *component)
{
    return g_list_copy (component->priv->engines);
}

gboolean
ibus_component_check_modification (IBusComponent *component)
{
    g_assert (IBUS_IS_COMPONENT (component));

    GList *p;

    for (p = component->priv->observed_paths; p != NULL; p = p->next) {
        if (ibus_observed_path_check_modification ((IBusObservedPath *)p->data))
            return TRUE;
    }
    return FALSE;
}

GList *
ibus_component_get_observed_paths (IBusComponent *component)
{
    g_assert (IBUS_IS_COMPONENT (component));
    return g_list_copy (component->priv->observed_paths);
}
