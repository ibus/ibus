#!/usr/bin/python3
# vim:set fileencoding=utf-8 et sts=4 sw=4:
#
# ibus - The Input Bus
#
# Copyright (c) 2026 Takao Fujiwara <takao.fujiwara1@gmail.com>
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


import json
import os
import shutil
import subprocess
import sys
from pathlib import Path

# Set the previous ibus version from `git tag -l` command.
IBUS_PREV_VERSION = os.getenv('IBUS_PREV_VERSION')
BUILD_ROOT = os.getenv('MESON_PROJECT_BUILD_ROOT', '.')
SOURCE_ROOT = os.getenv('MESON_PROJECT_SOURCE_ROOT', '.')
DIST_ROOT = os.getenv('MESON_PROJECT_DIST_ROOT', '.')

keymap_exception = [ 'common', 'meson.build', 'modifiers',
                     'us', 'jp', 'kr', 'in'
                   ]
ignore_file_pairs = [
    { 'name': 'common',
      'paths': [ '.github', '.gitignore', '.travis.yml',
                 'codereview.settings',
               ],
    },
    { 'name': 'ibus',
      'paths': [ 'conf/gconf',
                 'data/ibus.schemas.in', 'data/icons/ibus-help.png',
                 'data/icons/ibus-locale.svg', 'data/icons/ibus-zh.svg',
                 'data/keymaps', 'ibus', 'python-config.py', 'src/python',
                 'src/tests/ibus-engine.c',
                 'src/tests/ibus-global-engine.c',
                 'src/tests/ibus-keymap.c',
                 'src/tests/ibus-proxy.c',
                 'ui/gtk2',
               ],
    },
]


def run_git_subprocess_withreturncode(command):
    result = subprocess.run(command, shell = True)
    if result.returncode != 0:
        print(f'Failed to run git command {command}', file=sys.stderr)
        sys.exit(result.returncode)


cmd = ['meson', 'introspect', '--projectinfo', BUILD_ROOT]
info = None
try:
    result = subprocess.run(cmd, text=True, check=True,
                            stdout=subprocess.PIPE,
                            stderr=subprocess.STDOUT,
                            cwd=BUILD_ROOT).stdout
    info = json.loads(result)
except subprocess.CalledProcessError as e:
    print(f'Failed to run meson introspect: {e}', file=sys.stderr)
    sys.exit(1)
except json.JSONDecodeError as e:
    print(f'Failed to decode JSON from meson introspect: {e}',
          file=sys.stderr)
    sys.exit(1)

os.chdir(SOURCE_ROOT)
if not Path('.git').is_dir():
    print(f'{SOURCE_ROOT}/.git is not a directory', file=sys.stderr)
    sys.exit(0)

name_str = info.get('descriptive_name')
version_str = info.get('version')
if name_str == None or version_str == None:
    print(f'You should run meson setup {BUILD_ROOT}', file=sys.stderr)
    sys.exit(1)

versions = version_str.split('.')
major_version = int(versions[0])
minor_version = int(versions[1])
micro_version = int(versions[2].split('-')[0])
if IBUS_PREV_VERSION == None:
    prev_micro_version = micro_version - 1
    IBUS_PREV_VERSION = '%d.%d.%d' % (major_version,
                                      minor_version,
                                      prev_micro_version)

run_git_subprocess_withreturncode(
    'git log --name-status --date=iso > "{0}/{1}"'.format(
    DIST_ROOT, 'ChangeLog'))
run_git_subprocess_withreturncode(
    'echo "Changes in {0} {1}" > "{2}/{3}"'.format(
    name_str, version_str,
    DIST_ROOT, 'NEWS'))
run_git_subprocess_withreturncode(
    'echo "" >> "{0}/{1}"'.format(
    DIST_ROOT, 'NEWS'))
run_git_subprocess_withreturncode(
    'git shortlog {0}...{1} >> "{2}/{3}"'.format(
    IBUS_PREV_VERSION, version_str,
    DIST_ROOT, 'NEWS'))
run_git_subprocess_withreturncode(
    'echo "" >> "{0}/{1}"'.format(
    DIST_ROOT, 'NEWS'))
run_git_subprocess_withreturncode(
    'git log {0}...{1} {2} >> "{3}/{4}"'.format(
    IBUS_PREV_VERSION, version_str,
    '--reverse --pretty=format:"%s (%an) %h"',
    DIST_ROOT, 'NEWS'))

try:
    shutil.copy2('autogen.sh', f'{DIST_ROOT}')
    # Do you wish to copy configure and Makefile.in ?
except OSError as e:
    print(f'Failed to copy autogen.sh: {e}', file=sys.stderr)
    sys.exit(1)

os.chdir(DIST_ROOT)
# Should ignore deprecated files or Git files.
for ignore_file_pair in ignore_file_pairs:
    _name = ignore_file_pair['name']
    if _name != 'common' and  _name != name_str:
        continue
    for _path in ignore_file_pair['paths']:
        ignored_path = Path(_path)
        if ignored_path.exists():
            try:
                if _name == name_str and _path == 'data/keymaps':
                    if not ignored_path.is_dir():
                        print(f'{ignored_path} should be a directory',
                              file=sys.stderr)
                        sys.exit(1)
                    for child in ignored_path.iterdir():
                        if child.name not in keymap_exception and \
                           not child.is_dir():
                            child.unlink()
                    print(f'Delete deprecated files in {ignored_path}')
                else:
                    if ignored_path.is_dir():
                        shutil.rmtree(ignored_path)
                    else:
                        ignored_path.unlink()
                    print(f'Delete deprecated {ignored_path}')
            except Exception as e:
                print(f'Failed to rm {ignored_path}: {e}', file=sys.stderr)
                sys.exit(1)
        elif _name == name_str:
            print(f'{ignored_path} does not exist')
