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

__all__ = ("IIBus", )

import dbus.service
from ibus.common import \
    IBUS_IFACE_IBUS, \
    IBUS_IFACE_CONFIG

class IIBus(dbus.service.Object):
    # define method decorator.
    method = lambda **args: \
        dbus.service.method(dbus_interface=IBUS_IFACE_IBUS, \
                            connection_keyword="dbusconn", \
                            **args)

    # define async method decorator.
    async_method = lambda **args: \
        dbus.service.method(dbus_interface=IBUS_IFACE_IBUS, \
                            connection_keyword="dbusconn", \
                            async_callbacks=("reply_cb", "error_cb"), \
                            **args)

    # define signal decorator.
    signal = lambda **args: \
        dbus.service.signal(dbus_interface=IBUS_IFACE_IBUS, \
            **args)

    @method(out_signature="s")
    def GetAddress(self, dbusconn): pass

    # methods for ibus clients
    @method(in_signature="s", out_signature="o")
    def CreateInputContext(self, client_name, dbusconn): pass

    @method(out_signature="o")
    def CurrentInputContext(self, dbusconn): pass

    # general methods
    @method(in_signature="av")
    def RegisterComponent(self, components, dbusconn): pass

    @method(out_signature="av")
    def ListEngines(self, dbusconn): pass
    
    @method(in_signature="as", out_signature="av")
    def GetEnginesByNames(self, names, dbusconn): pass

    @method(out_signature="av")
    def ListActiveEngines(self, dbusconn): pass

    @method(in_signature="s")
    def SetGlobalEngine(self, name, dbusconn):pass

    @method(in_signature="b")
    def Exit(self, restart, dbusconn): pass

    @method(in_signature="v", out_signature="v")
    def Ping(self, data, dbusconn): pass

    @signal(signature="")
    def RegistryChanged(self): pass

