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

#define GDK_DISABLE_DEPRECATION_WARNINGS
#include <gtk/gtk.h>
#undef GDK_DISABLE_DEPRECATION_WARNINGS

#include "iconwidget.h"

#define IBUS_THEMED_RGBA_GET_PRIVATE(o)  \
   ((IBusThemedRGBAPrivate *)ibus_themed_rgba_get_instance_private (o))

enum {
    PROP_0 = 0,
    PROP_STYLE_CONTEXT,
    PROP_STYLE,
};

typedef struct _IBusThemedRGBAPrivate IBusThemedRGBAPrivate;
struct _IBusThemedRGBAPrivate {
#if GTK_CHECK_VERSION (2, 91, 0)
    GtkStyleContext *style_context;
#else
    GtkStyle        *style;
#endif
};

static void ibus_themed_rgba_set_property (IBusThemedRGBA *rgba,
                                           guint           prop_id,
                                           const GValue   *value,
                                           GParamSpec     *pspec);
static void ibus_themed_rgba_get_property (IBusThemedRGBA *rgba,
                                           guint           prop_id,
                                           GValue         *value,
                                           GParamSpec     *pspec);
static void ibus_themed_rgba_destroy      (IBusObject     *object);

G_DEFINE_TYPE_WITH_PRIVATE (IBusThemedRGBA, ibus_themed_rgba, IBUS_TYPE_OBJECT)


static void
ibus_themed_rgba_class_init (IBusThemedRGBAClass *class)
{
    GObjectClass *gobject_class = G_OBJECT_CLASS (class);
    IBusObjectClass *ibus_object_class = IBUS_OBJECT_CLASS (class);

    gobject_class->set_property =
            (GObjectSetPropertyFunc)ibus_themed_rgba_set_property;
    gobject_class->get_property =
            (GObjectGetPropertyFunc)ibus_themed_rgba_get_property;
    ibus_object_class->destroy = ibus_themed_rgba_destroy;

#if GTK_CHECK_VERSION (2, 91, 0)
    /**
     * IBusThemedRGBA:style-context:
     *
     * The GtkStyleContext of property
     */
    g_object_class_install_property (gobject_class,
            PROP_STYLE_CONTEXT,
            g_param_spec_object ("style-context",
                    "style context",
                    "The style context of property",
                    GTK_TYPE_STYLE_CONTEXT,
                    G_PARAM_READWRITE | G_PARAM_CONSTRUCT));
#else
    /**
     * IBusThemedRGBA:style:
     *
     * The GtkStyle of property
     */
    g_object_class_install_property (gobject_class,
            PROP_STYLE,
            g_param_spec_pointer ("style",
                    "style",
                    "The style of property",
                    G_PARAM_READWRITE | G_PARAM_CONSTRUCT));
#endif
}


static void
ibus_themed_rgba_init (IBusThemedRGBA *rgba)
{
}


#if GTK_CHECK_VERSION (2, 91, 0)
static IBusRGBA *
ibus_rgba_new_with_gdk_rgba (GdkRGBA *gdk_rgba)
{
    IBusRGBA *ibus_rgba;

    g_return_val_if_fail (gdk_rgba, NULL);
    ibus_rgba = g_slice_new (IBusRGBA);
    ibus_rgba->red = gdk_rgba->red;
    ibus_rgba->green = gdk_rgba->green;
    ibus_rgba->blue = gdk_rgba->blue;
    ibus_rgba->alpha = gdk_rgba->alpha;
    return ibus_rgba;
}


static IBusRGBA *
ibus_rgba_new_with_style_context (GtkStyleContext *style_context,
                                  const gchar     *theme_name,
                                  const gchar     *regacy_theme_name,
                                  const gchar     *fallback_rgba)
{
    GdkRGBA color = { 0, };
    if (gtk_style_context_lookup_color (style_context, theme_name, &color)) {
        ;
    } else if (gtk_style_context_lookup_color (style_context,
                                               regacy_theme_name,
                                               &color)) {
        ;
    } else if (fallback_rgba) {
        gdk_rgba_parse (&color, fallback_rgba);
    }
    return ibus_rgba_new_with_gdk_rgba (&color);
}

#else

static IBusRGBA *
ibus_rgba_new_with_gdk_color (GdkColor *gdk_color)
{
    IBusRGBA *ibus_rgba;

    g_return_val_if_fail (gdk_color, NULL);
    ibus_rgba = g_slice_new (IBusRGBA);
    ibus_rgba->red = (float)gdk_color->red / 0xffff;
    ibus_rgba->green = (float)gdk_color->green / 0xffff;
    ibus_rgba->blue = (float)gdk_color->blue / 0xffff;
    ibus_rgba->alpha = 1.; /* (float)gdk_color->pixel / 0xffff */
    return ibus_rgba;
}
#endif


static void
ibus_themed_rgba_set_property (IBusThemedRGBA *rgba,
                               guint           prop_id,
                               const GValue   *value,
                               GParamSpec     *pspec)
{
    IBusThemedRGBAPrivate *priv;

    g_return_if_fail (IBUS_THEMED_RGBA (rgba));
    priv = IBUS_THEMED_RGBA_GET_PRIVATE (rgba);
    switch (prop_id) {
#if GTK_CHECK_VERSION (2, 91, 0)
    case PROP_STYLE_CONTEXT:
        if (priv->style_context)
            g_object_unref (priv->style_context);
        priv->style_context = g_value_get_object (value);
        if (priv->style_context) {
            g_object_ref (priv->style_context);
            ibus_themed_rgba_get_colors (rgba);
        }
        break;
#else
    case PROP_STYLE:
        priv->style = g_value_get_pointer (value);
        if (priv->style)
            ibus_themed_rgba_get_colors (rgba);
        break;
#endif
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (rgba, prop_id, pspec);
    }
}


static void
ibus_themed_rgba_get_property (IBusThemedRGBA *rgba,
                               guint           prop_id,
                               GValue         *value,
                               GParamSpec     *pspec)
{
    IBusThemedRGBAPrivate *priv;

    g_return_if_fail (IBUS_THEMED_RGBA (rgba));
    priv = IBUS_THEMED_RGBA_GET_PRIVATE (rgba);
    switch (prop_id) {
#if GTK_CHECK_VERSION (2, 91, 0)
    case PROP_STYLE_CONTEXT:
        if (priv->style_context) {
            g_value_set_object (value,
                                g_object_ref (priv->style_context));
        }
        break;
#else
    case PROP_STYLE:
        if (priv->style)
            g_value_set_pointer (value, priv->style);
        break;
#endif
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (rgba, prop_id, pspec);
    }
}


static void
ibus_themed_rgba_destroy (IBusObject *object)
{
    IBusThemedRGBA *rgba = (IBusThemedRGBA *)object;
    IBusThemedRGBAPrivate *priv;

    g_return_if_fail (IBUS_IS_THEMED_RGBA (rgba));
    priv = IBUS_THEMED_RGBA_GET_PRIVATE (rgba);
#if GTK_CHECK_VERSION (2, 91, 0)
    g_clear_object (&priv->style_context);
#else
    priv->style = NULL;
#endif

    if (rgba->normal_fg) {
        g_slice_free (IBusRGBA, rgba->normal_fg);
        rgba->normal_fg = NULL;
    }
    if (rgba->normal_bg) {
        g_slice_free (IBusRGBA, rgba->normal_bg);
        rgba->normal_bg = NULL;
    }
    if (rgba->selected_fg) {
        g_slice_free (IBusRGBA, rgba->selected_fg);
        rgba->selected_fg = NULL;
    }
    if (rgba->selected_bg) {
        g_slice_free (IBusRGBA, rgba->selected_bg);
        rgba->selected_bg = NULL;
    }
}


IBusThemedRGBA *
#if GTK_CHECK_VERSION (2, 91, 0)
ibus_themed_rgba_new (GtkStyleContext *style_context)
#else
ibus_themed_rgba_new (GtkStyle        *style)
#endif
{
    return (IBusThemedRGBA *)g_object_new (IBUS_TYPE_THEMED_RGBA,
#if GTK_CHECK_VERSION (2, 91, 0)
                                           "style-context",
                                           style_context,
#else
                                           "style",
                                           style,
#endif
                                           NULL);
}


void
ibus_themed_rgba_get_colors (IBusThemedRGBA *rgba)
{
    IBusThemedRGBAPrivate *priv;

    g_return_if_fail (IBUS_IS_THEMED_RGBA (rgba));
    priv = IBUS_THEMED_RGBA_GET_PRIVATE (rgba);
    if (rgba->normal_fg) {
        g_slice_free (IBusRGBA, rgba->normal_fg);
        rgba->normal_fg = NULL;
    }
    if (rgba->normal_bg) {
        g_slice_free (IBusRGBA, rgba->normal_bg);
        rgba->normal_bg = NULL;
    }
    if (rgba->selected_fg) {
        g_slice_free (IBusRGBA, rgba->selected_fg);
        rgba->selected_fg = NULL;
    }
    if (rgba->selected_bg) {
        g_slice_free (IBusRGBA, rgba->selected_bg);
        rgba->selected_bg = NULL;
    }
#if GTK_CHECK_VERSION (2, 91, 0)
    rgba->normal_fg = ibus_rgba_new_with_style_context (priv->style_context,
                                                        "theme_fg_color",
                                                        "fg_normal",
                                                        "#2e3436");
    rgba->normal_bg = ibus_rgba_new_with_style_context (priv->style_context,
                                                        "theme_bg_color",
                                                        "base_normal",
                                                        "#f6f5f4");
    rgba->selected_fg =
            ibus_rgba_new_with_style_context (priv->style_context,
                                              "theme_selected_fg_color",
                                              "fg_selected",
                                              "#ffffff");
    rgba->selected_bg =
            ibus_rgba_new_with_style_context (priv->style_context,
                                              "theme_selected_bg_color",
                                              "base_selected",
                                              "#3584e4");
#else
    rgba->normal_fg = ibus_rgba_new_with_gdk_color (
            &priv->style->fg[GTK_STATE_NORMAL]);
    rgba->normal_bg = ibus_rgba_new_with_gdk_color (
            &priv->style->base[GTK_STATE_NORMAL]);
    rgba->selected_fg = ibus_rgba_new_with_gdk_color (
            &priv->style->fg[GTK_STATE_SELECTED]);
    rgba->selected_bg = ibus_rgba_new_with_gdk_color (
            &priv->style->base[GTK_STATE_SELECTED]);
#endif
}
