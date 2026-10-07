/* -*- mode: C; c-basic-offset: 4; indent-tabs-mode: nil; -*- */
/* vim:set et sts=4: */
/* ibus - The Input Bus
 * Copyright (C) 2011 Peng Huang <shawn.p.huang@gmail.com>
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

#include "ibuserror.h"

#include <gio/gio.h>
#include "ibustypes.h"

static const GDBusErrorEntry ibus_error_entries[] =
{
    { IBUS_ERROR_NO_ENGINE,         "org.freedesktop.IBus.Error.NoEngine" },
};

GQuark
ibus_error_quark (void)
{
    static volatile gsize quark_volatile = 0;
    g_dbus_error_register_error_domain ("ibus-error-quark",
                                        &quark_volatile,
                                        ibus_error_entries,
                                        G_N_ELEMENTS (ibus_error_entries));
    return (GQuark) quark_volatile;
}
