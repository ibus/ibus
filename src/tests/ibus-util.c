/* -*- mode: C; c-basic-offset: 4; indent-tabs-mode: nil; -*- */

#include <locale.h>

#include "ibus.h"

int main (int argc, char **argv)
{
    gchar *name;
    setlocale(LC_ALL, "C");

    g_assert_cmpstr (name = ibus_get_language_name ("eng"), ==, "English");
    g_free (name);

    return 0;
}
