/* -*- mode: C; c-basic-offset: 4; indent-tabs-mode: nil; -*- */
/* vim:set et sts=4: */
/* bus - The Input Bus
 * Copyright (C) 2010 Peng Huang <shawn.p.huang@gmail.com>
 * Copyright (C) 2010 Google Inc.
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
#ifndef __BUS_COMPONENT_H_
#define __BUS_COMPONENT_H_

#include <ibus.h>
#include "factoryproxy.h"

/*
 * Type macros.
 */

/* define GOBJECT macros */
#define BUS_TYPE_COMPONENT             \
    (bus_component_get_type ())
#define BUS_COMPONENT(obj)             \
    (G_TYPE_CHECK_INSTANCE_CAST ((obj), BUS_TYPE_COMPONENT, BusComponent))
#define BUS_COMPONENT_CLASS(klass)     \
    (G_TYPE_CHECK_CLASS_CAST ((klass), BUS_TYPE_COMPONENT, BusComponentClass))
#define BUS_IS_COMPONENT(obj)          \
    (G_TYPE_CHECK_INSTANCE_TYPE ((obj), BUS_TYPE_COMPONENT))
#define BUS_IS_COMPONENT_CLASS(klass)  \
    (G_TYPE_CHECK_CLASS_TYPE ((klass), BUS_TYPE_COMPONENT))
#define BUS_COMPONENT_GET_CLASS(obj)   \
    (G_TYPE_INSTANCE_GET_CLASS ((obj), BUS_TYPE_COMPONENT, BusComponentClass))

G_BEGIN_DECLS

typedef struct _BusComponent BusComponent;
typedef struct _BusComponentClass BusComponentClass;

GType            bus_component_get_type          (void);
BusComponent    *bus_component_new               (IBusComponent   *component,
                                                  BusFactoryProxy *factory);
IBusComponent   *bus_component_get_component     (BusComponent    *component);
void             bus_component_set_factory       (BusComponent    *compinent,
                                                  BusFactoryProxy *factory);
BusFactoryProxy *bus_component_get_factory       (BusComponent    *factory);
void             bus_component_set_destroy_with_factory
                                                 (BusComponent    *component,
                                                  gboolean         with_factory);

/**
 * bus_component_get_name:
 *
 * Return a component name such as "org.freedesktop.IBus.Panel" and "com.google.IBus.Mozc"
 */
const gchar     *bus_component_get_name          (BusComponent    *component);

/**
 * bus_component_get_engines:
 *
 * Return a list of IBusEngineDesc objects the component has.
 */
GList           *bus_component_get_engines       (BusComponent    *component);

/**
 * bus_component_start:
 * @verbose: if TRUE, the stdout and stderr of the child process is not redirected to /dev/null.
 * @returns: TRUE if the component is successfully started.
 *
 * Start the component by forking and executing an executable file for the component.
 */
gboolean         bus_component_start             (BusComponent    *component,
                                                  gboolean         verbose);

/**
 * bus_component_stop:
 * @returns: TRUE
 *
 * Kill a process for the component.
 */
gboolean         bus_component_stop              (BusComponent    *component);

/**
 * bus_component_stop:
 * @returns: TRUE if a process for the component exists.
 */
gboolean         bus_component_is_running        (BusComponent    *component);

void             bus_component_set_restart       (BusComponent    *component,
                                                  gboolean         restart);
BusComponent    *bus_component_from_engine_desc  (IBusEngineDesc  *engine);

G_END_DECLS
#endif

