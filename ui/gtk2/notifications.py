# vim:set et sts=4 sw=4:
#
# ibus - The Input Bus
#
# Copyright(c) 2007-2010 Peng Huang <shawn.p.huang@gmail.com>
# Copyright(c) 2007-2010 Red Hat, Inc.
#
# SPDX-License-Identifier: LGPL-2.1-or-later
#
# This library is free software; you can redistribute it and/or
# modify it under the terms of the GNU Lesser General Public
# License as published by the Free Software Foundation; either
# version 2.1 of the License, or (at your option) any later version.
#
# This library is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
# Lesser General Public License for more details.
#
# You should have received a copy of the GNU Lesser General Public
# License along with this library; if not, see <https://www.gnu.org/licenses/>.

import gtk
import gtk.gdk as gdk
import gobject
import ibus
import dbus
import icon as _icon
import os
import sys
from os import path
from ibus import interface

_  = ibus._
N_ = lambda a : a

class Notifications(ibus.NotificationsBase):
    def __init__(self, bus):
        super(Notifications, self).__init__(bus)
        self.__bus = bus
        try:
            self.__dbus = dbus.SessionBus()
            self.__dbus.watch_name_owner(ibus.IBUS_SERVICE_NOTIFICATIONS,
                    self.__notifications_name_owner_changed_cb)
        except:
            self.__dbus = None
        self.__notifications = None
        self.__ids = set([])
        self.__init_notifications()
        self.__status_icons = None
        self.__bus.request_name(ibus.IBUS_SERVICE_NOTIFICATIONS, 0)

    def __notifications_name_owner_changed_cb(self, unique_name):
        if unique_name:
            self.__init_notifications()
        else:
            self.__notifications = None

    def __init_notifications(self):
        if self.__dbus == None:
            return

        try:
            self.__notifications = self.__dbus.get_object(
                    ibus.IBUS_SERVICE_NOTIFICATIONS, ibus.IBUS_PATH_NOTIFICATIONS)
            self.__notifications.connect_to_signal("NotificationClosed",
                    self.__notification_closed_cb,
                    dbus_interface=ibus.IBUS_IFACE_NOTIFICATIONS)
            self.__notifications.connect_to_signal("ActionInvoked",
                    self.__action_invoked_cb,
                    dbus_interface=ibus.IBUS_IFACE_NOTIFICATIONS)
            self.__ids = set([])
        except:
            self.__notifications = None

    def set_status_icon(self, status_icon):
        self.__status_icon = status_icon

    def notify(self, replaces_id, app_icon, summary, body, actions, expire_timeout):
        if self.__notifications == None:
            return 0
        if app_icon == "":
            app_icon = "ibus"
        hints = dbus.Dictionary(signature="sv")
        if self.__status_icon != None:
            rect = self.__status_icon.get_geometry()[1]
            hints["x"] = rect.x + rect.width / 2
            hints["y"] = rect.y + rect.height / 2
        id = self.__notifications.Notify("ibus",
                        dbus.UInt32(replaces_id),
                        app_icon,
                        summary,
                        body,
                        actions,
                        hints,
                        expire_timeout)
        self.__ids.add(id)
        return id

    def close_notification(self, id):
        if self.__notifications == None:
            return
        return self.__notifications.CloseNotifications(id)

    def __notification_closed_cb(self, id, reason):
        if self.__notifications == None:
            return
        if id in self.__ids:
            self.notification_closed(id, reason)

    def __action_invoked_cb(self, id, action_key):
        if self.__notifications == None:
            return
        if id in self.__ids:
            self.action_invoked(id, action_key)

if __name__ == "__main__":
    import gtk
    icon = gtk.StatusIcon()
    icon.set_visible(True)
    notify = Notifications(ibus.Bus())
    notify.set_status_icon(icon)
    while ibus.main_iteration(): pass
    notify.notify(0, "", "Hello Summary", "Hello Body", ["NoAgain", "Do not show me again"], 5000)
    ibus.main()
