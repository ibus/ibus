#!/usr/bin/env python3
#
# Takes an file as input parameter and outputs it again as a C file with the
# contents as an escaped string.

import os
import sys


if len(sys.argv) != 2:
    print('Wrong usage, expected only 1 parameter but got %d' % len(sys.argv - 1), file=sys.stderr)
    sys.exit(1)

path = sys.argv[1]
# The variable name is the basename, with underscores
basename = os.path.basename(path)
variable = '_' + os.path.splitext(basename)[0].replace('-', '_')
print('const char *%s =' % variable)

with open(path) as file:
    for line in file.readlines():
        # Escape double quotes
        line_escaped = line.replace('"', '\\"')
        print('"%s\\n"' % line_escaped.rstrip('\n'))
print(';')
