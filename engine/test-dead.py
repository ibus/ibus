#!/usr/bin/python3
# vim:set fileencoding=utf-8 et sts=4 sw=4:
#
# ibus - Intelligent Input Bus for Linux / Unix OS
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
# License along with this library. If not, see <http://www.gnu.org/licenses/>.

# Check src/ibusinternal.h:IS_DEAD_KEY()

import os, sys
from pathlib import Path

class TestDeadKey:
    __srcdir = None
    __prgname = None
    __is_dead_key_file = Path('ibusinternal.h')
    __dead_keys_file  = Path('ibuskeysyms.h')
    __max_dead_line = None
    __max_dead_key  = None
    __max_dead_val = 0
    __min_dead_line = None
    __min_dead_key  = None
    __min_dead_val = 0xffff

    @classmethod
    def parse_args(cls):
        arg0 = Path(sys.argv[0])
        cls.__srcdir = arg0.parent.parent
        cls.__prgname = arg0.name
        lib_srcdir = os.getenv('lib_srcdir')
        if len(sys.argv) > 1:
            cls.__srcdir = Path(sys.argv[1])
        elif lib_srcdir != None:
            cls.__srcdir = Path(lib_srcdir)
        if str(cls.__srcdir) == '':
            cls.__srcdir = Path('.')

    def tests(self):
        print('TAP version 14')
        print('1..1')
        dead_keys_path = self.__srcdir / self.__dead_keys_file
        if not dead_keys_path.exists():
            print(f'fail Not found {str(dead_keys_path)}', file=sys.stderr)
            sys.exit(1)
        is_dead_key_path = self.__srcdir / self.__is_dead_key_file
        if not is_dead_key_path.exists():
            print(f'fail Not found {str(is_dead_key_path)}', file=sys.stderr)
            sys.exit(1)
        self.read_dead_keys_file(dead_keys_path)
        if self.read_is_dead_key_file(is_dead_key_path):
            print(f'ok 1 /{self.__prgname}')
        else:
            sys.exit(1)

    def read_dead_keys_file(self, dead_keys_path):
        with dead_keys_path.open() as f:
            for line in f:
                elements = line.split()
                if len(elements) < 3:
                    continue
                if elements[0] != '#define':
                    continue
                if not elements[1].startswith('IBUS_KEY_dead'):
                    continue
                val = int(elements[2], 16)
                if self.__max_dead_val < val:
                    self.__max_dead_val = val
                    self.__max_dead_line = line
                    self.__max_dead_key = elements[1]
                if self.__min_dead_val > val:
                    self.__min_dead_val = val
                    self.__min_dead_line = line
                    self.__min_dead_key = elements[1]
        if self.__min_dead_val >= self.__max_dead_val:
            print(f'fail {self.__min_dead_line} < {self.__max_dead_line}',
                  file=sys.stderr)
            sys.exit(1)

    def read_is_dead_key_file(self, is_dead_key_path):
        has_definition = False
        min_key = ''
        max_key = ''
        with is_dead_key_path.open() as f:
            for line in f:
                elements = line.split()
                if not has_definition:
                    if len(elements) < 2:
                        continue
                    if elements[0] != '#define':
                        continue
                    if elements[1].startswith('IS_DEAD_KEY('):
                        has_definition = True
                # ((k) >= IBUS_KEY_dead_min && (k) <= IBUS_KEY_dead_max)
                else:
                    if len(elements) < 7:
                        continue
                    min_key = elements[2]
                    max_key = elements[6]
                    if max_key[len(max_key) - 1] == ')':
                        max_key = max_key[:len(max_key) - 1]
                    break
        is_failed = False
        print(f'# min: {min_key} < max: {max_key}')
        if self.__min_dead_key != min_key:
            print(f'fail min of IS_DEAD_KEY() is %s but min in %s is %s' % \
                  (min_key, str(self.__dead_keys_file), self.__min_dead_key),
                  file=sys.stderr)
            is_failed = True
        if self.__max_dead_key != max_key:
            print(f'fail max of IS_DEAD_KEY() is %s but max in %s is %s' % \
                  (max_key, str(self.__dead_keys_file), self.__max_dead_key),
                  file=sys.stderr)
            is_failed = True
        return not is_failed


def main():
    TestDeadKey.parse_args()
    obj = TestDeadKey()
    obj.tests()
    sys.exit(0)

main()
