#!/usr/bin/env python3
#
# Little helper script for Meson to get the languages
# of all installed emoji annotations, since Meson doesn't
# support globbing

import glob
import os
import sys

# Sanity check (in case someone screws up the Meson call)
if len(sys.argv) != 2:
    print('Expected 1 argument but got {}', len(sys.argv) - 1)
    sys.exit(1)

emoji_annotations_dir = sys.argv[1]

paths = glob.glob(emoji_annotations_dir + "/*.xml")
basenames = [os.path.basename(p) for p in paths]
langs = sorted([os.path.splitext(b)[0] for b in basenames])
print('\n'.join(langs), end='')
