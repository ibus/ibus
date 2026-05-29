/* -*- mode: C; c-basic-offset: 4; indent-tabs-mode: nil; -*- */
/* vim:set et sts=4: */
/* ibus - The Input Bus
 * Copyright (C) 2008-2010 Peng Huang <shawn.p.huang@gmail.com>
 * Copyright (C) 2008-2010 Red Hat, Inc.
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

#ifndef __IBUS_INTERNEL_H_
#define __IBUS_INTERNEL_H_

#include <glib.h>
/**
 * I_:
 * @string: A string
 * @returns: The canonical representation for the string.
 *
 * Returns a canonical representation for string.
 * Interned strings can be compared for equality by comparing the pointers, instead of using strcmp().
 */
#define I_(string) g_intern_static_string (string)

/**
 * DBUS_SERVICE_DBUS:
 *
 * Address of D-Bus service.
 */
#define DBUS_SERVICE_DBUS "org.freedesktop.DBus"

/**
 * DBUS_PATH_DBUS:
 *
 * D-Bus path for D-Bus.
 */
#define DBUS_PATH_DBUS "/org/freedesktop/DBus"

/**
 * DBUS_INTERFACE_DBUS:
 *
 * D-Bus interface for D-Bus.
 */
#define DBUS_INTERFACE_DBUS "org.freedesktop.DBus"

G_GNUC_INTERNAL void
ibus_g_variant_get_child_string (GVariant *variant, gsize index, char **str);

#ifdef IBUS_KEY_dead_grave
#ifdef IBUS_KEY_dead_longsolidusoverlay
/* Checks if a keysym is a dead key. Dead key keysym values are defined in
 * ibuskeysyms.h and the first is GDK_KEY_dead_grave.
 */
#define IS_DEAD_KEY(k) \
      ((k) >= IBUS_KEY_dead_grave && (k) <= IBUS_KEY_dead_longsolidusoverlay)
#endif /* IBUS_KEY_dead_longsolidusoverlay */
#endif /* IBUS_KEY_dead_grave */

#endif
