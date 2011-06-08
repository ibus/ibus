# vim:set et sts=4 sw=4:
#
# ibus - The Input Bus
#
# Copyright(c) 2007-2010 Peng Huang <shawn.p.huang@gmail.com>
# Copyright(c) 2007-2010 Red Hat, Inc.
#
# This library is free software; you can redistribute it and/or
# modify it under the terms of the GNU Lesser General Public
# License as published by the Free Software Foundation; either
# version 2 of the License, or(at your option) any later version.
#
# This library is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU Lesser General Public License for more details.
#
# You should have received a copy of the GNU Lesser General Public
# License along with this program; if not, write to the
# Free Software Foundation, Inc., 59 Temple Place, Suite 330,
# Boston, MA  02111-1307  USA

__all__ = ("IEngine", )

import dbus.service
from ibus.common import \
    IBUS_IFACE_ENGINE

class IEngine(dbus.service.Object):
    # define method decorator.
    method = lambda **args: \
        dbus.service.method(dbus_interface=IBUS_IFACE_ENGINE, \
                            **args)

    # define signal decorator.
    signal = lambda **args: \
        dbus.service.signal(dbus_interface=IBUS_IFACE_ENGINE, \
                            **args)

    # define async method decorator.
    async_method = lambda **args: \
        dbus.service.method(dbus_interface=IBUS_IFACE_ENGINE, \
                            async_callbacks=("reply_cb", "error_cb"), \
                            **args)

    @method(in_signature="uuu", out_signature="b")
    def ProcessKeyEvent(self, keyval, keycode, state):
        pass

    @method(in_signature="iiii")
    def SetCursorLocation(self, x, y, w, h): pass

    @method(in_signature="vu")
    def SetSurroundingText(self, text, cursor_index): pass

    @method(in_signature="u")
    def SetCapabilities(self, cap): pass

    @method()
    def FocusIn(self): pass

    @method()
    def FocusOut(self): pass

    @method()
    def Reset(self): pass

    # signals for lookup table
    @method()
    def PageUp(self): pass

    @method()
    def PageDown(self): pass

    @method()
    def CursorUp(self): pass

    @method()
    def CursorDown(self): pass

    @method(in_signature="uuu")
    def CandidateClicked(self, index, button, state):
        pass

    @method()
    def Enable(self): pass

    @method()
    def Disable(self): pass

    @method(in_signature="su")
    def PropertyActivate(self, prop_name, prop_state): pass

    @method(in_signature="s")
    def PropertyShow(self, prop_name): pass

    @method(in_signature="s")
    def PropertyHide(self, prop_name): pass

    @method()
    def Destroy(self): pass


    @signal(signature="v")
    def CommitText(self, text): pass

    @signal(signature="uuu")
    def ForwardKeyEvent(self, keyval, keycode, state): pass

    @signal(signature="vubu")
    def UpdatePreeditText(self, text, cursor_pos, visible, mode): pass

    @signal()
    def ShowPreeditText(self): pass

    @signal()
    def HidePreeditText(self): pass

    @signal(signature="vb")
    def UpdateAuxiliaryText(self, text, visible): pass

    @signal()
    def ShowAuxiliaryText(self): pass

    @signal()
    def HideAuxiliaryText(self): pass

    @signal(signature="vb")
    def UpdateLookupTable(self, lookup_table, visible): pass

    @signal()
    def ShowLookupTable(self): pass

    @signal()
    def HideLookupTable(self): pass

    @signal()
    def PageUpLookupTable(self): pass

    @signal()
    def PageDownLookupTable(self): pass

    @signal()
    def CursorUpLookupTable(self): pass

    @signal()
    def CursorDownLookupTable(self): pass

    @signal(signature="v")
    def RegisterProperties(self, props): pass

    @signal(signature="v")
    def UpdateProperty(self, prop): pass

    @signal(signature="iu")
    def DeleteSurroundingText(self, offset_from_cursor, nchars): pass

    @signal()
    def RequireSurroundingText(self): pass
