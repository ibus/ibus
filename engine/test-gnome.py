#!/usr/bin/python
# vim:set fileencoding=utf-8 et sts=4 sw=4:
#
# ibus - Intelligent Input Bus for Linux / Unix OS
#
# Copyright © 2025 Takao Fujiwara <takao.fujiwara1@gmail.com>
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
# License along with this library. If not, see <http://www.gnu.org/licenses/>.

# Check gnome-shell/js/misc/keyboardManager.js

from gi import require_version as gi_require_version
gi_require_version('GnomeDesktop', '4.0')

from gi.repository import GnomeDesktop

import os, re, sys


class TestGnomeDesktop:
    __srcdir = '.'
    __option_debug = False
    __simple_filename = 'simple.xml.in'
    __denylist_filename = 'denylist.txt'
    __xkb_info = None
    __simple_contents = None
    __denylist_lines = None

    @classmethod
    def parse_args(cls):
        cls.__srcdir = os.path.dirname(sys.argv[0])
        if cls.__srcdir == '':
            cls.__srcdir = '.'
        if len(sys.argv) > 1:
            arg1 = sys.argv[1]
            if arg1 == '--debug':
                cls.__option_debug = True

    def __init__(self):
        self.__simple_filename = '%s/%s' % (self.__srcdir,
                                            self.__simple_filename)
        self.__denylist_filename = '%s/%s' % (self.__srcdir,
                                              self.__denylist_filename)
        self.__xkb_info = GnomeDesktop.XkbInfo()

    def read_simple(self):
        simple_file = open(self.__simple_filename)
        self.__simple_contents = simple_file.read()
        simple_file.close()

    def read_denylist(self):
        denylist_file = open(self.__denylist_filename)
        self.__denylist_lines = denylist_file.readlines()
        denylist_file.close()
        l = []
        for line in self.__denylist_lines:
            line = line.rstrip()
            if len(line) == 0:
                continue
            if not line.startswith('#'):
                l.append(line)
        self.__denylist_lines = l

    def __check_known_issue_engine_name(self, desc):
        reason = None
        if desc == 'xkb:in:eng:eng':
            wrong_desc = 'xkb:in:eng:en'
            simple_res = re.search(wrong_desc, self.__simple_contents)
            if simple_res != None:
                reason = '# Info: xkeyboard-config 2.42 has incorrect %s ' \
                         "but it's fixed in 2.44" % wrong_desc
        return reason

    def test_simple_with_gnome(self):
        errnum = 0
        for layout_variant in self.__xkb_info.get_all_layouts():
            languages = self.__xkb_info.get_languages_for_layout(layout_variant)
            layouts = layout_variant.split('+')
            if len(languages) == 0:
                if self.__option_debug:
                    print('# Info: No language: %s' % layout_variant)
                continue
            if len(layouts) >= 2:
                desc = 'xkb:%s:%s:%s' % (layouts[0], layouts[1], languages[0])
            else:
                desc = 'xkb:%s::%s' % (layouts[0], languages[0])
            if self.__option_debug:
                print('# Debug: Test', layout_variant, languages, desc)
            simple_res = re.search(desc, self.__simple_contents)
            if simple_res == None:
                denylist_res = None
                for line in self.__denylist_lines:
                    denylist_res = re.match(line, desc)
                    if denylist_res != None:
                        break
                if denylist_res == None:
                    reason = self.__check_known_issue_engine_name(desc)
                    if reason != None:
                        if self.__option_debug:
                            print(reason)
                    else:
                        errnum = errnum + 1
                        print('# Error: not found: %s' % desc, file=sys.stderr)
                elif self.__option_debug:
                    print('# Info: %s in denylist' % desc)
        return errnum


def main():
    TestGnomeDesktop.parse_args()
    obj = TestGnomeDesktop()
    obj.read_simple()
    obj.read_denylist()
    errnum = obj.test_simple_with_gnome()
    if errnum == 0:
        print('Succeeded.')
    else:
        print('Failed. %s errors found.' % errnum, file=sys.stderr)
        sys.exit(1)

main()
