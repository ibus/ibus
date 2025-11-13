/* -*- mode: C; c-basic-offset: 4; indent-tabs-mode: nil; -*- */
/* vim:set et sts=4: */
/* ibus - The Input Bus
 * Copyright (C) 2008-2013 Peng Huang <shawn.p.huang@gmail.com>
 * Copyright (C) 2015-2025 Takao Fujiwara <takao.fujiwara1@gmail.com>
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

#if !defined (__IBUS_H_INSIDE__) && !defined (IBUS_COMPILATION)
#error "Only <ibus.h> can be included directly"
#endif

#ifndef __IBUS_SHARE_H_
#define __IBUS_SHARE_H_

/**
 * SECTION: ibusshare
 * @short_description: Shared utility functions and definition.
 * @stability: Stable
 *
 * This file defines some utility functions and definition
 * which are shared among ibus component and services.
 */

#include <glib.h>

/**
 * IBUS_SERVICE_IBUS:
 *
 * Address of IBus service.
 */
#define IBUS_SERVICE_IBUS       "org.freedesktop.IBus"

/**
 * IBUS_SERVICE_PORTAL:
 *
 * Address of IBus portalservice.
 */
#define IBUS_SERVICE_PORTAL     "org.freedesktop.portal.IBus"

/**
 * IBUS_SERVICE_PANEL:
 *
 * Address of IBus panel service.
 */
#define IBUS_SERVICE_PANEL      "org.freedesktop.IBus.Panel"

/**
 * IBUS_SERVICE_PANEL_EXTENSION:
 *
 * Address of IBus panel extension service.
 */
#define IBUS_SERVICE_PANEL_EXTENSION "org.freedesktop.IBus.Panel.Extension"

/**
 * IBUS_SERVICE_PANEL_EXTENSION_EMOJI:
 *
 * Address of IBus panel extension service for emoji.
 * This service provides emoji, Unicode code point, Unicode name features.
 */
#define IBUS_SERVICE_PANEL_EXTENSION_EMOJI \
        "org.freedesktop.IBus.Panel.Extension.Emoji"

/**
 * IBUS_SERVICE_CONFIG:
 *
 * Address of IBus config service.
 */
#define IBUS_SERVICE_CONFIG     "org.freedesktop.IBus.Config"

/**
 * IBUS_SERVICE_NOTIFICATIONS:
 *
 * Address of IBus notification service.
 */
#define IBUS_SERVICE_NOTIFICATIONS    "org.freedesktop.IBus.Notifications"

/**
 * IBUS_PATH_IBUS:
 *
 * D-Bus path for IBus
 */
#define IBUS_PATH_IBUS          "/org/freedesktop/IBus"

/**
 * IBUS_PATH_FACTORY:
 *
 * D-Bus path for IBus factory.
 */
#define IBUS_PATH_FACTORY       "/org/freedesktop/IBus/Factory"

/**
 * IBUS_PATH_PANEL:
 *
 * D-Bus path for IBus panel.
 */
#define IBUS_PATH_PANEL         "/org/freedesktop/IBus/Panel"

/**
 * IBUS_PATH_PANEL_EXTENSION_EMOJI:
 *
 * D-Bus path for IBus extension panel for emoji.
 * This service provides emoji, Unicode code point, Unicode name features.
 */
#define IBUS_PATH_PANEL_EXTENSION_EMOJI \
        "/org/freedesktop/IBus/Panel/Extension/Emoji"

/**
 * IBUS_PATH_CONFIG:
 *
 * D-Bus path for IBus config.
 */
#define IBUS_PATH_CONFIG        "/org/freedesktop/IBus/Config"

/**
 * IBUS_PATH_NOTIFICATIONS:
 *
 * D-Bus path for IBus notifications.
 */
#define IBUS_PATH_NOTIFICATIONS "/org/freedesktop/IBus/Notifications"

/**
 * IBUS_PATH_INPUT_CONTEXT:
 *
 * Template of D-Bus path for IBus input context.
 */
#define IBUS_PATH_INPUT_CONTEXT "/org/freedesktop/IBus/InputContext_%d"

/**
 * IBUS_INTERFACE_IBUS:
 *
 * D-Bus interface for IBus.
 */
#define IBUS_INTERFACE_IBUS     "org.freedesktop.IBus"

/**
 * IBUS_INTERFACE_PORTAL:
 *
 * D-Bus interface for IBus portal.
 */
#define IBUS_INTERFACE_PORTAL   "org.freedesktop.IBus.Portal"

/**
 * IBUS_INTERFACE_INPUT_CONTEXT:
 *
 * D-Bus interface for IBus input context.
 */
#define IBUS_INTERFACE_INPUT_CONTEXT \
                                "org.freedesktop.IBus.InputContext"

/**
 * IBUS_INTERFACE_FACTORY:
 *
 * D-Bus interface for IBus factory.
 */
#define IBUS_INTERFACE_FACTORY  "org.freedesktop.IBus.Factory"

/**
 * IBUS_INTERFACE_ENGINE:
 *
 * D-Bus interface for IBus engine.
 */
#define IBUS_INTERFACE_ENGINE   "org.freedesktop.IBus.Engine"

/**
 * IBUS_INTERFACE_PANEL:
 *
 * D-Bus interface for IBus panel.
 */
#define IBUS_INTERFACE_PANEL    "org.freedesktop.IBus.Panel"

/**
 * IBUS_INTERFACE_CONFIG:
 *
 * D-Bus interface for IBus config.
 */
#define IBUS_INTERFACE_CONFIG   "org.freedesktop.IBus.Config"

/**
 * IBUS_INTERFACE_NOTIFICATIONS:
 *
 * D-Bus interface for IBus notifications.
 */
#define IBUS_INTERFACE_NOTIFICATIONS    "org.freedesktop.IBus.Notifications"

G_BEGIN_DECLS

/**
 * ibus_get_local_machine_id:
 *
 * Obtains the machine UUID of the machine this process is running on.
 *
 * Returns: A const string that shows the UUID of the machine.
 */
const gchar     *ibus_get_local_machine_id
                                        (void);

/**
 * ibus_set_display:
 * @display: Display address, as in DISPLAY environment for X.
 *
 * Set the display address.
 */
void             ibus_set_display       (const gchar    *display);

/**
 * ibus_get_address:
 *
 * Return the D-Bus address of IBus.
 * It will find the address from following source:
 * <orderedlist>
 *    <listitem><para>Environment variable IBUS_ADDRESS</para></listitem>
 *    <listitem><para>Socket file under ~/.config/ibus/bus/</para></listitem>
 * </orderedlist>
 *
 * Returns: D-Bus address of IBus. %NULL for not found.
 *
 * See also: ibus_write_address().
 */
const gchar     *ibus_get_address       (void);

/**
 * ibus_write_address:
 * @address: D-Bus address of IBus.
 *
 * Write D-Bus address to socket file.
 *
 * See also: ibus_get_address().
 */
void             ibus_write_address     (const gchar    *address);

/**
 * ibus_get_user_name:
 *
 * Get the current user name.
 * It is determined by:
 * <orderedlist>
 *    <listitem><para>getlogin()</para></listitem>
 *    <listitem><para>Environment variable SUDO_USER</para></listitem>
 *    <listitem><para>Environment variable USERHELPER_UID</para></listitem>
 *    <listitem><para>Environment variable USERNAME</para></listitem>
 *    <listitem><para>Environment variable LOGNAME</para></listitem>
 *    <listitem><para>Environment variable USER</para></listitem>
 *    <listitem><para>Environment variable LNAME</para></listitem>
 * </orderedlist>
 *
 * Returns: A const string that stores current user name.
 */
const gchar     *ibus_get_user_name     (void);

/**
 * ibus_get_group_name:
 *
 * Get the current user group name.
 *
 * Returns: A const string that stores current user group name.
 *
 * Since: 1.5.34
 * Stability: Unstable
 */
const gchar     *ibus_get_group_name    (void);

/**
 * ibus_get_daemon_uid:
 *
 * Get UID of ibus-daemon.
 *
 * Returns: UID of ibus-daemon; or 0 if UID is not available.
 *
 * Deprecated: This function has been deprecated and should
 * not be used in newly written code.
 */
glong            ibus_get_daemon_uid    (void) G_GNUC_DEPRECATED;

/**
 * ibus_get_socket_path:
 *
 * Get the path of socket file.
 *
 * Returns: A const string that stores the path of socket file.
 */
const gchar     *ibus_get_socket_path   (void);

/**
 * ibus_get_timeout:
 *
 * Get the GDBus timeout in milliseconds. The timeout is for clients (e.g.
 * im-ibus.so), not for ibus-daemon.
 * Note that the timeout for ibus-daemon could be set by --timeout command
 * line option of the daemon.
 *
 * Returns: A GDBus timeout in milliseconds. -1 when default timeout for
 *     GDBus should be used.
 */
gint             ibus_get_timeout       (void);

/**
 * ibus_free_strv:
 * @strv: List of strings.
 *
 * Free a list of strings.
 * Deprecated: This function has been deprecated and should
 * not be used in newly written code.
 */
void             ibus_free_strv         (gchar          **strv)
    G_GNUC_DEPRECATED;

/**
 * ibus_key_event_to_string:
 * @keyval: Key symbol.
 * @modifiers: Modifiers such as Ctrl or Shift.
 *
 * Return the name of a key symbol and modifiers.
 *
 * For example, if press ctrl, shift, and enter, then this function returns:
 * Shift+Control+enter.
 *
 * Returns: The name of a key symbol and modifier.
 */
gchar           *ibus_key_event_to_string
                                        (guint           keyval,
                                         guint           modifiers);

/**
 * ibus_key_event_from_string:
 * @string: Key event string.
 * @keyval: (out): Variable that hold key symbol result.
 * @modifiers: (out): Variable that hold modifiers result.
 *
 * Parse key event string and return key symbol and modifiers.
 *
 * Returns: %TRUE for succeed; %FALSE if failed.
 */
gboolean         ibus_key_event_from_string
                                        (const gchar    *string,
                                         guint          *keyval,
                                         guint          *modifiers);

/**
 * ibus_init:
 *
 * Initialize the ibus types.
 */
void             ibus_init              (void);

/**
 * ibus_main:
 *
 * Runs an IBus main loop until ibus_quit() is called in the loop.
 *
 * See also: ibus_quit().
 */
void             ibus_main              (void);

/**
 * ibus_quit:
 *
 * Stops an IBus from running.
 *
 * Any calls to ibus_quit() for the loop will return.
 * See also: ibus_main().
 */
void             ibus_quit              (void);

/**
 * ibus_set_log_handler:
 * @verbose: TRUE for verbose logging.
 *
 * Sets GLIB's log handler to ours. Our log handler adds time info
 * including hour, minute, second, and microsecond, like:
 *
 * (ibus-daemon:7088): IBUS-DEBUG: 18:06:45.822819: ibus-daemon started
 *
 * If @verbose is %TRUE, all levels of messages will be logged. Otherwise,
 * DEBUG and WARNING messages will be ignored.  The function is used in
 * ibus-daemon, but can be useful for IBus client programs as well for
 * debugging. It's totally fine for not calling this function. If you
 * don't set a custom GLIB log handler, the default GLIB log handler will
 * be used.
 */
void             ibus_set_log_handler   (gboolean verbose);

/**
 * ibus_unset_log_handler:
 *
 * Remove the log handler which is set by ibus_set_log_handler.
 */
void             ibus_unset_log_handler (void);

G_END_DECLS
#endif
