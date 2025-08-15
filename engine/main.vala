/* vim:set et sts=4 sw=4:
 *
 * ibus - The Input Bus
 *
 * Copyright (c) 2011-2013 Peng Huang <shawn.p.huang@gmail.com>
 * Copyright (c) 2015-2025 Takao Fujiwara <takao.fujiwara1@gmail.com>
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
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301
 * USA
 */

using IBus;

static IBus.EngineSimple m_simple;

class DummyEngine : IBus.EngineSimple {
}

public int main(string[] args) {
    Intl.setlocale(LocaleCategory.ALL, "");
    IBus.init();

    IBus.Bus bus = new IBus.Bus();
    if (!bus.is_connected()) {
        warning("ibus-daemon does not exist.");
        return 1;
    }

    bus.disconnected.connect((bus) => {
        debug("bus disconnected");
        IBus.quit();
    });

    IBus.Factory factory = new IBus.Factory(bus.get_connection());
    
    int id = 0;

    factory.create_engine.connect((factory, name) => {
        if (!name.has_prefix("xkb:"))
            return null;

        const string path = "/org/freedesktop/IBus/engine/simple/%d";

        IBus.Engine engine = new IBus.Engine.with_type(
            typeof(IBus.EngineSimple), name,
            path.printf(++id), bus.get_connection());
        /* the type change causes engine.ref(). */
        m_simple = (IBus.EngineSimple)engine;
        m_simple.unref();

        /* Use Idle.add() to reduce the lag caused by file io */
        GLib.Idle.add(() => {
            /* I think "c" + "'" is c with acute U+0107 and
             * "c" + "," is c with cedilla U+00E7 and they are
             * visually comprehensible. But pt-br people need
             * "c" + "'" is c with cedilla and I think the
             * cedilla_compose_seqs is needed for the specific keyboards
             * or regions.
             * X11 uses compose by locale:
             * In /usr/share/X11/locale/en_US.UTF-8/Compose ,
             * <Multi_key> <apostrophe> <c> : U0107
             */
            m_simple.add_table_by_locale(null);

            string user_file = null;

            var home = GLib.Environment.get_home_dir();
            if (home != null) {
                user_file = home + "/.XCompose";
                if (GLib.FileUtils.test(user_file, GLib.FileTest.EXISTS))
                    m_simple.add_compose_file(user_file);
            }

            user_file = GLib.Environment.get_variable("XCOMPOSEFILE");
            if (user_file != null) {
                if (GLib.FileUtils.test(user_file, GLib.FileTest.EXISTS))
                    m_simple.add_compose_file(user_file);
            }

            return false;
        });

        return engine;
    });

    uint flags = 
        IBus.BusNameFlag.REPLACE_EXISTING |
        IBus.BusNameFlag.ALLOW_REPLACEMENT;
    uint retval = bus.request_name("org.freedesktop.IBus.Simple", flags);

    if (retval == 0) {
        warning("Registry bus name org.freedesktop.IBus.Simple failed!");
        return 1;
    }

    IBus.main();
    m_simple = null;

    return 0;
}
