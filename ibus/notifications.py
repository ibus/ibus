# vim:set et sts=4 sw=4:
#
# ibus - The Input Bus
#
# Copyright (c) 2007-2010 Peng Huang <shawn.p.huang@gmail.com>
# Copyright (c) 2007-2010 Red Hat, Inc.
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

__all__ = (
        "NotificationsBase",
        "IBUS_SERVICE_NOTIFICATIONS",
        "IBUS_PATH_NOTIFICATIONS"
    )

IBUS_SERVICE_NOTIFICATIONS = "org.freedesktop.IBus.Notifications"
IBUS_PATH_NOTIFICATIONS = "/org/freedesktop/IBus/Notifications"

import ibus
from ibus import interface

class NotificationsBase(ibus.Object):
    def __init__(self, bus):
        super(NotificationsBase, self).__init__()
        self.__proxy = NotificationsProxy(self, bus.get_dbusconn())

    def notify(self, replaces_id, app_icon, summary, body, actions, expire_timeout):
        pass

    def close_notification(self, id):
        pass

    def notification_closed(self, id, reason):
        self.__proxy.NotificationClosed(id, reason)

    def action_invoked(self, id, action_key):
        self.__proxy.ActionInvoked(id, action_key)

class NotificationsProxy(interface.INotifications):
    def __init__ (self, notify, dbusconn):
        super(NotificationsProxy, self).__init__(dbusconn, IBUS_PATH_NOTIFICATIONS)
        self.__dbusconn = dbusconn
        self.__notify = notify
    
    def Notify(self, replaces_id, app_icon, summary, body, actions, expire_timeout):
        return self.__notify.notify(replaces_id, app_icon, summary, body, actions, expire_timeout)
    
    def CloseNotification(self, id):
        return self.__notify.close_notification(id)
