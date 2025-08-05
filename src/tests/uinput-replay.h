#include <glib.h>

struct uinput_replay_device;

/* libevdev/libevdev-uinput.h -> libevdev/libevdev.h -> linux/input.h
 * `struct input_event` depends on linux/input.h
 */
struct evdev_uinput_event {
        guint16 index;
        guint32 usec;
        guint16 type;
        guint16 code;
        gint32  value;
};

struct uinput_replay_device *
uinput_replay_create_device   (const char                      *recording,
                               GError                         **error);

struct uinput_replay_device *
uinput_replay_create_keyboard (GError                         **error);

void
uinput_replay_device_replay   (struct uinput_replay_device     *dev);

void
uinput_replay_device_replay_event
                              (struct uinput_replay_device     *dev,
                               const struct evdev_uinput_event *event,
                               guint                           *prev_time);

void
uinput_replay_device_destroy  (struct uinput_replay_device     *dev);
