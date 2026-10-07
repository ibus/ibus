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
        "PanelBase",
        "PanelItem",
        "PanelButton",
        "PanelToggleButton",
        "PanelMenu",
        "IBUS_SERVICE_PANEL",
        "IBUS_PATH_PANEL"
    )

IBUS_SERVICE_PANEL = "org.freedesktop.IBus.Panel"
IBUS_PATH_PANEL = "/org/freedesktop/IBus/Panel"

from serializable import *
from object import Object
import interface
import dbus

class PanelItem:
    pass

class PanelButton(PanelItem):
    pass

class PanelToggleButton(PanelButton):
    pass

class PanelMenu(PanelItem):
    pass

class PanelBase(Object):
    def __init__(self, bus):
        super(PanelBase, self).__init__()
        self.__bus = bus
        self.__proxy = PanelProxy(self, bus)

    def set_cursor_location(self, x, y, w, h):
        pass

    def update_preedit_text(self, text, cursor_pos, visible):
        pass

    def show_preedit_text(self):
        pass

    def hide_preedit_text(self):
        pass

    def update_auxiliary_text(self, text, visible):
        pass

    def show_auxiliary_text(self):
        pass

    def hide_auxiliary_text(self):
        pass

    def update_lookup_table(self, lookup_table, visible):
        pass

    def show_lookup_table(self):
        pass

    def hide_lookup_table(self):
        pass

    def show_candidate_window(self):
        pass

    def page_up_lookup_table(self):
        pass

    def page_down_lookup_table(self):
        pass

    def cursor_up_lookup_table(self):
        pass

    def cursor_down_lookup_table(self):
        pass

    def hide_candidate_window(self):
        pass

    def show_language_bar(self):
        pass

    def hide_language_bar(self):
        pass

    def register_properties(self, props):
        pass

    def update_property(self, prop):
        pass

    def focus_in(self, ic):
        pass

    def focus_out(self, ic):
        pass

    def state_changed(self):
        pass

    def reset(self):
        pass

    def start_setup(self):
        pass

    def page_up(self):
        self.__proxy.PageUp()

    def page_down(self):
        self.__proxy.PageDown()

    def cursor_up(self):
        self.__proxy.CursorUp()

    def cursor_down(self):
        self.__proxy.CursorDown()

    def candidate_clicked(self, index, button, state):
        self.__proxy.CandidateClicked(index, button, state)

    def property_activate(self, prop_name, prop_state):
        prop_name = dbus.String(prop_name)
        prop_state = dbus.Int32(prop_state)
        self.__proxy.PropertyActivate(prop_name, prop_state)

    def property_show(self, prop_name):
        prop_name = dbus.String(prop_name)
        self.__proxy.PropertyShow(prop_name)

    def property_hide(self, prop_name):
        prop_name = dbus.String(prop_name)
        self.__proxy.PropertyHide(prop_name)


class PanelProxy(interface.IPanel):
    def __init__ (self, panel, bus):
        super(PanelProxy, self).__init__(bus.get_dbusconn(), IBUS_PATH_PANEL)
        self.__bus = bus
        self.__panel = panel
        self.__focus_ic = None

    def SetCursorLocation(self, x, y, w, h):
        self.__panel.set_cursor_location(x, y, w, h)

    def UpdatePreeditText(self, text, cursor_pos, visible):
        text = deserialize_object(text)
        self.__panel.update_preedit_text(text, cursor_pos, visible)

    def ShowPreeditText(self):
        self.__panel.show_preedit_text()

    def HidePreeditText(self):
        self.__panel.hide_preedit_text()

    def UpdateAuxiliaryText(self, text, visible):
        text = deserialize_object(text)
        self.__panel.update_auxiliary_text(text, visible)

    def ShowAuxiliaryText(self):
        self.__panel.show_auxiliary_text()

    def HideAuxiliaryText(self):
        self.__panel.hide_auxiliary_text()

    def UpdateLookupTable(self, lookup_table, visible):
        lookup_table = deserialize_object(lookup_table)
        self.__panel.update_lookup_table(lookup_table, visible)

    def ShowLookupTable(self):
        self.__panel.show_lookup_table()

    def HideLookupTable(self):
        self.__panel.hide_lookup_table()

    def PageUpLookupTable(self):
        self.__panel.page_up_lookup_table()

    def PageDownLookupTable(self):
        self.__panel.page_down_lookup_table()

    def CursorUpLookupTable(self):
        self.__panel.cursor_up_lookup_table()

    def CursorDownLookupTable(self):
        self.__panel.cursor_down_lookup_table()

    def ShowCandidateWindow(self):
        self.__panel.show_candidate_window()

    def HideCandidateWindow(self):
        self.__panel.hide_candidate_window()

    def ShowLanguageBar(self):
        self.__panel.show_language_bar()

    def HideLanguageBar(self):
        self.__panel.hide_language_bar()

    def RegisterProperties(self, props):
        props = deserialize_object(props)
        self.__panel.register_properties(props)

    def UpdateProperty(self, prop):
        prop = deserialize_object(prop)
        self.__panel.update_property(prop)

    def FocusIn(self, ic):
        self.__panel.focus_in(ic)

    def FocusOut(self, ic):
        self.__panel.focus_out(ic)

    def StateChanged(self):
        self.__panel.state_changed()

    def Reset(self):
        self.__panel.reset()

    def StartSetup(self):
        self.__panel.start_setup()

    def Destroy(self):
        self.__panel.destroy()

def test():
    import gtk
    from bus import Bus
    from inputcontext import InputContext
    import factory
    import attribute
    import property
    import text
    import lookuptable

    class TestPanel(PanelBase):
        def __init__(self):
            self.__bus = Bus()
            self.__bus.connect("disconnected", gtk.main_quit)
            super(TestPanel, self).__init__(self.__bus)
            self.__bus.request_name(IBUS_SERVICE_PANEL, 0)

        def focus_in(self, ic):
            print "focus-in:", ic
            context = InputContext(self.__bus, ic)
            info = context.get_factory_info()
            print "factory:", info.name

        def focus_out(self, ic):
            print "focus-out:", ic

        def update_auxiliary_text(self, text, visible):
            print "update-auxiliary-text:", text.text

        def update_lookup_table(self, table, visible):
            print "update-lookup-table", table

    panel = TestPanel()
    gtk.main()


if __name__ == "__main__":
    test()
