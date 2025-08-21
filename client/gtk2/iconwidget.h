/* -*- mode: C; c-basic-offset: 4; indent-tabs-mode: nil; -*- */
/* vim:set et sts=4: */
/* ibus - The Input Bus
 * Copyright (C) 2025 Takao Fujiwara <takao.fujiwara1@gmail.com>
 * Copyright (C) 2025 Red Hat, Inc.
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

#include <gtk/gtk.h>
#include <ibus.h>

#ifndef __IBUS_THEMED_RGBA_H_
#define __IBUS_THEMED_RGBA_H_

#define IBUS_TYPE_THEMED_RGBA                   \
    (ibus_themed_rgba_get_type ())
#define IBUS_THEMED_RGBA(obj)                   \
    (G_TYPE_CHECK_INSTANCE_CAST ((obj), IBUS_TYPE_THEMED_RGBA, IBusThemedRGBA))
#define IBUS_THEMED_RGBA_CLASS(klass)           \
    (G_TYPE_CHECK_CLASS_CAST ((klass),          \
                              IBUS_TYPE_THEMED_RGBA, \
                              IBusThemedRGBAClass))
#define IBUS_IS_THEMED_RGBA(obj)                \
    (G_TYPE_CHECK_INSTANCE_TYPE ((obj), IBUS_TYPE_THEMED_RGBA))
#define IBUS_IS_THEMED_RGBA_CLASS(klass)        \
    (G_TYPE_CHECK_CLASS_TYPE ((klass), IBUS_TYPE_THEMED_RGBA))
#define IBUS_THEMED_RGBA_GET_CLASS(obj)         \
    (G_TYPE_INSTANCE_GET_CLASS ((obj),          \
                                IBUS_TYPE_THEMED_RGBA, \
                                IBusThemedRGBAClass))

typedef struct _IBusThemedRGBA IBusThemedRGBA;
typedef struct _IBusThemedRGBAClass IBusThemedRGBAClass;

struct _IBusThemedRGBA {
    IBusObject parent;
    /* instance members */
    IBusRGBA *normal_fg;
    IBusRGBA *normal_bg;
    IBusRGBA *selected_fg;
    IBusRGBA *selected_bg;
};

struct _IBusThemedRGBAClass {
    IBusObjectClass parent;
};

GType            ibus_themed_rgba_get_type     (void);
#if GTK_CHECK_VERSION (2, 91, 0)
IBusThemedRGBA  *ibus_themed_rgba_new          (GtkStyleContext *style_context);
#else
IBusThemedRGBA  *ibus_themed_rgba_new          (GtkStyle        *style);
#endif
void             ibus_themed_rgba_get_colors   (IBusThemedRGBA  *rgba);

#endif
