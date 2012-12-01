/* vim:set et sts=4 sw=4:
 *
 * ibus - The Input Bus
 *
 * Copyright(c) 2011 Peng Huang <shawn.p.huang@gmail.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or(at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this program; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place, Suite 330,
 * Boston, MA  02111-1307  USA
 */

Pango.AttrList get_pango_attr_list_from_ibus_text(IBus.Text text) {
    Pango.AttrList pango_attrs = new Pango.AttrList();
    unowned IBus.AttrList attrs = text.get_attributes();
    if (attrs == null)
        return pango_attrs;

    unowned string str = text.get_text();
    long nchars = str.char_count();
    long[] offsets = new long[nchars + 1];
    for (int i = 0; i <= nchars; i++)
        offsets[i] = str.index_of_nth_char(i);

    IBus.Attribute attr;
    int i = 0;
    while(true) {
        attr = attrs.get(i++);
        if (attr == null)
            break;
        long start_index =  attr.start_index;
        if (start_index <= 0) start_index = 0;
        start_index = start_index <= nchars ? offsets[start_index] : offsets[-1];

        long end_index = attr.end_index;
        if (end_index <= 0) end_index = 0;
        end_index = end_index <= nchars ? offsets[end_index] : offsets[-1];

        Pango.Attribute pango_attr = null;
        switch(attr.type) {
        case IBus.AttrType.FOREGROUND:
            {
                uint16 r = (uint16)((attr.value & 0x00ff0000) >> 8);
                uint16 g = (uint16)(attr.value & 0x0000ff00);
                uint16 b = (uint16)(attr.value & 0x000000ff) << 8;
                pango_attr = Pango.attr_foreground_new(r, g, b);
                break;
            }
        case IBus.AttrType.BACKGROUND:
            {
                uint16 r = (uint16)((attr.value & 0x00ff0000) >> 8);
                uint16 g = (uint16)(attr.value & 0x0000ff00);
                uint16 b = (uint16)(attr.value & 0x000000ff) << 8;
                pango_attr = Pango.attr_background_new(r, g, b);
                break;
            }
        case IBus.AttrType.UNDERLINE:
            {
                pango_attr = Pango.attr_underline_new((Pango.Underline)attr.value);
                break;
            }
        default:
            continue;
        }
        pango_attr.start_index = (uint)start_index;
        pango_attr.end_index = (uint)end_index;
        // Transfer the ownership to pango_attrs
        pango_attrs.insert((owned)pango_attr);
    }
    return pango_attrs;
}
