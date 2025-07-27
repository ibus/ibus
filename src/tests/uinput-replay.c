/* -*- mode: C; c-basic-offset: 4; indent-tabs-mode: nil; -*- */
/* vim:set et sts=4: */
/* ibus - The Input Bus
 * Copyright (c) 2025 Peter Hutterer <peter.hutterer@who-t.net>
 * Copyright (C) 2025 Takao Fujiwara <takao.fujiwara1@gmail.com>
 * Copyright (C) 2025 Red Hat, Inc.
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

#include <glib.h>

#include <errno.h>
#include <grp.h>
#include <linux/input.h>
#include <linux/input-event-codes.h>
#include <libevdev/libevdev-uinput.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <unistd.h>

#define msleep(t) usleep((t) * 1000)


static gchar *
get_case_contents (const gchar *case_path)
{
    gchar *contents = NULL;
    gsize length = 0;
    GError *error = NULL;

    g_return_val_if_fail (case_path, NULL);
    if (!g_file_get_contents (case_path, &contents, &length, &error)) {
        g_warning ("Failed to open %s: %s", case_path, error->message);
        g_error_free (error);
    } else if (length == 0) {
        g_warning ("No contents file %s", case_path);
    }
    return contents;
}


static struct libevdev_uinput *
ibus_uidev_new (void)
{
    struct libevdev *dev = libevdev_new ();
    unsigned int code;
    struct libevdev_uinput *uidev = NULL;
    int retval;
    const char *syspath;
    const char *devnode;
    struct stat buf;
    struct group *grp = NULL;

    libevdev_set_name (dev, "ibusdev");
    /* serial makes it an internal keyboard */
    libevdev_set_id_bustype (dev, BUS_I8042);

    for (code = KEY_ESC; code < BTN_MISC; code++)
        libevdev_enable_event_code (dev, EV_KEY, code, NULL);

    retval = libevdev_uinput_create_from_device (
            dev,
            LIBEVDEV_UINPUT_OPEN_MANAGED,
            &uidev);

    if (retval) {
        g_warning ("Failed to create uinput: %s\n", g_strerror (-retval));
        libevdev_free (dev);
        return NULL;
    }

    libevdev_free (dev);


    syspath = libevdev_uinput_get_syspath (uidev);
    g_assert (syspath != NULL);
    devnode = libevdev_uinput_get_devnode (uidev);
    g_assert (devnode != NULL);

    /* You need to wait here until the device actually exists, either via a
     * sleep or checking udev until the device shows up. Use
     * libevdev_uinput_get_syspath() for the latter
     */
    errno = 0;
    do {
        msleep (10);
        if (stat (devnode, &buf)) {
            g_warning ("Failed to get stat of %s: %s\n",
                       devnode, g_strerror (errno));
            libevdev_uinput_destroy (uidev);
            return NULL;
        }
        if (!(grp = getgrgid (buf.st_gid))) {
            g_warning ("Failed to get gid of %s: %s\n",
                       devnode, g_strerror (errno));
            libevdev_uinput_destroy (uidev);
            return NULL;
        }
        g_debug ("gr_name %s", grp->gr_name);
    } while (strcmp (grp->gr_name, "input"));
    msleep (100);

    return uidev;
}


static gboolean
parse_array (const gchar *line,
             guint       *array)
{
    const gchar *head = line;
    int array_index = 0;

    g_assert (line);
    g_assert (array);

    while (*head != '\0') {
        gchar *end = NULL;
        array[array_index] = g_ascii_strtoull (head, &end, 10);
        while (*end != ',' && *end != ']' && *end != '\0') ++end;
        if (*end == ',') {
            ++array_index;
            head = end + 1;
        } else if (*end == ']') {
            if (array_index == 4) {
                return TRUE;
            } else {
                g_warning ("Fewer array index %d < 4", array_index);
                return FALSE;
            }
        } else {
            g_warning ("array does not enclose with ']'");
            return FALSE;
        }
    }
    return FALSE;
}


static gboolean
parse_evdev (const gchar            *line,
             guint                  *start_events,
             guint                  *start_evdev,
             guint                  *start_keys,
             guint                  *array)
{
    const gchar *p = line;
    guint indent;

    g_assert (line);
    g_assert (start_events);
    g_assert (start_evdev);
    g_assert (start_keys);

    while (*p == ' ') ++p;
    if (*p == '-') {
        ++p;
        while (*p == ' ') ++p;
    }
    if (*p == '#' || *p == '\0')
        return FALSE;
    indent = p - line;
    if (*start_events == 0 && !g_ascii_strncasecmp (p, "events", 6)) {
        *start_events = indent;
    } else if (*start_events > 0 && indent > *start_events  &&
               !g_ascii_strncasecmp (p, "evdev", 5)) {
        *start_evdev = indent;
    } else if (*start_events > 0 && *start_evdev > 0) {
        if (*p == '[') {
            if (*start_keys == 0) {
                *start_keys = indent;
            } else if (*start_keys != indent) {
                g_warning ("Wrong indent");
                *start_events = 0;
                *start_evdev = 0;
                *start_keys = 0;
                return FALSE;
            }
            if (parse_array (p + 1, array))
                return TRUE;
        } else {
            *start_events = 0;
            *start_evdev = 0;
            *start_keys = 0;
        }
    } else {
        *start_events = 0;
        *start_evdev = 0;
        *start_keys = 0;
    }
    return FALSE;
}


static gboolean
ibus_uidev_write_event_array (struct libevdev_uinput *uidev,
                              guint                  *array)
{
    static guint prev_time = 0;
    guint interval;
    int retval;

    g_assert (array);
    g_assert (array[1] >= prev_time);
    interval = array[1] - prev_time;
    prev_time = array[1];
    if (interval > 0) {
        g_debug ("usleep %u", interval);
        usleep(interval);
    }
    g_debug ("write uinput(%u, %u, %u, %u, %u)",
             array[0], array[1], array[2], array[3], array[4]);
    retval = libevdev_uinput_write_event (uidev, array[2], array[3], array[4]);
    if (retval) {
        g_warning ("Failed to write uinput(%u, %u, %u, %u, %u): %s",
                   array[0], array[1], array[2], array[3], array[4],
                   g_strerror (-retval));
        return FALSE;
    }
    return TRUE;
}


static void
ibus_uidev_replay_with_yaml_data (struct libevdev_uinput *uidev,
                                  const gchar            *contents)
{
    const gchar *head = contents;
    guint start_events = 0;
    guint start_evdev = 0;
    guint start_keys = 0;

    g_assert (uidev);
    g_assert (contents);
    while (*head == '\n') ++head;
    while (*head != '\0') {
        const gchar *end = head;
        gchar *line;
        guint array[5] = { 0, };

        while (*end != '\n' && *end != '\0') ++end;
        if (*head == '#') {
            head = end;
            if (*head != '\0')
                ++head;
            continue;
        }
        line = g_strndup (head, end - head);
        if (parse_evdev (line, &start_events, &start_evdev, &start_keys, array))
            if (!ibus_uidev_write_event_array (uidev, array))
                return;
        g_free (line);
        head = end;
        if (*head != '\0')
            ++head;
    }
}

int
main (int argc, char *argv[]) {
    gchar *contents;
    struct libevdev_uinput *uidev;

    if (argc == 1) {
        gchar *prgname = g_path_get_basename (argv[0]);
        g_warning ("Usage: %s libinput-test.yml", prgname);
        g_free (prgname);
        return EXIT_FAILURE;
    }

    if (!g_strcmp0 (argv[1], "--username")) {
        g_print ("%s\n", g_get_user_name ());
        return EXIT_SUCCESS;
    }

    if (!(contents = get_case_contents (argv[1])))
        return EXIT_FAILURE;
    if (!(uidev = ibus_uidev_new ()))
        return EXIT_FAILURE;

    ibus_uidev_replay_with_yaml_data (uidev, contents);

    g_free (contents);
    libevdev_uinput_destroy (uidev);

    return EXIT_SUCCESS;
}
