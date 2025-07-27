#include <glib.h>

struct uinput_replay_device;

struct uinput_replay_device *
uinput_replay_create_device(const char *recording, GError **error);

void
uinput_replay_device_replay(struct uinput_replay_device *dev);

void
uinput_replay_device_destroy(struct uinput_replay_device *dev);
