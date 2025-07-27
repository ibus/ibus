/* -*- mode: C; c-basic-offset: 4; indent-tabs-mode: nil; -*- */
/* vim:set et sts=4: */
/*
 * Copyright (c) 2013 Marcin Slusarz <marcin.slusarz@gmail.com>
 * Copyright (c) 2025 Peter Hutterer <peter.hutterer@who-t.net>
 # Copyright (c) 2025 Takao Fujiwara <takao.fujiwara1@gmail.com>
 * Copyright (c) 2025 Red Hat, Inc.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 */

#include <glib.h>

#include <errno.h>
#include <grp.h>
#include <linux/input.h>
#include <linux/input-event-codes.h>
#include <libevdev/libevdev-uinput.h>
#include <libudev.h>

#include <fcntl.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <unistd.h>

#include "uinput-replay.h"


#define msleep(t) usleep((t) * 1000)

struct uinput_replay_device
{
    struct libevdev_uinput *uidev;
    char                   *contents;
};


static gchar *
get_case_contents (const gchar *case_path,
                   GError     **error)
{
    gchar *contents = NULL;
    gsize length = 0;

    g_return_val_if_fail (case_path, NULL);
    if (!g_file_get_contents (case_path, &contents, &length, error)) {
        if (error) {
            gchar *error_message = g_strdup ((*error)->message);
            g_clear_pointer (error, g_error_free);
            g_set_error (error, G_FILE_ERROR, G_FILE_ERROR_FAILED,
                         "Failed to open %s: %s", case_path, error_message);
            g_free (error_message);
        }
    } else if (length == 0) {
        if (error) {
            g_set_error (error, G_FILE_ERROR, G_FILE_ERROR_FAILED,
                         "No contents file %s", case_path);
        }
    }
    return contents;
}


static inline int
streq (const char *str1, const char *str2)
{
    /* one NULL, one not NULL is always false */
    if (str1 && str2)
        return strcmp (str1, str2) == 0;
    return str1 == str2;
}


/* Refer libinput/test/litest.c:udev_setup_monitor() */
static struct udev_monitor *
setup_udev_monitor (void)
{
    struct udev *udev = udev_new ();
    struct udev_monitor *udev_monitor = NULL;

    g_assert (udev);
    udev_monitor = udev_monitor_new_from_netlink (udev, "udev");
    g_assert (udev_monitor);
    udev_unref (udev);
    udev_monitor_filter_add_match_subsystem_devtype (udev_monitor,
                                                     "input",
                                                     NULL);
    errno = 0;
    if (fcntl (udev_monitor_get_fd (udev_monitor), F_SETFL, 0) < 0) {
        g_warning ("Failed to F_SETFL %s", g_strerror (errno));
        udev_monitor_unref (udev_monitor);
        return NULL;
    }
    if (udev_monitor_enable_receiving (udev_monitor)) {
        g_warning ("Failed to enable udev_monitor");
        udev_monitor_unref (udev_monitor);
        return NULL;
    }
    return udev_monitor;
}


/* Refer libinput/test/litest.c:udev_wait_for_device_event() */
static struct udev_device *
udev_wait_for_device_event (struct udev_monitor *udev_monitor,
                            const char          *udev_event,
                            const char          *syspath)
{
    while (1) {
        struct udev_device *udev_device = NULL;
        const char *udev_syspath = NULL;
        const char *udev_action;

        udev_device = udev_monitor_receive_device (udev_monitor);
        if (!udev_device) {
            if (errno == EAGAIN)
                continue;

            g_warning ("Failed to receive udev device from monitor: %s",
                       g_strerror (errno));
        }
        udev_action = udev_device_get_action (udev_device);
        if (!udev_action || !streq (udev_action, udev_event)) {
            continue;
        }

        udev_syspath = udev_device_get_syspath (udev_device);
        if (g_str_has_prefix (udev_syspath, syspath))
            return udev_device;
    }
}


static struct libevdev_uinput *
ibus_uidev_new (GError **error)
{
    struct udev_monitor *udev_monitor = setup_udev_monitor ();
    struct libevdev *dev = libevdev_new ();
    unsigned int code;
    struct libevdev_uinput *uidev = NULL;
    int retval;
    const char *syspath;
    const char *devnode;
    struct udev_device *udev_device;
    gchar *path;

    g_return_val_if_fail (udev_monitor, NULL);
    g_return_val_if_fail (dev, NULL);
    libevdev_set_name (dev, "ibusdev");
    /* serial makes it an internal keyboard */
    libevdev_set_id_bustype (dev, BUS_I8042);

    for (code = KEY_ESC; code < BTN_MISC; code++)
        libevdev_enable_event_code (dev, EV_KEY, code, NULL);

    /* chmod a+rw /dev/uinput */
    retval = libevdev_uinput_create_from_device (
            dev,
            LIBEVDEV_UINPUT_OPEN_MANAGED,
            &uidev);
    libevdev_free (dev);

    if (retval) {
        if (error) {
            g_set_error (error, G_FILE_ERROR, G_FILE_ERROR_FAILED,
                         "Failed to create uinput: %s\n", g_strerror (-retval));
        }
        return NULL;
    }

    syspath = libevdev_uinput_get_syspath (uidev);
    g_assert (syspath != NULL);
    devnode = libevdev_uinput_get_devnode (uidev);
    g_assert (devnode != NULL);

    /* You need to wait here until the device actually exists, either via a
     * sleep or checking udev until the device shows up. Use
     * libevdev_uinput_get_syspath() for the latter
     */
    path = g_strdup_printf ("%s/event", syspath);
    udev_device = udev_wait_for_device_event (udev_monitor, "add", path);
    g_free (path);
    g_assert (udev_device);
    udev_device_unref (udev_device);
    udev_monitor_unref (udev_monitor);
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


struct uinput_replay_device *
uinput_replay_create_device (const char *recording,
                             GError    **error)
{
    gchar *contents;
    struct libevdev_uinput *uidev;
    struct uinput_replay_device *dev;

    g_return_val_if_fail (recording != NULL, NULL);

    g_return_val_if_fail ((contents = get_case_contents (recording, error)),
                          NULL);
    if (!(uidev = ibus_uidev_new (error))) {
        g_free (contents);
        return NULL;
    }

    dev = g_new0 (struct uinput_replay_device, 1);
    dev->uidev = uidev;
    dev->contents = contents;

    return dev;
}


void
uinput_replay_device_destroy (struct uinput_replay_device *dev)
{
    g_free (dev->contents);
    libevdev_uinput_destroy (dev->uidev);
    g_free (dev->contents);
}


void
uinput_replay_device_replay (struct uinput_replay_device *dev)
{
    ibus_uidev_replay_with_yaml_data (dev->uidev, dev->contents);
}


#ifdef UINPUT_REPLAY_MAIN
int
main (int argc, char *argv[]) {
    GError *error = NULL;
    gchar *contents;
    struct libevdev_uinput *uidev;

    if (argc == 1) {
        gchar *prgname = g_path_get_basename (argv[0]);
        g_warning ("Usage: %s libinput-test.yml", prgname);
        g_free (prgname);
        return EXIT_FAILURE;
    }

    if (!(contents = get_case_contents (argv[1], &error))) {
        g_warning ("%s", error->message);
        g_error_free (error);
        return EXIT_FAILURE;
    }
    if (!(uidev = ibus_uidev_new (&error))) {
        g_warning ("%s", error->message);
        g_error_free (error);
        return EXIT_FAILURE;
    }

    ibus_uidev_replay_with_yaml_data (uidev, contents);

    g_free (contents);
    libevdev_uinput_destroy (uidev);

    return EXIT_SUCCESS;
}
#endif
