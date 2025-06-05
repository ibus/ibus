/* -*- mode: C; c-basic-offset: 4; indent-tabs-mode: nil; -*- */

#include <config.h>

#include <gtk/gtk.h>
#include <ibus.h>

#ifdef ENABLE_NLS
#include <locale.h>
#endif

#ifndef MAX
#define MAX(a, b) (a) >= (b) ? (a) : (b)
#endif

#define MAX_KEYVAL 0x00FFFFFF
#define MAX_UNICODE 0x0010FFFF


static gboolean
test_keyval (void)
{
    guint si, bi;
    gunichar ibus_uch, gdk_uch;

    for (si = 0; si < MAX_KEYVAL; ++si) {
        ibus_uch  = ibus_keyval_to_unicode (si);
        gdk_uch = gdk_keyval_to_unicode (si);
        g_assert_cmpuint (ibus_uch, ==, gdk_uch);

        bi = si | 0x01000000;
        ibus_uch  = ibus_keyval_to_unicode (bi);
        gdk_uch = gdk_keyval_to_unicode (bi);
        g_assert_cmpuint (ibus_uch, ==, gdk_uch);

        if (!(si % 0x100000))
            g_message ("0x%08X/0x%08X is done", si, MAX_KEYVAL);
    }
    return TRUE;
}


static gboolean
test_unicode (void)
{
    gunichar i;
    guint ibus_key, gdk_key;
    for (i = 0; i < MAX_UNICODE; ++i) {
        ibus_key = ibus_unicode_to_keyval (i);
        gdk_key = gdk_unicode_to_keyval (i);
        g_assert_cmpuint (ibus_key, ==, gdk_key);
        if (!(i % 0x100000))
            g_message ("0x%08X/0x%08X is done", i, MAX_UNICODE);
    }
    return TRUE;
}


int
main (int argc, char *argv[])
{
#if !GTK_CHECK_VERSION (4, 0, 0)
    g_message ("The latest GTK 4 is needed for this test case.");
    return 77;
#else
#ifdef ENABLE_NLS
    setlocale (LC_ALL, "");
#endif
    test_keyval ();
    g_message ("----------");
    test_unicode ();
    return EXIT_SUCCESS;
#endif
}
