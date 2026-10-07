/* -*- mode: C; c-basic-offset: 4; indent-tabs-mode: nil; -*- */
/* vim:set et sts=4: */
/* bus - The Input Bus
 * Copyright (C) 2008-2015 Peng Huang <shawn.p.huang@gmail.com>
 * Copyright (C) 2008-2015 Red Hat, Inc.
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

#if !defined (__IBUS_H_INSIDE__) && !defined (IBUS_COMPILATION)
#error "Only <ibus.h> can be included directly"
#endif

#ifndef __IBUS_COMPONENT_H_
#define __IBUS_COMPONENT_H_

/**
 * SECTION: ibuscomponent
 * @short_description:  Component (executable) specification.
 * @title: IBusComponent
 * @stability: Stable
 *
 * An IBusComponent is an executable program.
 * It provides services such as user interface, configuration,
 * and input method engine (IME).
 *
 * It is recommended that IME developers provide
 * a component XML file and
 * load the XML file by ibus_component_new_from_file().
 *
 * The format of a component XML file is described  at
 * <ulink url="https://github.com/ibus/ibus/wiki/DevXML">https://github.com/ibus/ibus/wiki/DevXML</ulink>
 */

#include "ibusserializable.h"
#include "ibusobservedpath.h"
#include "ibusenginedesc.h"
#include "ibusxml.h"

/*
 * Type macros.
 */

/* define GOBJECT macros */
#define IBUS_TYPE_COMPONENT             \
    (ibus_component_get_type ())
#define IBUS_COMPONENT(obj)             \
    (G_TYPE_CHECK_INSTANCE_CAST ((obj), IBUS_TYPE_COMPONENT, IBusComponent))
#define IBUS_COMPONENT_CLASS(klass)     \
    (G_TYPE_CHECK_CLASS_CAST ((klass), IBUS_TYPE_COMPONENT, IBusComponentClass))
#define IBUS_IS_COMPONENT(obj)          \
    (G_TYPE_CHECK_INSTANCE_TYPE ((obj), IBUS_TYPE_COMPONENT))
#define IBUS_IS_COMPONENT_CLASS(klass)  \
    (G_TYPE_CHECK_CLASS_TYPE ((klass), IBUS_TYPE_COMPONENT))
#define IBUS_COMPONENT_GET_CLASS(obj)   \
    (G_TYPE_INSTANCE_GET_CLASS ((obj), IBUS_TYPE_COMPONENT, IBusComponentClass))

G_BEGIN_DECLS

typedef struct _IBusComponent IBusComponent;
typedef struct _IBusComponentClass IBusComponentClass;
typedef struct _IBusComponentPrivate IBusComponentPrivate;

/**
 * IBusComponent:
 *
 * An IBusComponent stores component information.
 * You can get extended values with g_object_get_properties.
 */
struct _IBusComponent {
    /*< private >*/
    IBusSerializable parent;
    IBusComponentPrivate *priv;
    /* instance members */

    /*< public >*/

    /*< private >*/
    /* padding */
    gpointer pdummy[7];  // We can add 7 pointers without breaking the ABI.
};

struct _IBusComponentClass {
  IBusSerializableClass parent;

  /* class members */
};

GType            ibus_component_get_type        (void);

/**
 * ibus_component_new:
 * @name: Name of the component.
 * @description: Detailed description of component.
 * @version: Component version.
 * @license: Distribution license of this component.
 * @author: Author(s) of the component.
 * @homepage: Homepage of the component.
 * @command_line: path to component executable.
 * @textdomain: Domain name for dgettext()
 *
 * Creates a new #IBusComponent.
 *
 * Returns: A newly allocated #IBusComponent.
 */
IBusComponent   *ibus_component_new             (const gchar    *name,
                                                 const gchar    *description,
                                                 const gchar    *version,
                                                 const gchar    *license,
                                                 const gchar    *author,
                                                 const gchar    *homepage,
                                                 const gchar    *command_line,
                                                 const gchar    *textdomain);

/**
 * ibus_component_new_varargs:
 * @first_property_name: Name of the first property.
 * @...: the NULL-terminated arguments of the properties and values.
 *
 * Creates a new #IBusComponent.
 * ibus_component_new_varargs() supports the va_list format.
 * name property is required. e.g.
 * IBusComponent *component = ibus_component_new_varargs ("name", "ibus-foo",
 *                                                        "command_line", "/usr/libexec/ibus-engine-foo --ibus",
 *                                                     NULL)
 *
 * Returns: A newly allocated #IBusComponent.
 */
IBusComponent   *ibus_component_new_varargs     (const gchar    *first_property_name,
                                                 ...);

/**
 * ibus_component_new_from_xml_node:
 * @node: Root node of component XML tree.
 *
 * Creates a new #IBusComponent from an XML tree.
 *
 * Returns: A newly allocated #IBusComponent.
 */
IBusComponent   *ibus_component_new_from_xml_node
                                                (XMLNode        *node);

/**
 * ibus_component_new_from_file:
 * @filename: An XML file that contains component information.
 *
 * Creates a new #IBusComponent from an XML file.
 * Note that a component file usually contains engine descriptions,
 * if it does, ibus_engine_desc_new_from_xml_node() will be called
 * to load the engine descriptions.
 *
 * Returns: A newly allocated #IBusComponent.
 */
IBusComponent   *ibus_component_new_from_file   (const gchar    *filename);

/**
 * ibus_component_get_name:
 * @component: An #IBusComponent
 *
 * Gets the name property in #IBusComponent. It should not be freed.
 *
 * Returns: name property in #IBusComponent
 */
const gchar     *ibus_component_get_name        (IBusComponent  *component);

/**
 * ibus_component_get_description:
 * @component: An #IBusComponent
 *
 * Gets the description property in #IBusComponent. It should not be freed.
 *
 * Returns: description property in #IBusComponent
 */
const gchar     *ibus_component_get_description (IBusComponent  *component);

/**
 * ibus_component_get_version:
 * @component: An #IBusComponent
 *
 * Gets the version property in #IBusComponent. It should not be freed.
 *
 * Returns: version property in #IBusComponent
 */
const gchar     *ibus_component_get_version     (IBusComponent  *component);

/**
 * ibus_component_get_license:
 * @component: An #IBusComponent
 *
 * Gets the license property in #IBusComponent. It should not be freed.
 *
 * Returns: license property in #IBusComponent
 */
const gchar     *ibus_component_get_license     (IBusComponent  *component);

/**
 * ibus_component_get_author:
 * @component: An #IBusComponent
 *
 * Gets the author property in #IBusComponent. It should not be freed.
 *
 * Returns: author property in #IBusComponent
 */
const gchar     *ibus_component_get_author      (IBusComponent  *component);

/**
 * ibus_component_get_homepage:
 * @component: An #IBusComponent
 *
 * Gets the homepage property in #IBusComponent. It should not be freed.
 *
 * Returns: homepage property in #IBusComponent
 */
const gchar     *ibus_component_get_homepage    (IBusComponent  *component);

/**
 * ibus_component_get_exec:
 * @component: An #IBusComponent
 *
 * Gets the exec property in #IBusComponent. It should not be freed.
 *
 * Returns: exec property in #IBusComponent
 */
const gchar     *ibus_component_get_exec        (IBusComponent  *component);

/**
 * ibus_component_get_textdomain:
 * @component: An #IBusComponent
 *
 * Gets the textdomain property in #IBusComponent. It should not be freed.
 *
 * Returns: textdomain property in #IBusComponent
 */
const gchar     *ibus_component_get_textdomain  (IBusComponent  *component);

/**
 * ibus_component_add_observed_path:
 * @component: An #IBusComponent
 * @path: Observed path to be added.
 * @access_fs: %TRUE for filling the file status; %FALSE otherwise.
 *
 * Add an observed path to #IBusComponent.
 */
void             ibus_component_add_observed_path
                                                (IBusComponent  *component,
                                                 const gchar    *path,
                                                 gboolean        access_fs);

/**
 * ibus_component_add_engine:
 * @component: An #IBusComponent
 * @engine: A description of an engine.
 *
 * Add an engine to #IBusComponent according to the description in @engine.
 */
void             ibus_component_add_engine      (IBusComponent  *component,
                                                 IBusEngineDesc *engine);

/**
 * ibus_component_get_engines:
 * @component: An #IBusComponent.
 *
 * Gets the engines of this component.
 *
 * Returns: (transfer container) (element-type IBusEngineDesc):
 * A newly allocated GList that contains engines.
 */
GList           *ibus_component_get_engines     (IBusComponent  *component);

/**
 * ibus_component_output:
 * @component: An #IBusComponent.
 * @output: GString that holds the result.
 * @indent: level of indent.
 *
 * Output #IBusComponent as an XML-formatted string.
 * The output string can be then shown on the screen or written to file.
 */
void             ibus_component_output          (IBusComponent  *component,
                                                 GString        *output,
                                                 gint            indent);

/**
 * ibus_component_output_engines:
 * @component: An #IBusComponent.
 * @output: GString that holds the result.
 * @indent: level of indent.
 *
 * Output engine description  as an XML-formatted string.
 * The output string can be then shown on the screen or written to file.
 */
void             ibus_component_output_engines  (IBusComponent  *component,
                                                 GString        *output,
                                                 gint            indent);

/**
 * ibus_component_check_modification:
 * @component: An #IBusComponent.
 *
 * Check whether the observed paths of component is modified.
 *
 * Returns: %TRUE if at least one of the observed paths is modified;
 * %FALSE otherwise.
 */
gboolean         ibus_component_check_modification
                                                (IBusComponent  *component);

/**
 * ibus_component_get_observed_paths:
 * @component: An #IBusComponent.
 *
 * Gets the observed paths of this component.
 *
 * Returns: (transfer container) (element-type IBusObservedPath): A
 * newly allocated GList that contains observed paths.
 */
GList           *ibus_component_get_observed_paths
                                                (IBusComponent *component);

G_END_DECLS
#endif

