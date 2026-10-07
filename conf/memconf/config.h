/* -*- mode: C; c-basic-offset: 4; indent-tabs-mode: nil; -*- */
/* vim:set et sts=4: */
/* ibus - The Input Bus
 * Copyright (c) 2010, Google Inc. All rights reserved.
 * Copyright (C) 2010 Peng Huang <shawn.p.huang@gmail.com>
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
#ifndef __CONFIG_MEMCONF_H__
#define __CONFIG_MEMCONF_H__

#include <ibus.h>

#define IBUS_TYPE_CONFIG_MEMCONF	    \
	(ibus_config_memconf_get_type ())
#define IBUS_CONFIG_MEMCONF(obj)            \
    (G_TYPE_CHECK_INSTANCE_CAST ((obj), IBUS_TYPE_CONFIG_MEMCONF, IBusConfigMemconf))
#define IBUS_CONFIG_MEMCONF_CLASS(klass)     \
    (G_TYPE_CHECK_CLASS_CAST ((klass), IBUS_TYPE_CONFIG_MEMCONF, IBusConfigMemconfClass))
#define IBUS_IS_CONFIG_MEMCONF(obj)          \
    (G_TYPE_CHECK_INSTANCE_TYPE ((obj), IBUS_TYPE_CONFIG_MEMCONF))
#define IBUS_IS_CONFIG_MEMCONF_CLASS(klass)  \
    (G_TYPE_CHECK_CLASS_TYPE ((klass), IBUS_TYPE_CONFIG_MEMCONF))
#define IBUS_CONFIG_MEMCONF_GET_CLASS(obj)   \
    (G_TYPE_INSTANCE_GET_CLASS ((obj), IBUS_TYPE_CONFIG_MEMCONF, IBusConfigMemconfClass))

typedef struct _IBusConfigMemconf IBusConfigMemconf;

GType                ibus_config_memconf_get_type   (void);
IBusConfigMemconf   *ibus_config_memconf_new        (GDBusConnection    *connection);

#endif
