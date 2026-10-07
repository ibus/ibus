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
#include "component.h"

#include <gio/gio.h>
#include <glib/gstdio.h>
#include <stdlib.h>
#include <string.h>

#include "global.h"
#include "marshalers.h"
#include "types.h"

enum {
    LAST_SIGNAL,
};

enum {
    PROP_0 = 0,
    PROP_COMPONENT,
    PROP_FACTORY,
};

struct _BusComponent {
    IBusObject parent;

    /* instance members */

    /* an object which represents one XML file in the ibus/component/ directory. */
    IBusComponent *component;
    /* a proxy object which starts an engine. */
    BusFactoryProxy *factory;

    /* TRUE if the component started in the verbose mode. */
    gboolean verbose;
    /* TRUE if the component needs to be restarted when it dies. */
    gboolean restart;
    /* TRUE if the component will be destroyed with factory. */
    gboolean destroy_with_factory;

    /* process id of the process (e.g. ibus-config, ibus-engine-*, ..) of the component. */
    GPid     pid;
    guint    child_source_id;
};

struct _BusComponentClass {
    IBusObjectClass parent;
    /* class members */
};

/* functions prototype */
static GObject* bus_component_constructor   (GType                  type,
                                             guint                  n_construct_params,
                                             GObjectConstructParam *construct_params);
static void     bus_component_set_property  (BusComponent          *component,
                                             guint                  prop_id,
                                             const GValue          *value,
                                             GParamSpec            *pspec);
static void     bus_component_get_property  (BusComponent          *component,
                                             guint                  prop_id,
                                             GValue                *value,
                                             GParamSpec            *pspec);
static void     bus_component_destroy       (BusComponent          *component);

G_DEFINE_TYPE (BusComponent, bus_component, IBUS_TYPE_OBJECT)

static void
bus_component_class_init (BusComponentClass *class)
{
    GObjectClass *gobject_class = G_OBJECT_CLASS (class);
    IBusObjectClass *ibus_object_class = IBUS_OBJECT_CLASS (class);

    gobject_class->constructor  = bus_component_constructor;
    gobject_class->set_property = (GObjectSetPropertyFunc) bus_component_set_property;
    gobject_class->get_property = (GObjectGetPropertyFunc) bus_component_get_property;
    ibus_object_class->destroy  = (IBusObjectDestroyFunc) bus_component_destroy;

    /* install properties */
    g_object_class_install_property (gobject_class,
                    PROP_COMPONENT,
                    g_param_spec_object ("component", /* canonical name of the property */
                        "component", /* nick name */
                        "component", /* description */
                        IBUS_TYPE_COMPONENT, /* object type */
                        G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));

    g_object_class_install_property (gobject_class,
                    PROP_FACTORY,
                    g_param_spec_object ("factory",
                        "factory",
                        "factory",
                        BUS_TYPE_FACTORY_PROXY,
                        G_PARAM_READWRITE));
}

static void
bus_component_init (BusComponent *component)
{
}

/**
 * bus_component_constructor:
 *
 * A constructor method which is called after bus_component_init is called.
 */
static GObject*
bus_component_constructor (GType                  type,
                           guint                  n_construct_params,
                           GObjectConstructParam *construct_params)
{
    GObject *object;
    object = G_OBJECT_CLASS (bus_component_parent_class)->constructor (type,
                                                                       n_construct_params,
                                                                       construct_params);
    BusComponent *component = (BusComponent *) object;
    /* we have to override the _constructor method since in _init method, the component->component property is not set yet. */
    g_assert (IBUS_IS_COMPONENT (component->component));

    static GQuark quark = 0;
    if (quark == 0) {
        quark = g_quark_from_static_string ("BusComponent");
    }

    /* associate each engine with BusComponent. a component might have one or more components. For example, ibus-engine-pinyin would
     * have two - 'pinyin' and 'bopomofo' and ibus-engine-m17n has many. On the other hand, the gtkpanel component does not have an
     * engine, of course. */
    GList *engines = ibus_component_get_engines (component->component);
    GList *p;
    for (p = engines; p != NULL; p = p->next) {
        g_object_set_qdata ((GObject *) p->data, quark, component);
    }
    g_list_free (engines);

    return object;
}

static void
bus_component_set_property (BusComponent *component,
                            guint         prop_id,
                            const GValue *value,
                            GParamSpec   *pspec)
{
    switch (prop_id) {
    case PROP_COMPONENT:
        g_assert (component->component == NULL);
        component->component = g_value_dup_object (value);
        break;
    case PROP_FACTORY:
        bus_component_set_factory (component, (BusFactoryProxy *) g_value_get_object (value));
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (component, prop_id, pspec);
    }
}

static void
bus_component_get_property (BusComponent *component,
                            guint         prop_id,
                            GValue       *value,
                            GParamSpec   *pspec)
{
    switch (prop_id) {
    case PROP_COMPONENT:
        g_value_set_object (value, bus_component_get_component (component));
        break;
    case PROP_FACTORY:
        g_value_set_object (value, bus_component_get_factory (component));
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (component, prop_id, pspec);
    }
}

static void
bus_component_destroy (BusComponent *component)
{
    if (component->pid != 0) {
        bus_component_stop (component);
        g_spawn_close_pid (component->pid);
        component->pid = 0;
    }

    if (component->child_source_id != 0) {
        g_source_remove (component->child_source_id);
        component->child_source_id = 0;
    }

    if (component->component != NULL) {
        g_object_unref (component->component);
        component->component = NULL;
    }

    IBUS_OBJECT_CLASS (bus_component_parent_class)->destroy (IBUS_OBJECT (component));
}

BusComponent *
bus_component_new (IBusComponent   *component,
                   BusFactoryProxy *factory)
{
    g_assert (IBUS_IS_COMPONENT (component));

    return (BusComponent *) g_object_new (BUS_TYPE_COMPONENT,
                                          /* properties below will be set via the bus_component_set_property function. */
                                          "component", component,
                                          "factory", factory,
                                          NULL);
}

static void
bus_component_factory_destroy_cb (BusFactoryProxy *factory,
                                  BusComponent    *component)
{
    g_return_if_fail (component->factory == factory);

    g_object_unref (component->factory);
    component->factory = NULL;
    /* emit the "notify" signal for the factory property on component. */
    g_object_notify ((GObject *) component, "factory");

    if (component->destroy_with_factory)
        ibus_object_destroy ((IBusObject *) component);
}

IBusComponent *
bus_component_get_component (BusComponent *component)
{
    g_assert (BUS_IS_COMPONENT (component));
    return component->component;
}

void
bus_component_set_factory (BusComponent    *component,
                           BusFactoryProxy *factory)
{
    g_assert (BUS_IS_COMPONENT (component));

    if (component->factory == factory) {
        return;
    }

    if (component->factory) {
        g_signal_handlers_disconnect_by_func (component->factory,
                                              bus_component_factory_destroy_cb,
                                              component);
        g_object_unref (component->factory);
        component->factory = NULL;
    }

    if (factory) {
        g_assert (BUS_IS_FACTORY_PROXY (factory));
        component->factory = (BusFactoryProxy *) g_object_ref (factory);
        g_signal_connect (factory, "destroy",
                          G_CALLBACK (bus_component_factory_destroy_cb), component);
    }

    /* emit the "notify" signal for the factory property on component. */
    g_object_notify ((GObject*) component, "factory");
}

BusFactoryProxy *
bus_component_get_factory (BusComponent *component)
{
    g_assert (BUS_IS_COMPONENT (component));
    return component->factory;
}

const gchar *
bus_component_get_name (BusComponent *component)
{
    g_assert (BUS_IS_COMPONENT (component));

    return ibus_component_get_name (component->component);
}

GList *
bus_component_get_engines (BusComponent *component)
{
    g_assert (BUS_IS_COMPONENT (component));

    return ibus_component_get_engines (component->component);
}

void
bus_component_set_destroy_with_factory (BusComponent *component,
                                        gboolean      with_factory)
{
    g_assert (BUS_IS_COMPONENT (component));

    component->destroy_with_factory = with_factory;
}

void
bus_component_set_restart (BusComponent *component,
                           gboolean      restart)
{
    g_assert (BUS_IS_COMPONENT (component));
    component->restart = restart;
}

/**
 * bus_component_child_cb:
 *
 * A callback function to be called when the child process is terminated.
 */
static void
bus_component_child_cb (GPid          pid,
                        gint          status,
                        BusComponent *component)
{
    g_assert (BUS_IS_COMPONENT (component));
    g_assert (component->pid == pid);
    g_spawn_close_pid (pid);
    component->pid = 0;
    component->child_source_id = 0;

    if (component->restart) {
        bus_component_start (component, component->verbose);
    }
}

gboolean
bus_component_start (BusComponent *component,
                     gboolean      verbose)
{
    g_assert (BUS_IS_COMPONENT (component));

    if (component->pid != 0)
        return TRUE;

    component->verbose = verbose;

    gint argc;
    gchar **argv;
    gboolean retval;

    GError *error = NULL;
    if (!g_shell_parse_argv (ibus_component_get_exec (component->component),
                             &argc,
                             &argv,
                             &error)) {
        g_warning ("Can not parse component %s exec: %s",
                   ibus_component_get_name (component->component),
                   error->message);
        g_error_free (error);
        return FALSE;
    }

    error = NULL;
    GSpawnFlags flags = G_SPAWN_DO_NOT_REAP_CHILD;
    if (!verbose) {
        flags |= G_SPAWN_STDOUT_TO_DEV_NULL | G_SPAWN_STDERR_TO_DEV_NULL;
    }
    retval = g_spawn_async (NULL, argv, NULL,
                            flags,
                            NULL, NULL,
                            &(component->pid), &error);
    g_strfreev (argv);
    if (!retval) {
        g_warning ("Can not execute component %s: %s",
                   ibus_component_get_name (component->component),
                   error->message);
        g_error_free (error);
        return FALSE;
    }

    component->child_source_id =
        g_child_watch_add (component->pid,
                           (GChildWatchFunc) bus_component_child_cb,
                           component);

    return TRUE;
}

gboolean
bus_component_stop (BusComponent *component)
{
    g_assert (BUS_IS_COMPONENT (component));

    if (component->pid == 0)
        return TRUE;

    kill (component->pid, SIGTERM);
    return TRUE;
}

gboolean
bus_component_is_running (BusComponent *component)
{
    g_assert (BUS_IS_COMPONENT (component));

    return (component->pid != 0);
}

BusComponent *
bus_component_from_engine_desc (IBusEngineDesc *engine)
{
    g_assert (IBUS_IS_ENGINE_DESC (engine));

    static GQuark quark = 0;
    if (quark == 0) {
        quark = g_quark_from_static_string ("BusComponent");
    }

    return (BusComponent *) g_object_get_qdata ((GObject *) engine, quark);
}
