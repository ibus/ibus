#!/usr/bin/python
# vim:set fileencoding=utf-8 et sts=4 sw=4:
#
# ibus - Intelligent Input Bus for Linux / Unix OS
#
# Copyright © 2020 Takao Fujiwara <takao.fujiwara1@gmail.com>
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


# This script generates simple.xml with /usr/share/X11/xkb/rules/evdev.xml,
# /usr/share/xml/iso-codes/iso_639.xml and denylist.txt


from xml.dom import minidom
from xml.sax import make_parser as sax_make_parser
from xml.sax.handler import feature_namespaces as sax_feature_namespaces
from xml.sax.saxutils import XMLFilterBase, XMLGenerator, escape
from xml.sax.xmlreader import AttributesImpl
from xml.sax._exceptions import SAXParseException

import codecs
import getopt
import io
import os
import sys

VERSION='0.2'
EVDEV_XML = '/usr/share/X11/xkb/rules/evdev.xml'
EXEC_PATH='/usr/lib/ibus-engine-simple'
ISO_PATH='/usr/share/xml/iso-codes/iso_639.xml'
PY3K = sys.version_info >= (3, 0)

if PY3K:
    from io import StringIO
else:
    # io.StringIO does not work with XMLGenerator
    from cStringIO import StringIO
    # iso_639.xml includes UTF-8
    reload(sys)
    sys.setdefaultencoding('utf-8')

def usage(prgname):
    print('''\
%s Version %s
Usage:
  %s [OPTION...]

Options:
  -h, --help                         Show this message
  -i, --input=EVDEV_XML              Load EVDEV_XML file (default is:
                                         %s)
  -o, --output=FILE                  Output FILE (default is stdout)
  -V, --version=VERSION              Set IBus VERSION (default is %s)
  -e, --exec-path=EXEC_PATH          Set EXEC_PATH file (default is:
                                         %s)
  -I, --iso-path=ISO_PATH            Load ISO_PATH file (default is:
                                         %s)
  -1, --first-language               Pull first language only in language list
''' % (prgname, VERSION, prgname, EVDEV_XML, VERSION, EXEC_PATH, ISO_PATH))


class EvdevXML(XMLFilterBase):
    def __init__(self, parser=None, downstream=None, iso639=None,
                 denylist=None, author=None, first=False, io=False):
        XMLFilterBase.__init__(self, parser)
        self.__downstream = downstream
        self.__iso639 = iso639
        self.__denylist = denylist
        self.__author = author
        self.__first = first
        self.__io = io
        self.__is_layout = False
        self.__is_description = False
        self.__is_config_item = False
        self.__is_variant = False
        self.__is_iso639 = False
        self.__is_name = False
        self.__layout = ''
        self.__description = ''
        self.__variant = ''
        self.__list_iso639 = []
        self.__list_iso639_for_variant = []
    def startDocument(self):
        if self.__downstream:
            self.__downstream.startDocument()
            dtd = '<!DOCTYPE component SYSTEM "component.dtd">'
            self.__io.write(dtd)
            self.__downstream.startElement('engines', AttributesImpl({}))
    def endDocument(self):
        if self.__downstream:
            self.__downstream.endElement('engines')
            self.__downstream.endDocument()
    def startElement(self, name, attrs):
        if name == 'layout':
            self.__is_layout = True
        elif name == 'description':
            self.__is_description = True
        elif name == 'configItem':
            self.__is_config_item = True
        elif name == 'languageList':
            if self.__is_variant and self.__is_config_item:
                self.__list_iso639_for_variant = []
            elif self.__is_layout and self.__is_config_item:
                self.__list_iso639 = []
        elif name == 'iso639Id':
            self.__is_iso639 = True
        elif name == 'variant':
            self.__is_variant = True
        elif name == 'name':
            self.__is_name = True
    def endElement(self, name):
        if name == 'layout':
            self.__is_layout = False
            self.__layout = ''
            self.__description = ''
            self.__variant = ''
            self.__list_iso639 = []
        elif name == 'description':
            self.__is_description = False
        elif name == 'configItem':
            self.save()
            self.__is_config_item = False
        elif name == 'iso639Id':
            self.__is_iso639 = False
        elif name == 'variant':
            self.__is_variant = False
            self.__list_iso639_for_variant = []
        elif name == 'name':
            self.__is_name = False
    def characters(self, text):
        if self.__is_description:
            self.__description = text
        elif self.__is_name:
            if self.__is_variant and self.__is_config_item:
                self.__variant = text
            elif self.__is_layout and self.__is_config_item:
                self.__layout = text
        elif self.__is_iso639:
            if self.__is_variant and self.__is_config_item:
                self.__list_iso639_for_variant.append(text)
            elif self.__is_layout and self.__is_config_item:
                self.__list_iso639.append(text)
    def save(self):
        if not self.__downstream:
            return
        list_iso639 = []
        if self.__is_variant and self.__is_config_item:
            list_iso639 = self.__list_iso639_for_variant
            if len(list_iso639) == 0:
                list_iso639 = self.__list_iso639
        elif self.__is_layout and self.__is_config_item:
            list_iso639 = self.__list_iso639
        for iso in list_iso639:
            do_deny = False
            for [xkb, layout, variant, lang] in self.__denylist:
                if xkb == 'xkb' \
                   and ( layout == self.__layout or layout == '*' ) \
                   and ( variant == self.__variant or variant == '*' ) \
                   and ( lang == iso or variant == '*' ):
                    do_deny = True
                    break
            if do_deny:
                continue
            self.__downstream.startElement('engine', AttributesImpl({}))
            self.__downstream.startElement('name', AttributesImpl({}))
            name = 'xkb:%s:%s:%s' % (
                self.__layout,
                self.__variant,
                iso
            )
            self.__downstream.characters(name)
            self.__downstream.endElement('name')
            self.__downstream.startElement('longname', AttributesImpl({}))
            self.__downstream.characters(self.__description)
            self.__downstream.endElement('longname')
            self.__downstream.startElement('language', AttributesImpl({}))
            iso639_1 = self.__iso639.code2to1(iso)
            if iso639_1 != None:
                iso = iso639_1
            self.__downstream.characters(iso)
            self.__downstream.endElement('language')
            self.__downstream.startElement('layout', AttributesImpl({}))
            self.__downstream.characters(self.__layout)
            self.__downstream.endElement('layout')
            self.__downstream.startElement('license', AttributesImpl({}))
            self.__downstream.characters('GPL')
            self.__downstream.endElement('license')
            if self.__author != None:
                self.__downstream.startElement('author', AttributesImpl({}))
                self.__downstream.characters(self.__author)
                self.__downstream.endElement('author')
            self.__downstream.startElement('icon', AttributesImpl({}))
            self.__downstream.characters('ibus-keyboard')
            self.__downstream.endElement('icon')
            self.__downstream.startElement('description', AttributesImpl({}))
            self.__downstream.characters(self.__description)
            self.__downstream.endElement('description')
            self.__downstream.startElement('rank', AttributesImpl({}))
            if self.__variant == '':
                self.__downstream.characters('50')
            else:
                self.__downstream.characters('1')
            self.__downstream.endElement('rank')
            if self.__variant != '':
                self.__downstream.startElement('layout_variant',
                                               AttributesImpl({}))
                self.__downstream.characters(self.__variant)
                self.__downstream.endElement('layout_variant')
            self.__downstream.endElement('engine')
            if self.__first:
                break


class GenerateEngineXML():
    _NAME = 'org.freedesktop.IBus.Simple'
    _DESCRIPTION = 'A table based simple engine'
    _AUTHOR = 'Peng Huang <shawn.p.huang@gmail.com>'
    _HOMEPAGE = 'https://github.com/ibus/ibus/wiki'
    _DOMAIN = 'ibus'
    def __init__(self, path, iso639=None, denylist='', version='', exec='',
                 first=False):
        self.__path = path
        self.__iso639 = iso639
        self.__denylist = denylist
        self.__version = version
        self.__exec = exec
        self.__first = first
        self.__result = StringIO()
        downstream = XMLGenerator(self.__result, 'utf-8')
        self.__load(downstream)

    def __load(self, downstream=None):
        parser = sax_make_parser()
        parser.setFeature(sax_feature_namespaces, 0)
        self.__handler = EvdevXML(parser,
                                  downstream,
                                  self.__iso639,
                                  self.__denylist,
                                  self._AUTHOR,
                                  self.__first,
                                  self.__result)
        parser.setContentHandler(self.__handler)
        f = open(self.__path, 'r', encoding='utf-8')
        try:
            parser.parse(f)
        except SAXParseException:
            print('Error: Invalid file format: %s' % path)
        finally:
            f.close()
    def write(self, output=None):
        if output != None:
            od = open(output, 'w', encoding='utf-8')
        else:
            if PY3K:
                od = io.TextIOWrapper(sys.stdout.buffer, encoding='utf-8')
            else:
                od = codecs.getwriter('utf-8')(sys.stdout)
        contents = self.__result.getvalue()
        index = contents.find('<engines>')
        if index >= 0:
            author = escape(self._AUTHOR)
            contents = '%s<component><name>%s</name>\
<description>%s</description><exec>%s</exec><version>%s</version>\
<license>%s</license><author>%s</author><homepage>%s</homepage>\
<textdomain>%s</textdomain>%s</component>' % (
        contents[:index],
        self._NAME, self._DESCRIPTION,
        self.__exec, self.__version, 'GPL', author,
        self._HOMEPAGE, self._DOMAIN, contents[index:] )
        parsed = minidom.parseString(contents)
        # format with indent and encoding attribute in header
        xml = parsed.toprettyxml(indent='    ', encoding='utf-8')
        # convert byte to str
        od.write(str(xml, 'utf-8'))
        #od.write(contents)


class ISO639XML(XMLFilterBase):
    def __init__(self, parser=None):
        self.__code2to1 = {}
        self.__codetoname = {}
        XMLFilterBase.__init__(self, parser)
    def startElement(self, name, attrs):
        if name != 'iso_639_entry':
            return
        n = attrs.get('name')
        iso639_1 = attrs.get('iso_639_1_code')
        iso639_2b = attrs.get('iso_639_2B_code')
        iso639_2t = attrs.get('iso_639_2T_code')
        if iso639_1 != None:
            self.__codetoname[iso639_1] = n
            if iso639_2b != None:
                self.__code2to1[iso639_2b] = iso639_1
                self.__codetoname[iso639_2b] = n
            if iso639_2t != None and iso639_2b != iso639_2t:
                self.__code2to1[iso639_2t] = iso639_1
                self.__codetoname[iso639_2t] = n
    def code2to1(self, iso639_2):
        try:
            return self.__code2to1[iso639_2]
        except KeyError:
            return None


def parse_iso639(path):
    f = open(path, 'r', encoding='utf-8')
    parser = sax_make_parser()
    parser.setFeature(sax_feature_namespaces, 0)
    handler = ISO639XML(parser)
    parser.setContentHandler(handler)
    try:
        parser.parse(f)
    except SAXParseException:
        print('Error: Invalid file format: %s' % path)
    finally:
        f.close()
    return handler


def parse_denylist(denyfile):
    denylist = []
    f = open(denyfile, 'r', encoding='utf-8')
    for line in f.readlines():
        if line == '\n' or line[0] == '#':
            continue
        line = line.rstrip()
        entry = line.split(':')
        if len(entry) != 4:
            print('WARNING: format error: \'%s\' against \'%s\'' \
                  % (line, 'xkb:layout:variant:lang'))
            continue
        denylist.append(entry)
    f.close()
    return denylist


if __name__ == '__main__':
    prgname = os.path.basename(sys.argv[0])
    mydir = os.path.dirname(sys.argv[0])
    try:
        opts, args = getopt.getopt(sys.argv[1:],
                                   'hi:o:V:e:I:1',
                                   ['help', 'input=', 'output=', 'version=',
                                    'exec-path=', 'iso-path=',
                                    'first-language'])
    except getopt.GetoptError as err:
        print(err)
        usage(prgname)
        sys.exit(2)
    if len(args) > 0:
        usage(prgname)
        sys.exit(2)
    input = EVDEV_XML
    output = None
    version=VERSION
    exec_path=EXEC_PATH
    iso_path=ISO_PATH
    first=False
    for opt, arg in opts:
        if opt in ('-h', '--help'):
            usage(prgname)
            sys.exit()
        elif opt in ('-i', '--input'):
            input = arg
        elif opt in ('-o', '--output'):
            output = arg
        elif opt in ('-V', '--version'):
            version = arg
        elif opt in ('-e', '--exec-path'):
            exec_path = arg
        elif opt in ('-I', '--iso-path'):
            iso_path = arg
        elif opt in ('-1', '--first-langauge'):
            first=True

    iso639 = parse_iso639(iso_path)
    denylist = parse_denylist('%s/%s' % ( mydir, 'denylist.txt'))
    xml = GenerateEngineXML(input, iso639, denylist, version, exec_path, first)
    xml.write(output)
