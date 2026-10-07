/* -*- mode: C; c-basic-offset: 4; indent-tabs-mode: nil; -*- */
/* vim:set et sts=4: */
/* ibus - The Input Bus
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

#ifndef __IBUS_FACTORY_H_
#define __IBUS_FACTORY_H_

/**
 * SECTION: ibusfactory
 * @short_description: Factory for creating engine instances.
 * @title: IBusFactory
 * @stability: Stable
 *
 * An IBusFactory is an #IBusService that creates input method engine (IME) instance.
 * It provides CreateEngine remote method, which creates an IME instance by name,
 * and returns the D-Bus object path to IBus daemon.
 *
 * see_also: #IBusEngine
 *
 */

#include "ibusservice.h"
#include "ibusserializable.h"
#include "ibusengine.h"

G_BEGIN_DECLS

/*
 * Type macros.
 */

/* define GOBJECT macros */
/**
 * IBUS_TYPE_FACTORY:
 *
 * Return GType of IBus factory.
 */
#define IBUS_TYPE_FACTORY               \
    (ibus_factory_get_type ())

/**
 * IBUS_FACTORY:
 * @obj: An object which is subject to casting.
 *
 * Casts an IBUS_FACTORY or derived pointer into a (IBusFactory*) pointer.
 * Depending on the current debugging level, this function may invoke
 * certain runtime checks to identify invalid casts.
 */
#define IBUS_FACTORY(obj)               \
    (G_TYPE_CHECK_INSTANCE_CAST ((obj), IBUS_TYPE_FACTORY, IBusFactory))

/**
 * IBUS_FACTORY_CLASS:
 * @klass: A class to be casted.
 *
 * Casts a derived IBusFactoryClass structure into a IBusFactoryClass structure.
 */
#define IBUS_FACTORY_CLASS(klass)       \
    (G_TYPE_CHECK_CLASS_CAST ((klass), IBUS_TYPE_FACTORY, IBusFactoryClass))

/**
 * IBUS_IS_FACTORY:
 * @obj: Instance to check for being a IBUS_FACTORY.
 *
 * Checks whether a valid GTypeInstance pointer is of type IBUS_FACTORY.
 */
#define IBUS_IS_FACTORY(obj)            \
    (G_TYPE_CHECK_INSTANCE_TYPE ((obj), IBUS_TYPE_FACTORY))

/**
 * IBUS_IS_FACTORY_CLASS:
 * @klass: A class to be checked.
 *
 * Checks whether class "is a" valid IBusFactoryClass structure of type IBUS_FACTORY or derived.
 */
#define IBUS_IS_FACTORY_CLASS(klass)    \
    (G_TYPE_CHECK_CLASS_TYPE ((klass), IBUS_TYPE_FACTORY))

/**
 * IBUS_FACTORY_GET_CLASS:
 * @obj: An object.
 *
 * Get the class of a given object and cast the class to IBusFactoryClass.
 */
#define IBUS_FACTORY_GET_CLASS(obj)     \
    (G_TYPE_INSTANCE_GET_CLASS ((obj), IBUS_TYPE_FACTORY, IBusFactoryClass))

typedef struct _IBusFactory IBusFactory;
typedef struct _IBusFactoryClass IBusFactoryClass;
typedef struct _IBusFactoryPrivate IBusFactoryPrivate;

/**
 * IBusFactory:
 *
 * An opaque data type representing an IBusFactory.
 */
struct _IBusFactory {
    /*< private >*/
    IBusService parent;
    IBusFactoryPrivate *priv;

    /* instance members */
};

struct _IBusFactoryClass {
    /*< private >*/
    IBusServiceClass parent;

    /*< public >*/
    /* signals */
    IBusEngine *
                (* create_engine)
                                    (IBusFactory    *factory,
                                     const gchar    *engine_name);

    /*< private >*/
    /* padding */
    gpointer pdummy[7];
};

/**
 * ibus_factory_info_get_type:
 *
 * Gets GType of IBus factory information.
 *
 * Returns: GType of IBus factory information.
 */
GType            ibus_factory_get_type          (void);

/**
 * ibus_factory_new:
 * @connection: An GDBusConnection.
 *
 * Creates a new #IBusFactory.
 *
 * Returns: A newly allocated #IBusFactory.
 */
IBusFactory     *ibus_factory_new               (GDBusConnection *connection);

/**
 * ibus_factory_add_engine:
 * @factory: An IBusFactory.
 * @engine_name: Name of an engine.
 * @engine_type: GType of an engine.
 *
 * Add an engine to the factory.
 */
void             ibus_factory_add_engine        (IBusFactory    *factory,
                                                 const gchar    *engine_name,
                                                 GType           engine_type);

/**
 * ibus_factory_create_engine:
 * @factory: An #IBusFactory.
 * @engine_name: Name of an engine.
 *
 * Creates an #IBusEngine with @engine_name.
 *
 * Returns: (transfer full): #IBusEngine with @engine_name.
 */
IBusEngine      *ibus_factory_create_engine     (IBusFactory    *factory,
                                                 const gchar    *engine_name);

G_END_DECLS
#endif

