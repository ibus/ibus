/* -*- mode: C; c-basic-offset: 4; indent-tabs-mode: nil; -*- */
/* vim:set et sts=4: */
/* ibus - The Input Bus
 * Copyright (C) 2008-2010 Peng Huang <shawn.p.huang@gmail.com>
 * Copyright (C) 2008-2010 Red Hat, Inc.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */
#ifndef __CONFIG_DCONF_H__
#define __CONFIG_DCONF_H__

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <ibus.h>
#ifdef DCONF_0_13_4
#  include <client/dconf-client.h>
#  include <common/dconf-paths.h>
#else
#  include <dconf/dconf.h>
#endif

#define IBUS_TYPE_CONFIG_DCONF            \
    (ibus_config_dconf_get_type ())
#define IBUS_CONFIG_DCONF(obj)            \
    (G_TYPE_CHECK_INSTANCE_CAST ((obj), IBUS_TYPE_CONFIG_DCONF, IBusConfigDConf))
#define IBUS_CONFIG_DCONF_CLASS(klass)     \
    (G_TYPE_CHECK_CLASS_CAST ((klass), IBUS_TYPE_CONFIG_DCONF, IBusConfigDConfClass))
#define IBUS_IS_CONFIG_DCONF(obj)          \
    (G_TYPE_CHECK_INSTANCE_TYPE ((obj), IBUS_TYPE_CONFIG_DCONF))
#define IBUS_IS_CONFIG_DCONF_CLASS(klass)  \
    (G_TYPE_CHECK_CLASS_TYPE ((klass), IBUS_TYPE_CONFIG_DCONF))
#define IBUS_CONFIG_DCONF_GET_CLASS(obj)   \
    (G_TYPE_INSTANCE_GET_CLASS ((obj), IBUS_TYPE_CONFIG_DCONF, IBusConfigDConfClass))

typedef struct _IBusConfigDConf IBusConfigDConf;
typedef struct _IBusConfigDConfClass IBusConfigDConfClass;

GType            ibus_config_dconf_get_type (void);
IBusConfigDConf *ibus_config_dconf_new      (GDBusConnection *connection);

#endif
