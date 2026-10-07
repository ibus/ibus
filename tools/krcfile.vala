/* vim:set et sts=4 sw=4:
 *
 * ibus - The Input Bus
 *
 * Copyright(c) 2024 Takao Fujiwara <takao.fujiwara1@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, see <https://www.gnu.org/licenses/>.
 */

struct KRcFileKeyValuePair {
    public string? key;
    public string? value;
}

struct KRcFileGroup {
    public string? name;
    public unowned GLib.List<KRcFileKeyValuePair?> key_value_pairs;
}

class KRcFile {
    // Make sure KRcFileGroup is a pointer for add_key_value_pair()
    private unowned KRcFileGroup? m_current_group;
    private GLib.List<KRcFileGroup?> m_groups;

    public KRcFile() {
        KRcFileGroup? group = KRcFileGroup();
        m_groups.prepend(group);
        // no name group for header comments
        // Make sure m_current_group does not duplicate the memory.
        m_current_group = (KRcFileGroup?)m_groups.data;
    }

    private bool is_comment(string line) {
        unichar ch = line.get_char(0);
        return ch == '#' || ch == '\0' || ch == '\n';
    }

    private bool is_group(string line) {
        if (line.get_char(0) != '[')
            return false;
        int i = 1;
        int length = line.char_count();
        while (i < length) {
            unichar ch = line.get_char(i);
            if (ch == ']')
                return true;
            i++;
        }
        return false;
    }

    // "[Tiling][12345678-9012-3456-7890-123456789012]" is supported.
    private bool is_group_name(string name) {
        int i = 0;
        int length = name.char_count();
        unichar ch = 0;
        while (i < length) {
            ch = name.get_char(i);
            if (ch == '\0' || ch.iscntrl())
                break;
            i++;
        }
        if (i != 0 && (i == length || ch == '\0'))
            return true;
        return false;
    }

    private bool is_key_value_pair(string line) {
        int pos = line.index_of_char('=');
        if (pos == -1 || pos == 0)
            return false;
        return true;
    }

    // Assume key name is ASCII only
    private bool is_key_name(string line, size_t length) {
        long i = 0;
        // Cast size_t to long.
        assert(length < Posix.Limits.SSIZE_MAX);
        while (i < length) {
            char ch = line.get(i);
            if (ch == '=' || ch == '[' || ch == ']')
                break;
            if (ch == '\0') {
                i = (long)length;
                break;
            }
            i++;
        }
        if (i == 0)
            return false;
        if (line.get(0) == ' ' || line.get(i - 1) == ' ')
            return false;
        if (i < length && line.get(i) == '[') {
            i++;
            while (i < length) {
                assert(Posix.Limits.SSIZE_MAX > length - i);
                unichar ch =
                        line.offset(i).get_char_validated((ssize_t)length - i);
                if (ch <= 0)
                    break;
                // kwinrc file supports "$e" language.
                if (!ch.isalnum() && ch != '-' && ch != '_' && ch != '.'
                    && ch != '@' && ch != '$') {
                    break;
                }
                i++;
            }
            if (i == length)
                return false;
            if (line.get(i) != ']')
                return false;
            // Now line[i] is ']'
            i++;
        }
        if (i < length)
            return false;
        return true;
    }

    private static int find_group(KRcFileGroup group1, KRcFileGroup group2) {
        if (group1.name == null) {
            if (group2.name == null)
                return 0;
            return -1;
        }
        if (group2.name == null)
            return 1;
        return group1.name.ascii_casecmp(group2.name);
    }

    private unowned GLib.List<KRcFileGroup?>
    get_group_node_with_name(string group_name,
                             bool if_throw_error=false)
                                     throws GLib.KeyFileError {
        var tmp = KRcFileGroup();
        tmp.name = group_name;
        unowned GLib.List<KRcFileGroup?> group_node =
                m_groups.find_custom(tmp,
                                     (GLib.CompareFunc)find_group);
        if (group_node == null && if_throw_error) {
            throw new GLib.KeyFileError.GROUP_NOT_FOUND(
                    "Key file does not have group %s", group_name);
        }
        return group_node;
    }

    private void add_key_value_pair(KRcFileGroup?       group,
                                    KRcFileKeyValuePair pair,
                                    GLib.List           sibling) {
        group.key_value_pairs.insert_before(sibling, pair);
    }

    private void add_key(KRcFileGroup? group,
                         string?       key,
                         string        value) {
        KRcFileKeyValuePair pair = KRcFileKeyValuePair();
        pair.key = key;
        pair.value = value;
        unowned GLib.List<KRcFileKeyValuePair?> lp = group.key_value_pairs;
        while (lp != null && lp.data.key == null)
            lp = lp.next;
        add_key_value_pair(group, pair, lp);
    }

    private void parse_comment(string line, long length) {
        KRcFileKeyValuePair pair = KRcFileKeyValuePair();
        pair.key = null;
        pair.value = line.substring(0, length);
        m_current_group.key_value_pairs.prepend(pair);
    }

    private void parse_group(string line,
                             long   length) throws GLib.KeyFileError {
        int l = (int)length;
        if (l < 0) {
            throw new GLib.KeyFileError.PARSE(
                    "Too long group name %s against IMT_MAX", line);
        }
        assert(l > 2);
        unichar ch;
        while (line.get_prev_char(ref l, out ch)) {
            if (ch != ']')
                break;
        }
        string group_name = line.substring(1, l);
        if (!is_group_name(group_name)) {
            throw new GLib.KeyFileError.PARSE(
                    "Invalid group name %s", group_name);
        }
        unowned GLib.List<KRcFileGroup?> group_node =
                get_group_node_with_name(group_name);
        if (group_node != null) {
            m_current_group = (KRcFileGroup?)group_node.data;
            return;
        }
        KRcFileGroup group = KRcFileGroup();
        group.name = group_name;
        m_groups.prepend(group);
        m_current_group = (KRcFileGroup?)m_groups.data;
    }

    private void parse_key_value_pair(string line,
                                      long   length) throws GLib.KeyFileError {
        if (m_current_group.name == null) {
            throw new GLib.KeyFileError.GROUP_NOT_FOUND(
                    "Key file does not start with a group");
        }
        int key_end = line.index_of_char('=');
        int value_start = key_end;
        warn_if_fail(key_end != -1);
        key_end--;
        value_start++;
        string key = line.substring(0, key_end + 1);
        key = key.chomp();
        if (!is_key_name(line, key_end + 1)) {
            throw new GLib.KeyFileError.PARSE(
                    "Invalid key name: %s", key);
        }
        string value = line.substring(value_start, line.length - value_start);
        value = value.chug();
        assert(m_current_group.name != null);
        if (key == "Encoding" && value != "UTF-8") {
            throw new GLib.KeyFileError.UNKNOWN_ENCODING(
                    "Key file contains unsupported encoding %s", value);
        }
        var pair = KRcFileKeyValuePair();
        pair.key = key;
        pair.value = value;
        add_key_value_pair(m_current_group,
                           pair,
                           m_current_group.key_value_pairs);
    }

    private void parse_line(string line, long length) throws GLib.KeyFileError {
        // unowned to owned string
        string _line = line;
        _line = _line.chug();
        if (is_comment(_line))
            parse_comment(line, length);
        if (is_group(_line))
            parse_group(_line, length - (length - _line.length));
        if (is_key_value_pair(_line))
            parse_key_value_pair(_line, length - (length - _line.length));
    }

    // Use data.get() instead of data.get_char() and support UTF-8 only with
    // parse_key_value_pair()
    private bool parse_data(string data, size_t length) {
        long i = 0;
        long offset = 0;
        while (i < length) {
            char ch = data.get(i);
            if (ch == '\n') {
                long sub_length = i - offset;
                string sub = data.substring(offset, sub_length);
                offset = i + 1;
                if (sub.length > 0 && sub.get(sub.length - 1) == '\r')
                    sub = sub.substring(0, --sub_length);
                try {
                    if (sub.length == 0)
                        parse_comment("", 0);
                    else
                        parse_line(sub, sub_length);
                } catch (GLib.KeyFileError e) {
                    warning("Failed to load data: %s", e.message);
                    return false;
                }
            }
            i++;
        }
        return true;
    }

    public bool load_from_file(string            path,
                               GLib.KeyFileFlags flags)
                                       throws GLib.KeyFileError,
                                              GLib.FileError  {
        if (!GLib.FileUtils.test(path, GLib.FileTest.EXISTS |
                                       GLib.FileTest.IS_REGULAR)) {
            throw new GLib.KeyFileError.PARSE(
                    "Not a regular file %s", path);
        }
        string contents = null;
        size_t length = 0;
        if (!GLib.FileUtils.get_contents(path, out contents, out length)) {
            warning("Failed to load %s", path);
            return false;
        }
        return parse_data(contents, length);
    }

    public bool has_group(string group_name) {
        try {
            if (get_group_node_with_name(group_name) != null)
                return true;
        } catch (GLib.KeyFileError e) {
            assert_not_reached();
        }
        return false;
    }

    public string[] get_keys(string group_name) throws GLib.KeyFileError {
        string[] keys = {};
        unowned GLib.List<KRcFileGroup?> group_node =
                get_group_node_with_name(group_name, true);
        unowned GLib.List<KRcFileKeyValuePair?> tmp =
                group_node.data.key_value_pairs;
        while (tmp != null) {
            KRcFileKeyValuePair pair = tmp.data;
            if (pair.key != null)
                keys += pair.key;
            tmp = tmp.next;
        }
        return keys;
    }

    public bool has_key(string group_name,
                        string key) throws GLib.KeyFileError {
        unowned GLib.List<KRcFileGroup?> group_node =
                get_group_node_with_name(group_name, true);
        unowned GLib.List<KRcFileKeyValuePair?> tmp =
                group_node.data.key_value_pairs;
        while (tmp != null) {
            KRcFileKeyValuePair pair = tmp.data;
            if (pair.key == key)
                return true;
            tmp = tmp.next;
        }
        return false;
    }

    public bool remove_group(string group_name) throws GLib.KeyFileError {
        unowned GLib.List<KRcFileGroup?> group_node =
                get_group_node_with_name(group_name, true);
        unowned GLib.List<KRcFileKeyValuePair?> tmp =
                group_node.data.key_value_pairs;
        while (tmp != null) {
            tmp.data = null;
            tmp.delete_link(tmp);
        }
        group_node.data = null;
        m_groups.delete_link(group_node);
        return true;
    }

    public bool remove_key(string group_name,
                           string key) throws GLib.KeyFileError {
        unowned GLib.List<KRcFileGroup?> group_node =
                get_group_node_with_name(group_name, true);
        unowned GLib.List<KRcFileKeyValuePair?> tmp =
                group_node.data.key_value_pairs;
        while (tmp != null) {
            KRcFileKeyValuePair? pair = tmp.data;
            if (pair.key == key)
                break;
            tmp = tmp.next;
        }
        if (tmp == null) {
            throw new GLib.KeyFileError.KEY_NOT_FOUND(
                    "Key file does not have key %s in group %s",
                    key, group_name);
        }
        tmp.data = null;
        tmp.delete_link(tmp);
        return true;
    }

    public string to_data(out ssize_t length) {
        var data_string = new GLib.StringBuilder();
        for (unowned GLib.List<KRcFileGroup?> group_node = m_groups.last();
             group_node != null;
             group_node = group_node.prev) {
             KRcFileGroup? group = group_node.data;
             if (group.name != null)
                 data_string.append_printf("[%s]\n", group.name);
             for (unowned GLib.List<KRcFileKeyValuePair?> key_file_node =
                          group.key_value_pairs.last();
                  key_file_node != null;
                  key_file_node = key_file_node.prev) {
                 KRcFileKeyValuePair pair = key_file_node.data;
                 if (pair.key != null)
                     data_string.append_printf("%s=%s\n", pair.key, pair.value);
                 else
                     data_string.append_printf("%s\n", pair.value);
             }
        }
        length = data_string.len;
        return data_string.str;
    }

    public bool save_to_file(string filename) throws GLib.FileError {
        ssize_t length = 0;
        string contents = to_data(out length);
        bool retval = GLib.FileUtils.set_contents(filename, contents, length);
        return retval;
    }

    public string? get_value(string group_name,
                             string key) throws GLib.KeyFileError {
        unowned GLib.List<KRcFileGroup?> group_node =
                get_group_node_with_name(group_name, true);
        unowned GLib.List<KRcFileKeyValuePair?> tmp =
                group_node.data.key_value_pairs;
        while (tmp != null) {
            KRcFileKeyValuePair? pair = tmp.data;
            if (pair.key == key)
                return pair.value;
            tmp = tmp.next;
        }
        throw new GLib.KeyFileError.KEY_NOT_FOUND(
                "Key file does not have key %s in group %s",
                key, group_name);
    }

    public void set_value(string group_name,
                          string key,
                          string value) {
        return_if_fail(is_group_name(group_name));
        return_if_fail(is_key_name(key, key.length));
        unowned GLib.List<KRcFileGroup?> group_node;
        try {
            group_node = get_group_node_with_name(group_name);
        } catch (GLib.KeyFileError e) {
            assert_not_reached();
        }
        if (group_node == null) {
            unowned KRcFileGroup? last_group = m_groups.data;
            if (last_group.key_value_pairs != null) {
                if (last_group.key_value_pairs.data.key != null)
                    parse_comment("", 0);
            }
            KRcFileGroup group = KRcFileGroup();
            group.name = group_name;
            m_groups.prepend(group);
            m_current_group = (KRcFileGroup?)m_groups.data;
            add_key(m_current_group, key, value);
        } else {
            unowned GLib.List<KRcFileKeyValuePair?> tmp =
                    group_node.data.key_value_pairs;
            while (tmp != null) {
                KRcFileKeyValuePair? pair = tmp.data;
                if (pair.key == key)
                    break;
                tmp = tmp.next;
            }
            if (tmp != null)
                tmp.data.value = value;
            else
                add_key(group_node.data, key, value);
        }
    }

}
