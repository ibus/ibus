# vim:set et sts=4 sw=4:
#
# ibus - The Input Bus
#
# Copyright (c) 2007-2010 Peng Huang <shawn.p.huang@gmail.com>
# Copyright (c) 2007-2010 Red Hat, Inc.
#
# This library is free software; you can redistribute it and/or
# modify it under the terms of the GNU Lesser General Public
# License as published by the Free Software Foundation; either
# version 2 of the License, or (at your option) any later version.
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

import cgi
import urllib2
from xml.dom import minidom

def simplify_dom(node):
    name = node.nodeName
    children = {}
    if len(node.childNodes) == 1 and node.childNodes[0].nodeType == node.TEXT_NODE:
        return name, node.childNodes[0].nodeValue
    for child in node.childNodes:
        if child.nodeType != node.ELEMENT_NODE:
            continue
        child_name, child_value = simplify_dom(child)
        if child_name not in children:
            children[child_name] = []
        children[child_name].append(child_value)
    return name, children

def parse_xml():
    filename = "/usr/share/X11/xkb/rules/evdev.xml"
    dom = minidom.parse(file(filename))
    name, root = simplify_dom(dom)

    layouts = root['xkbConfigRegistry'][0]['layoutList'][0]['layout']
    for layout in layouts:
        config = layout['configItem'][0]
        name = config['name'][0]
        short_desc = config.get('shortDescription', [''])[0]
        desc = config.get('description', [''])[0]
        languages = config.get('languageList', [{}])[0].get('iso639Id', [])
        variants = layout.get('variantList', [{}])[0].get('variant', [])
        yield name, '', short_desc, desc, languages
        for variant in variants:
            variant_config = variant['configItem'][0]
            variant_name = variant_config['name'][0]
            variant_short_desc = variant_config.get('shortDescription', [''])[0]
            variant_desc = variant_config.get('description', [''])[0]
            variant_languages = variant_config.get('languageList', [{}])[0].get('iso639Id', [])
            if not isinstance(variant_languages, list):
                variant_languages = [variant_languages]
            yield name, variant_name, variant_short_desc, variant_desc, languages + variant_languages

def gen_xml():
    header = u"""<?xml version="1.0" encoding="utf-8"?>
<component>
	<name>org.freedesktop.IBus.Simple</name>
	<description>A table based simple engine</description>
	<exec>@libexecdir@/ibus-engine-simple</exec>
	<version>@VERSION@</version>
	<author>Peng Huang &lt;shawn.p.huang@gmail.com&gt;</author>
	<license>GPL</license>
	<homepage>http://code.google.com/p/ibus</homepage>
	<textdomain>ibus</textdomain>
	<engines>"""
    engine = u"""\t\t<engine>
			<name>%s</name>
			<language>%s</language>
			<license>GPL</license>
			<author>Peng Huang &lt;shawn.p.huang@gmail.com&gt;</author>
			<layout>%s</layout>
			<longname>%s</longname>
			<description>%s</description>
                        <icon>ibus-keyboard</icon>
			<rank>%d</rank>
		</engine>"""
    footer = u"""\t</engines>
</component>"""

    print header

    whitelist = parse_whitelist()
    for name, vname, sdesc, desc, languages in parse_xml():
        layout = "%s(%s)" % (name, vname) if vname else name
        for lang in languages:
            ibus_name = "xkb:%s:%s:%s" % (name, vname, lang)
            if ibus_name not in whitelist:
                continue
            desc = cgi.escape(desc)
            out = engine % (ibus_name, lang, layout, desc, desc, 99)
            print out.encode("utf8")

    print footer

def parse_whitelist():
    url = "http://git.chromium.org/gitweb/?p=chromium/chromium.git;a=blob_plain;f=chrome/browser/chromeos/input_method/ibus_input_methods.txt;hb=HEAD"
    whitelist = []
    for line in urllib2.urlopen(url):
        line = line.strip()
        if not line:
            continue
        if line.startswith("#"):
            continue
        whitelist.append(line.split()[0])
    return set(whitelist)

if __name__ == "__main__":
    gen_xml()
