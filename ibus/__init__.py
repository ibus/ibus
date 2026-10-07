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

import warnings
warnings.warn("The ibus module is deprecated; "
    "Please use gobject-introspection instead", DeprecationWarning)

from object import *
from attribute import *
from property import *
from common import *
from interface import *
from exception import *
from lookuptable import *
from bus import *
from inputcontext import *
from lang import *
from utility import *
from engine import *
from factory import *
from panel import *
from notifications import *
from config import *
from serializable import *
from text import *
from observedpath import *
from enginedesc import *
from component import *
from _config import *
