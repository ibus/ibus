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

__all__ = ("IEngineFactory", )

import dbus.service
from ibus.common import \
    IBUS_IFACE_ENGINE_FACTORY

class IEngineFactory(dbus.service.Object):
    # define method decorator.
    method = lambda **args: \
        dbus.service.method(dbus_interface=IBUS_IFACE_ENGINE_FACTORY, \
                            **args)

    # define async method decorator.
    async_method = lambda **args: \
        dbus.service.method(dbus_interface=IBUS_IFACE_ENGINE_FACTORY, \
                            async_callbacks=("reply_cb", "error_cb"), \
                            **args)

    # Return a array. [name, default_language, icon_path, authors, credits]
    @method(out_signature="as")
    def GetInfo(self): pass

    # Factory should allocate all resources in this method
    @method()
    def Initialize(self): pass

    # Factory should free all allocated resources in this method
    @method()
    def Uninitialize(self): pass

    # Create an input context and return the id of the context.
    # If failed, it will return "" or None.
    @method(in_signature="s", out_signature="o")
    def CreateEngine(self, engine_name): pass

    # Destroy the engine
    @method()
    def Destroy(self): pass
