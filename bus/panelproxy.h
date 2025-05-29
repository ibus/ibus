/* -*- mode: C; c-basic-offset: 4; indent-tabs-mode: nil; -*- */
/* vim:set et sts=4: */
/* ibus - The Input Bus
 * Copyright (C) 2008-2014 Peng Huang <shawn.p.huang@gmail.com>
 * Copyright (C) 2017-2025 Takao Fujiwara <takao.fujiwara1@gmail.com>
 * Copyright (C) 2008-2025 Red Hat, Inc.
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
#ifndef __BUS_PANEL_PROXY_H_
#define __BUS_PANEL_PROXY_H_

#include <ibus.h>
#include "connection.h"
#include "inputcontext.h"

/*
 * Type macros.
 */

/* define GOBJECT macros */
#define BUS_TYPE_PANEL_PROXY             \
    (bus_panel_proxy_get_type ())
#define BUS_PANEL_PROXY(obj)             \
    (G_TYPE_CHECK_INSTANCE_CAST ((obj), BUS_TYPE_PANEL_PROXY, BusPanelProxy))
#define BUS_PANEL_PROXY_CLASS(klass)     \
    (G_TYPE_CHECK_CLASS_CAST ((klass),   \
                              BUS_TYPE_PANEL_PROXY, \
                              BusPanelProxyClass))
#define BUS_IS_PANEL_PROXY(obj)          \
    (G_TYPE_CHECK_INSTANCE_TYPE ((obj), BUS_TYPE_PANEL_PROXY))
#define BUS_IS_PANEL_PROXY_CLASS(klass)  \
    (G_TYPE_CHECK_CLASS_TYPE ((klass), BUS_TYPE_PANEL_PROXY))
#define BUS_PANEL_PROXY_GET_CLASS(obj)   \
    (G_TYPE_INSTANCE_GET_CLASS ((obj),   \
                                BUS_TYPE_PANEL_PROXY, \
                                BusPanelProxyClass))

G_BEGIN_DECLS

typedef enum
{
    PANEL_TYPE_NONE,
    PANEL_TYPE_PANEL,
    PANEL_TYPE_EXTENSION_EMOJI
} PanelType;

typedef struct _BusPanelProxy BusPanelProxy;
typedef struct _BusPanelProxyClass BusPanelProxyClass;

GType            bus_panel_proxy_get_type      (void);
BusPanelProxy   *bus_panel_proxy_new           (BusConnection     *connection,
                                                PanelType          panel_type);

gboolean         bus_panel_proxy_send_signal   (BusPanelProxy   *panel,
                                                const gchar     *interface_name,
                                                const gchar     *signal_name,
                                                GVariant        *parameters,
                                                GError         **error);
/* functions that invoke D-Bus methods of the panel component. */
void             bus_panel_proxy_focus_in      (BusPanelProxy     *panel,
                                                BusInputContext   *context);
void             bus_panel_proxy_focus_out     (BusPanelProxy     *panel,
                                                BusInputContext   *context);
void             bus_panel_proxy_destroy_context
                                               (BusPanelProxy     *panel,
                                                BusInputContext   *context);
void             bus_panel_proxy_set_cursor_location
                                               (BusPanelProxy     *panel,
                                                gint32             x,
                                                gint32             y,
                                                gint32             w,
                                                gint32             h);
void             bus_panel_proxy_set_cursor_location_relative
                                               (BusPanelProxy     *panel,
                                                gint32             x,
                                                gint32             y,
                                                gint32             w,
                                                gint32             h);
void             bus_panel_proxy_update_preedit_text
                                               (BusPanelProxy     *panel,
                                                IBusText          *text,
                                                guint              cursor_pos,
                                                gboolean           visible);
void             bus_panel_proxy_show_preedit_text
                                               (BusPanelProxy     *panel);
void             bus_panel_proxy_hide_preedit_text
                                               (BusPanelProxy     *panel);
void             bus_panel_proxy_update_auxiliary_text
                                               (BusPanelProxy     *panel,
                                                IBusText          *text,
                                                gboolean           visible);
void             bus_panel_proxy_show_auxiliary_text
                                               (BusPanelProxy     *panel);
void             bus_panel_proxy_hide_auxiliary_text
                                               (BusPanelProxy     *panel);
void             bus_panel_proxy_update_lookup_table
                                               (BusPanelProxy     *panel,
                                                IBusLookupTable   *table,
                                                gboolean           visible);
void             bus_panel_proxy_show_lookup_table
                                               (BusPanelProxy     *panel);
void             bus_panel_proxy_hide_lookup_table
                                               (BusPanelProxy     *panel);
void             bus_panel_proxy_page_up_lookup_table
                                               (BusPanelProxy     *panel);
void             bus_panel_proxy_page_down_lookup_table
                                               (BusPanelProxy     *panel);
void             bus_panel_proxy_cursor_up_lookup_table
                                               (BusPanelProxy     *panel);
void             bus_panel_proxy_cursor_down_lookup_table
                                               (BusPanelProxy     *panel);
void             bus_panel_proxy_register_properties
                                               (BusPanelProxy     *panel,
                                                IBusPropList      *prop_list);
void             bus_panel_proxy_update_property
                                               (BusPanelProxy     *panel,
                                                IBusProperty      *prop);
void             bus_panel_proxy_set_content_type
                                               (BusPanelProxy     *panel,
                                                guint              purpose,
                                                guint              hints);
PanelType        bus_panel_proxy_get_panel_type
                                               (BusPanelProxy     *panel);
void             bus_panel_proxy_panel_extension_received
                                               (BusPanelProxy     *panel,
                                                IBusExtensionEvent
                                                                  *event);
void             bus_panel_proxy_process_key_event
                                               (BusPanelProxy     *panel,
                                                guint              keyval,
                                                guint              keycode,
                                                guint              state,
                                                GAsyncReadyCallback
                                                                   callback,
                                                gpointer           user_data);
void             bus_panel_proxy_commit_text_received
                                               (BusPanelProxy     *panel,
                                                IBusText          *text);
void             bus_panel_proxy_candidate_clicked_lookup_table
                                               (BusPanelProxy     *panel,
                                                guint              index,
                                                guint              button,
                                                guint              state);
void             bus_panel_proxy_send_message_received
                                               (BusPanelProxy     *panel,
                                                GVariant          *parameters);

G_END_DECLS
#endif

