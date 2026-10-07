/* -*- mode: C; c-basic-offset: 4; indent-tabs-mode: nil; -*- */
/* vim:set et sts=4: */
/* ibus - The Input IBus
 * Copyright (C) 2008-2013 Peng Huang <shawn.p.huang@gmail.com>
 * Copyright (C) 2008-2013 Red Hat, Inc.
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

#ifndef __IBUS_OBSERVED_PATH_H_
#define __IBUS_OBSERVED_PATH_H_

/**
 * SECTION: ibusobservedpath
 * @short_description: Path object of IBus.
 * @stability: Stable
 *
 * IBusObservedPath provides methods for file path manipulation,
 * such as monitor modification, directory tree traversal.
 */

#include "ibusserializable.h"
#include "ibusxml.h"

/*
 * Type macros.
 */

/* define GOBJECT macros */
#define IBUS_TYPE_OBSERVED_PATH             \
    (ibus_observed_path_get_type ())
#define IBUS_OBSERVED_PATH(obj)             \
    (G_TYPE_CHECK_INSTANCE_CAST ((obj), IBUS_TYPE_OBSERVED_PATH, IBusObservedPath))
#define IBUS_OBSERVED_PATH_CLASS(klass)     \
    (G_TYPE_CHECK_CLASS_CAST ((klass), IBUS_TYPE_OBSERVED_PATH, IBusObservedPathClass))
#define IBUS_IS_OBSERVED_PATH(obj)          \
    (G_TYPE_CHECK_INSTANCE_TYPE ((obj), IBUS_TYPE_OBSERVED_PATH))
#define IBUS_IS_OBSERVED_PATH_CLASS(klass)  \
    (G_TYPE_CHECK_CLASS_TYPE ((klass), IBUS_TYPE_OBSERVED_PATH))
#define IBUS_OBSERVED_PATH_GET_CLASS(obj)   \
    (G_TYPE_INSTANCE_GET_CLASS ((obj), IBUS_TYPE_OBSERVED_PATH, IBusObservedPathClass))

G_BEGIN_DECLS

typedef struct _IBusObservedPath IBusObservedPath;
typedef struct _IBusObservedPathClass IBusObservedPathClass;

/**
 * IBusObservedPath:
 * @path: Path to be handled.
 * @mtime: Modified time.
 * @is_dir: Whether the file is the path directory.
 * @is_exist: Whether the file exists.
 *
 * Data structure of IBusObservedPath.
 */
struct _IBusObservedPath {
    IBusSerializable parent;
    /* instance members */

    /*< public >*/
    gchar *path;
    glong mtime;
    gboolean is_dir;
    gboolean is_exist;

};

struct _IBusObservedPathClass {
    IBusSerializableClass parent;

    /* class members */
};

GType                ibus_observed_path_get_type            (void);

/**
 * ibus_observed_path_new_from_xml_node:
 * @node: An XML node that contain path.
 * @fill_stat: Auto-fill the path status.
 *
 * Creates an new #IBusObservedPath from an XML node.
 *
 * Returns: A newly allocated #IBusObservedPath.
 */
IBusObservedPath    *ibus_observed_path_new_from_xml_node   (XMLNode            *node,
                                                             gboolean            fill_stat);

/**
 * ibus_observed_path_new:
 * @path: The path string.
 * @fill_stat: Auto-fill the path status.
 *
 * Creates a new #IBusObservedPath from an XML node.
 *
 * Returns: A newly allocated #IBusObservedPath.
 */
IBusObservedPath    *ibus_observed_path_new                 (const gchar        *path,
                                                             gboolean            fill_stat);

/**
 * ibus_observed_path_traverse:
 * @path: An IBusObservedPath.
 * @dir_only: Only looks for subdirs, not files
 *
 * Recursively traverse the path and put the files and subdirectory in to
 * a newly allocated
 * GLists, if the @path is a directory. Otherwise returns NULL.
 *
 * Returns: (transfer full) (element-type IBusObservedPath): A newly allocate
 * GList which holds content in path; NULL if @path is not directory.
 */
GList               *ibus_observed_path_traverse            (IBusObservedPath   *path,
                                                             gboolean            dir_only);

/**
 * ibus_observed_path_check_modification:
 * @path: An IBusObservedPath.
 *
 * Checks whether the path is modified by comparing the mtime in object and
 * mtime in file system.
 *
 * Returns: %TRUE if imtime is changed, otherwise %FALSE.
 */
gboolean             ibus_observed_path_check_modification  (IBusObservedPath   *path);

/**
 * ibus_observed_path_output:
 * @path: An IBusObservedPath.
 * @output: Path is appended to.
 * @indent: number of indent.
 *
 * Append the observed path to a string with following format:
 * &lt;path mtime="&lt;i&gt;modified time&lt;/i&gt;" &gt;&lt;i&gt;path&lt;/i&gt;&lt;/path&gt;
 */
void                 ibus_observed_path_output              (IBusObservedPath   *path,
                                                             GString            *output,
                                                             gint                indent);

G_END_DECLS
#endif

