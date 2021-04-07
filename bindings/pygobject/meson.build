get_overridedir = '''
import os
import sysconfig
libdir = sysconfig.get_config_var('SCRIPTDIR')
if not libdir:
  libdir = '/usr/lib'
try:
  import gi
  overridedir = gi._overridesdir
except ImportError:
  purelibdir = sysconfig.get_path('purelib')
  overridedir = os.path.join(purelibdir, 'gi', 'overrides')
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
