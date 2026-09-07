#!/usr/bin/python
# ibus - The Input Bus
#
# Copyright © 2026 Takao Fujiwara <takao.fujiwara1@gmail.com>
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
# License along with this library; if not, write to the Free Software
# Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301
# USA

import sys
import unittest
from pathlib import Path

# move the script path at the end, so the necessary modules in system
# pygobject can be loaded first
tests_builddir = Path(__file__).resolve().parent
sys.path = [path for path in sys.path if Path(path) != tests_builddir]
sys.path.append(str(tests_builddir))

from gi import require_versions as gi_require_versions

gi_require_versions({'IBus': '1.0', 'GLib':  '2.0', 'Gio': '2.0', })
from gi.repository import IBus
from gi.repository import GLib
from gi.repository import Gio


class TestOverride(unittest.TestCase):
    __bus = None
    __context = None
    __text = None
    __panel = None
    __attrs = None
    __invocation = None

    def setUp(self):
        IBus.init()
        self.__bus = IBus.Bus()

    def update_preedit_text_cb(self,
                               context,
                               text,
                               cursor,
                               visible):
        self.update_preedit_text_with_mode_cb(
                context, text, cursor, visible, 0)

    def update_preedit_text_with_mode_cb(self,
                                         context,
                                         text,
                                         cursor,
                                         visible,
                                         mode):
        self.__text = text
        self.__attrs = self.__text.get_attributes()
        print('# Got update-preedit-text-with-mode',
              self.__text.get_text(), cursor, visible, mode)


    def __test_deserialized_input_context_real(self, case):
        text = IBus.Text.new_from_string('test')
        if case == 1:
            text.append_attribute(IBus.AttrType.HINT,
                                  IBus.AttrPreedit.WHOLE,
                                  0,
                                  text.get_length())
        text_variant = text.serialize_object()
        update_preedit_variant = GLib.Variant.new_tuple(
            GLib.Variant.new_variant(text_variant),
            GLib.Variant.new_uint32(0),
            GLib.Variant.new_boolean(True),
            GLib.Variant.new_uint32(0))
        self.__context.do_g_signal(self.__context,
                                   'org.freedesktop.DBus',
                                   'UpdatePreeditTextWithMode',
                                   update_preedit_variant)
        print('# Last', self.__attrs, self.__text.get_text())
        self.__attrs = None
        self.__text = None

    def __test_deserialized_panel_real(self, case):
        text = IBus.Text.new_from_string('test')
        if case == 1:
            text.append_attribute(IBus.AttrType.HINT,
                                  IBus.AttrPreedit.WHOLE,
                                  0,
                                  text.get_length())
        text_variant = text.serialize_object()
        update_preedit_variant = GLib.Variant.new_tuple(
            GLib.Variant.new_variant(text_variant),
            GLib.Variant.new_uint32(0),
            GLib.Variant.new_boolean(True))
        self.__panel.do_service_method_call(
                self.__panel,
                self.__bus.get_connection(),
                'org.freedesktop.IBus.NullInvocation',
                IBus.PATH_PANEL + '/DryTest',
                IBus.INTERFACE_PANEL,
                'UpdatePreeditText',
                update_preedit_variant.ref(),
                self.__invocation)
        print('# Last', self.__attrs, self.__text.get_text())
        self.__attrs = None
        self.__text = None

    def test_deserialized_input_context(self):
        self.__context = self.__bus.create_input_context('test')
        self.__context.connect('update-preedit-text-with-mode',
                               self.update_preedit_text_with_mode_cb)
        self.__test_deserialized_input_context_real(0)
        self.__test_deserialized_input_context_real(1)

    def test_deserialized_panel(self):
        self.__panel = IBus.PanelService.new(self.__bus.get_connection())
        self.__panel.connect('update-preedit-text',
                             self.update_preedit_text_cb)
        self.__invocation = Gio.DBusMethodInvocation()
        self.__test_deserialized_panel_real(0)
        self.__test_deserialized_panel_real(1)


if __name__ == '__main__':
    GLib.log_set_fatal_mask('GLib', GLib.LogLevelFlags.LEVEL_WARNING)
    unittest.main()
