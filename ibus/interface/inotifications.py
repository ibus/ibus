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

__all__ = ("INotifications", )

import dbus.service
from ibus.common import \
    IBUS_IFACE_NOTIFICATIONS

class INotifications(dbus.service.Object):
    # define method decorator.
    method = lambda **args: \
        dbus.service.method(dbus_interface = IBUS_IFACE_NOTIFICATIONS, \
                            **args)

    # define signal decorator.
    signal = lambda **args: \
        dbus.service.signal(dbus_interface = IBUS_IFACE_NOTIFICATIONS, \
                            **args)

    # define async method decorator.
    async_method = lambda **args: \
        dbus.service.method(dbus_interface = IBUS_IFACE_NOTIFICATIONS, \
                            async_callbacks = ("reply_cb", "error_cb"), \
                            **args)

    @method(in_signature="usssasi", out_signature="u")
    def Notify(self, replaces_id, app_icon, summary, body, actions, expire_timeout): pass
    
    @method(in_signature="u")
    def CloseNotification(self, id): pass

    #signals
    @signal(signature="uu")
    def NotificationClosed(self, id, reason): pass
    
    @signal(signature="us")
    def ActionInvoked(self, id, action_key): pass
