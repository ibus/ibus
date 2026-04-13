get_overridedir = '''
import os
import sys
from pathlib import Path
import sysconfig
libdir = sysconfig.get_config_var('LIBDIR')
if not libdir:
  libdir = '/usr/lib'
if '.' in sys.path:
  sys.path.remove('.')
self_dir = str(Path(__file__).resolve().parent)
print(self_dir)
if self_dir in sys.path:
  sys.path.remove(self_dir)
try:
  import gi
  overridedir = gi._overridesdir
except ImportError:
  platlibdir = sysconfig.get_path('platlib')
  overridedir = os.path.join(platlibdir, 'gi', 'overrides')
if overridedir.startswith(libdir): # Should always be True..
  overridedir = overridedir[len(libdir) + 1:]
print(overridedir)
'''

ret = run_command([python.find_python(), '-c', get_overridedir])
if ret.returncode() != 0
  error('Failed to determine pygobject override directory')
else
  pygobject_override_dir = join_paths(get_option('libdir'), ret.stdout().strip())
endif

install_data('gi/overrides/IBus.py', install_dir: pygobject_override_dir)
