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

__all__ = ("IPanel", )

import dbus.service
from ibus.common import \
    IBUS_IFACE_PANEL

class IPanel(dbus.service.Object):
    # define method decorator.
    method = lambda **args: \
        dbus.service.method(dbus_interface=IBUS_IFACE_PANEL, \
                            **args)

    # define signal decorator.
    signal = lambda **args: \
        dbus.service.signal(dbus_interface=IBUS_IFACE_PANEL, \
                            **args)

    # define async method decorator.
    async_method = lambda **args: \
        dbus.service.method(dbus_interface=IBUS_IFACE_PANEL, \
                            async_callbacks=("reply_cb", "error_cb"), \
                            **args)
    @method(in_signature="iiii")
    def SetCursorLocation(self, x, y, w, h): pass

    @method(in_signature="vub")
    def UpdatePreeditText(self, text, cursor_pos, visible): pass

    @method()
    def ShowPreeditText(self): pass

    @method()
    def HidePreeditText(self): pass

    @method(in_signature="vb")
    def UpdateAuxiliaryText(self, text, visible): pass

    @method()
    def ShowAuxiliaryText(self): pass

    @method()
    def HideAuxiliaryText(self): pass

    @method(in_signature="vb")
    def UpdateLookupTable(self, lookup_table, visible): pass

    @method()
    def ShowLookupTable(self): pass

    @method()
    def HideLookupTable(self): pass

    @method()
    def PageUpLookupTable(self): pass

    @method()
    def PageDownLookupTable(self): pass

    @method()
    def CursorUpLookupTable(self): pass

    @method()
    def CursorDownLookupTable(self): pass

    @method(in_signature="v")
    def RegisterProperties(self, props): pass

    @method(in_signature="v")
    def UpdateProperty(self, prop): pass

    @method()
    def ShowLanguageBar(self): pass

    @method()
    def HideLanguageBar(self): pass

    @method(in_signature="o")
    def FocusIn(self, ic): pass

    @method(in_signature="o")
    def FocusOut(self, ic): pass

    @method()
    def StateChanged(self): pass

    @method()
    def Reset(self): pass

    @method()
    def StartSetup(self): pass

    @method()
    def Destroy(self): pass

    #signals
    @signal()
    def PageUp(self): pass

    @signal()
    def PageDown(self): pass

    @signal()
    def CursorUp(self): pass

    @signal(signature="uuu")
    def CandidateClicked(self, index, button, state): pass

    @signal()
    def CursorDown(self): pass

    @signal(signature="su")
    def PropertyActivate(self, prop_name, prop_state): pass

    @signal(signature="s")
    def PropertyShow(self, prop_name): pass

    @signal(signature="s")
    def PropertyHide(self, prop_name): pass

