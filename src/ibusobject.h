/* -*- mode: C; c-basic-offset: 4; indent-tabs-mode: nil; -*- */
/* vim:set et sts=4: */
/* ibus - The Input Bus
 * Copyright (C) 2008-2013 Peng Huang <shawn.p.huang@gmail.com>
 * Copyright (C) 2008-2013 Red Hat, Inc.
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

#if !defined (__IBUS_H_INSIDE__) && !defined (IBUS_COMPILATION)
#error "Only <ibus.h> can be included directly"
#endif

#ifndef __IBUS_OBJECT_H_
#define __IBUS_OBJECT_H_

/**
 * SECTION: ibusobject
 * @short_description: Base object of IBus.
 * @title: IBusObject
 * @stability: Stable
 *
 * IBusObject is the base object for all objects in IBus.
 */

#include <glib-object.h>
#include "ibustypes.h"
#include "ibusdebug.h"

/*
 * Type macros.
 */

/* define GOBJECT macros */
#define IBUS_TYPE_OBJECT             \
    (ibus_object_get_type ())
#define IBUS_OBJECT(obj)             \
    (G_TYPE_CHECK_INSTANCE_CAST ((obj), IBUS_TYPE_OBJECT, IBusObject))
#define IBUS_OBJECT_CLASS(klass)     \
    (G_TYPE_CHECK_CLASS_CAST ((klass), IBUS_TYPE_OBJECT, IBusObjectClass))
#define IBUS_IS_OBJECT(obj)          \
    (G_TYPE_CHECK_INSTANCE_TYPE ((obj), IBUS_TYPE_OBJECT))
#define IBUS_IS_OBJECT_CLASS(klass)  \
    (G_TYPE_CHECK_CLASS_TYPE ((klass), IBUS_TYPE_OBJECT))
#define IBUS_OBJECT_GET_CLASS(obj)   \
    (G_TYPE_INSTANCE_GET_CLASS ((obj), IBUS_TYPE_OBJECT, IBusObjectClass))

/**
 * IBusObjectFlags:
 * @IBUS_IN_DESTRUCTION: Used in GObjectClass::dispose
 * @IBUS_DESTROYED: Used during emitting IBusObject::destroy signal.
 * @IBUS_RESERVED_1: Reserved.
 * @IBUS_RESERVED_2: Reserved.
 *
 * The flags are used internally.
 */
typedef enum {
    IBUS_IN_DESTRUCTION = (1 << 0),
    IBUS_DESTROYED      = (1 << 1),
    IBUS_RESERVED_1     = (1 << 2),
    IBUS_RESERVED_2     = (1 << 3),
} IBusObjectFlags;

#define IBUS_OBJECT_FLAGS(obj)             (IBUS_OBJECT (obj)->flags)
#define IBUS_OBJECT_SET_FLAGS(obj,flag)    G_STMT_START{ (IBUS_OBJECT_FLAGS (obj) |= (flag)); }G_STMT_END
#define IBUS_OBJECT_UNSET_FLAGS(obj,flag)  G_STMT_START{ (IBUS_OBJECT_FLAGS (obj) &= ~(flag)); }G_STMT_END
#define IBUS_OBJECT_IN_DESTRUCTION(obj)    (IBUS_OBJECT_FLAGS (obj) & IBUS_IN_DESTRUCTION)
#define IBUS_OBJECT_DESTROYED(obj)         (IBUS_OBJECT_FLAGS (obj) & IBUS_DESTROYED)

G_BEGIN_DECLS

typedef struct _IBusObject IBusObject;
typedef struct _IBusObjectClass IBusObjectClass;
typedef struct _IBusObjectPrivate IBusObjectPrivate;

/**
 * IBusObject:
 *
 * All the fields in the <structname>IBusObject</structname> structure are
 * private to the #IBusObject and should never be accessed directly.
 */
struct _IBusObject {
    GInitiallyUnowned parent;
    /* instance members */
    guint32 flags;

    IBusObjectPrivate *priv;
};

typedef void ( *IBusObjectDestroyFunc) (IBusObject *object);

struct _IBusObjectClass {
    GInitiallyUnownedClass parent;

    /* signals */
    void (* destroy)        (IBusObject   *object);

    /*< private >*/
    /* padding */
    gpointer pdummy[7];
};

GType           ibus_object_get_type            (void);

/**
 * ibus_object_new:
 *
 * Creates  a new #IBusObject.
 *
 * Returns: A newly allocated #IBusObject
 */
IBusObject     *ibus_object_new                 (void);

/**
 * ibus_object_destroy:
 * @object: an #IBusObject to destroy.
 *
 * Emit the "destroy" signal notifying all reference holders that they should
 * release the #IBusObject.
 *
 * The memory for the object itself won't be deleted until its reference count
 * actually drops to 0; ibus_object_destroy merely asks reference holders to
 * release their references. It does not free the object.
 */
void            ibus_object_destroy             (IBusObject     *object);

G_END_DECLS
#endif

