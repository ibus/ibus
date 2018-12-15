#!/usr/bin/python
# vim:set fileencoding=utf-8 et sts=4 sw=4:
#
# ibus - Intelligent Input Bus for Linux / Unix OS
#
# Copyright © 2016 Takao Fujiwara <takao.fujiwara1@gmail.com>
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


# This script converts ISO 639-2 of three characters to ISO 639-1 of two
# characters in simple.xml.
# E.g. "eng" to "en"


from xml.sax import make_parser as sax_make_parser
from xml.sax.handler import feature_namespaces as sax_feature_namespaces
from xml.sax.saxutils import XMLFilterBase, XMLGenerator
from xml.sax._exceptions import SAXParseException

import codecs
import getopt
import io
import os
import sys

INSTALLED_SIMPLE_XML = '/usr/share/ibus/component/simple.xml'
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
Usage:
  %s [OPTION...]

Options:
  -h, --help                         Show this message
  -i, --input=SIMPLE_XML             Load SIMPLE_XML file (default is:
                                         %s)
  -o, --output=FILE                  Output FILE (default is stdout)
''' % (prgname, INSTALLED_SIMPLE_XML))


class ISO639XML(XMLFilterBase):
    def __init__(self, parser=None):
        self.__code2to3 = {}
        self.__codetoname = {}
        XMLFilterBase.__init__(self, parser)
    def startElement(self, name, attrs):
        if name != 'iso_639_3_entry':
            return
        n = attrs.get('name')
        iso639_3 = attrs.get('id')
        iso639_2b = attrs.get('part1_code')
        iso639_2t = attrs.get('part2_code')
        if iso639_3 != None:
            self.__codetoname[iso639_3] = n
            if iso639_2b != None:
                self.__code2to3[iso639_2b] = iso639_3
                self.__codetoname[iso639_2b] = n
            if iso639_2t != None and iso639_2b != iso639_2t:
                self.__code2to3[iso639_2t] = iso639_3
                self.__codetoname[iso639_2t] = n
    def code2to3(self, iso639_2):
        try:
            return self.__code2to3[iso639_2]
        except KeyError:
            return None


class IBusComponentXML(XMLFilterBase):
    def __init__(self, parser=None, downstream=None, iso639=None):
        XMLFilterBase.__init__(self, parser)
        self.__downstream = downstream
        self.__iso639 = iso639
        self.__is_language = False
    def startDocument(self):
        if self.__downstream:
            self.__downstream.startDocument()
    def endDocument(self):
        if self.__downstream:
            self.__downstream.endDocument()
    def startElement(self, name, attrs):
        if name == 'language':
            self.__is_language = True
        if self.__downstream:
            self.__downstream.startElement(name, attrs)
    def endElement(self, name):
        if name == 'language':
            self.__is_language = False
        if self.__downstream:
            self.__downstream.endElement(name)
    def characters(self, text):
        if self.__is_language:
            if self.__iso639:
                iso639_3 = self.__iso639.code2to3(text)
                if iso639_3 != None:
                    text = iso639_3
        if self.__downstream:
            self.__downstream.characters(text)


class ConvertEngineXML():
    def __init__(self, path, iso639=None):
        self.__path = path
        self.__iso639 = iso639

        self.__result = StringIO()
        downstream = XMLGenerator(self.__result, 'utf-8')
        self.__load(downstream)

    def __load(self, downstream=None):
        parser = sax_make_parser()
        parser.setFeature(sax_feature_namespaces, 0)
        self.__handler = IBusComponentXML(parser, downstream, self.__iso639)
        parser.setContentHandler(self.__handler)
        f = codecs.open(self.__path, 'r', encoding='utf-8')
        try:
            parser.parse(f)
        except SAXParseException:
            print('Error: Invalid file format: %s' % path)
        finally:
            f.close()
    def write(self, output=None):
        if output != None:
            od = codecs.open(output, 'w', encoding='utf-8')
        else:
            if PY3K:
                od = io.TextIOWrapper(sys.stdout.buffer, encoding='utf-8')
            else:
                od = codecs.getwriter('utf-8')(sys.stdout)
        contents = self.__result.getvalue()
        od.write(contents)


def parse_iso639(path):
    f = codecs.open(path, 'r', encoding='utf-8')
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


if __name__ == '__main__':
    prgname = os.path.basename(sys.argv[0])
    try:
        opts, args = getopt.getopt(sys.argv[1:],
                                   'hi:o:',
                                   ['help', 'input=', 'output='])
    except getopt.GetoptError as err:
        print(err)
        usage(prgname)
        sys.exit(2)
    if len(args) > 0:
        usage(prgname)
        sys.exit(2)
    input = INSTALLED_SIMPLE_XML
    output = None
    for opt, arg in opts:
        if opt in ('-h', '--help'):
            usage(prgname)
            sys.exit()
        elif opt in ('-i', '--input'):
            input = arg
        elif opt in ('-o', '--output'):
            output = arg

    iso639 = parse_iso639('/usr/share/xml/iso-codes/iso_639-3.xml')
    xml = ConvertEngineXML(input, iso639)
    xml.write(output)
