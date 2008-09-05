#!/usr/bin/python2.4
#
# Copyright 2008 Google Inc.  All rights reserved.


import os
from os.path import join, exists, basename
import re
import shutil
import subprocess
import sys
import tempfile
import types
import js2c


"""
Utility script for exporting the v8 source code to external subversion
"""


"""Builds the option parser for this script"""
def BuildOptionParser():
  from optparse import OptionParser
  parser = OptionParser()
  parser.add_option('-w', '--workspace', help="The v8 workspace to export")
  parser.add_option('-d', '--destination', help="Directory to export to")
  parser.add_option('-l', '--last', help="Last exported revision.")
  return parser


"""Validates the command-line options and issues an error message and
returns False if they are not as expected, otherwise return True"""
def CheckOptions(options):
  if not options.workspace:
    print "Error: no v8 workspace was specified"
    return False
  if not exists(options.workspace):
    print "Error: workspace %s could not be found" % options.workspace
    return False
  if not options.destination:
    print "Error: no destination specified"
    return False
  if not options.last:
    print "No last revision specified.  This is the revision of v8 that was last exported."
    return False
  return True


EXTRA_SOURCE_FILES = {
  join('..', 'public', 'v8.h'): None,
  join('..', 'public', 'debug.h'): None,
  join('..', 'tools', 'js2c.py'): None,
  join('..', 'LICENSE'): None,
  'macros.py': None,
  'dtoa-config.c': None,
  join('third_party', 'jscre', 'config.h'): None,
  join('third_party', 'dtoa', 'dtoa.c'): None,
  join('third_party', 'dtoa', 'COPYING'): None
}


TOPLEVEL_DIRS = [
    'src', 'public', 'tools',
    join('src', 'third_party'),
    join('src', 'third_party', 'jscre'),
    join('src', 'third_party', 'dtoa')
]


TOOLCHAINS = ['gcc', 'gcc-darwin', 'msvc']


MODES = ['release', 'debug']


CONFIGURATIONS = {
  'arch': ['arm', 'ia32'],
  'os': ['linux', 'macos', 'win32']
}


SCONSCRIPT_TEMPLATE = """\
# Copyright 2008 Google Inc.  All rights reserved.
# <<license>>

import sys
from os.path import join, dirname, abspath
root_dir = dirname(File('SConstruct').rfile().abspath)
sys.path.append(join(root_dir, 'tools'))
import js2c
Import('toolchain arch os mode use_snapshot library_type')


BUILD_OPTIONS_MAP = %(build_option_map)s


PLATFORM_INDEPENDENT_SOURCES = '''
%(platform_independent_sources)s
'''.split()


PLATFORM_DEPENDENT_SOURCES = %(platform_sources_map)s


LIBRARY_FILES = '''
%(native_source_files)s
'''.split()


JSCRE_FILES = '''
%(pcre_source_files)s
'''.split()


def Abort(message):
  print message
  sys.exit(1)


def BuildObject(env, input, **kw):
  if library_type == 'static':
    return env.StaticObject(input, **kw)
  elif library_type == 'shared':
    return env.SharedObject(input, **kw)
  else:
    return env.Object(input, **kw)


def ConfigureBuild():
  env = Environment()
  options = BUILD_OPTIONS_MAP[toolchain][mode]['default']
  env.Replace(**options)
  env['BUILDERS']['JS2C'] = Builder(action=js2c.JS2C)
  env['BUILDERS']['Snapshot'] = Builder(action='$SOURCE $TARGET --logfile $LOGFILE')

  # Build the standard platform-independent source files.
  source_files = PLATFORM_INDEPENDENT_SOURCES
%(add_platform_sources)s\
  full_source_files = [s for s in source_files]

  # Combine the javascript library files into a single C++ file and
  # compile it.
  library_files = [s for s in LIBRARY_FILES]
  library_files.append('macros.py')
  libraries_src, libraries_empty_src = env.JS2C(['libraries.cc', 'libraries-empty.cc'], library_files)
  libraries_obj = BuildObject(env, libraries_src, CPPPATH=['.'])

  # Build JSCRE.
  jscre_env = env.Copy()
  jscre_options = BUILD_OPTIONS_MAP[toolchain][mode]['jscre']
  jscre_env.Replace(**jscre_options)
  jscre_files = [join('third_party', 'jscre', s) for s in JSCRE_FILES]
  jscre_obj = BuildObject(jscre_env, jscre_files)

  # Build dtoa.
  dtoa_env = env.Copy()
  dtoa_options = BUILD_OPTIONS_MAP[toolchain][mode]['dtoa']
  dtoa_env.Replace(**dtoa_options)
  dtoa_files = ['dtoa-config.c']
  dtoa_obj = BuildObject(dtoa_env, dtoa_files)

  full_source_objs = BuildObject(env, full_source_files)
  non_snapshot_files = [jscre_obj, dtoa_obj, full_source_objs]

  # Create snapshot if necessary.
  empty_snapshot_obj = BuildObject(env, 'snapshot-empty.cc')
  if use_snapshot:
    mksnapshot_src = 'mksnapshot.cc'
    mksnapshot = env.Program('mksnapshot', [mksnapshot_src, libraries_obj, non_snapshot_files, empty_snapshot_obj], PDB='mksnapshot.exe.pdb')
    snapshot_cc = env.Snapshot('snapshot.cc', mksnapshot, LOGFILE=File('snapshot.log').abspath)
    snapshot_obj = BuildObject(env, snapshot_cc, CPPPATH=['.'])
    libraries_obj = BuildObject(env, libraries_empty_src, CPPPATH=['.'])
  else:
    snapshot_obj = empty_snapshot_obj

  all_files = [non_snapshot_files, libraries_obj, snapshot_obj]
  if library_type == 'static':
    env.StaticLibrary('v8', all_files)
  elif library_type == 'shared':
    # There seems to be a glitch in the way scons decides where to put
    # .pdb files when compiling using msvc so we specify it manually.
    # This should not affect any other platforms.
    env.SharedLibrary('v8', all_files, PDB='v8.dll.pdb')
  else:
    env.Library('v8', all_files)


ConfigureBuild()
"""


SCONSTRUCT_TEMPLATE = """\
# Copyright 2008 Google Inc.  All rights reserved.
# <<license>>

import platform
import re
import sys
from os.path import join, dirname, abspath
root_dir = dirname(File('SConstruct').rfile().abspath)
sys.path.append(join(root_dir, 'tools'))
import js2c


def Abort(message):
  print message
  sys.exit(1)


def GuessOS():
  id = platform.system()
  if id == 'Linux':
    return 'linux'
  elif id == 'Darwin':
    return 'macos'
  elif id == 'Windows':
    return 'win32'
  else:
    return '<none>'


def GuessProcessor():
  id = platform.machine()
  if id.startswith('arm'):
    return 'arm'
  elif (not id) or (not re.match('(x|i[3-6])86', id) is None):
    return 'ia32'
  else:
    return '<none>'


def GuessToolchain(os):
  tools = Environment()['TOOLS']
  if 'gcc' in tools:
    if os == 'macos' and 'Kernel Version 8' in platform.version():
      return 'gcc-darwin'
    else:
      return 'gcc'
  elif 'msvc' in tools:
    return 'msvc'
  else:
    return '<none>'


def GetOptions():
  result = Options()
  os_guess = GuessOS()
  toolchain_guess = GuessToolchain(os_guess)
  processor_guess = GuessProcessor()
  result.Add('mode', 'debug or release', 'release')
  result.Add('toolchain', 'the toolchain to use (gcc, gcc-darwin or msvc)', toolchain_guess)
  result.Add('os', 'the os to build for (linux, macos or win32)', os_guess)
  result.Add('processor', 'the processor to build for (arm or ia32)', processor_guess)
  result.Add('snapshot', 'build using snapshots for faster start-up (on, off)', 'off')
  result.Add('library', 'which type of library to produce (static, shared, default)', 'default')
  return result


def VerifyOptions(env):
  if not env['mode'] in ['debug', 'release']:
    Abort("Unknown build mode '%%s'." %% env['mode'])
  if not env['toolchain'] in ['gcc', 'gcc-darwin', 'msvc']:
    Abort("Unknown toolchain '%%s'." %% env['toolchain'])
  if not env['os'] in ['linux', 'macos', 'win32']:
    Abort("Unknown os '%%s'." %% env['os'])
  if not env['processor'] in ['arm', 'ia32']:
    Abort("Unknown processor '%%s'." %% env['processor'])
  if not env['snapshot'] in ['on', 'off']:
    Abort("Illegal value for option snapshot: '%%s'." %% env['snapshot'])
  if not env['library'] in ['static', 'shared', 'default']:
    Abort("Illegal value for option library: '%%s'." %% env['library'])


def Start():
  opts = GetOptions()
  env = Environment(options=opts)
  Help(opts.GenerateHelpText(env))
  VerifyOptions(env)

  os = env['os']
  arch = env['processor']
  toolchain = env['toolchain']
  mode = env['mode']
  use_snapshot = (env['snapshot'] == 'on')
  library_type = env['library']

  env.SConscript(
    join('src', 'SConscript'),
    build_dir=mode,
    exports='toolchain arch os mode use_snapshot library_type',
    duplicate=False
  )


Start()
"""


ADD_SOURCES_TEMPLATE = """\
  source_files += PLATFORM_DEPENDENT_SOURCES["%(name)s:%%s" %% %(name)s]
"""


def GetSourceList(lines):
  result = []
  for line in lines:
    line = line.strip()
    if len(line) > 0:
      parts = line.split()
      result.append((parts[-1], len(parts) > 1))
  return result


LICENSE_PATTERN = r"\n(\s*[^\s]*\s*)<<license>>[^\n]*\n"


AUTO_REPLACE = [
  (r'\s+//\s+NOLINT', '')
]


"""Utility class that keeps all the state associated with the export
operation"""
class Exporter(object):

  def __init__(self, options):
    self.workspace_path = options.workspace
    self.destination_path = options.destination

  def EnsurePath(self, name):
    if not exists(name):
      os.mkdir(name)

  """Attempts to create the destination folder if it doesn't already
  exist."""
  def EnsureDestination(self):
    try:
      self.EnsurePath(self.destination_path)
      for p in TOPLEVEL_DIRS:
        self.EnsurePath(join(self.destination_path, p))
    except OSError, e:
      print "Error creating destination: %s" % e
      return False
    return True

  def AppendConfig(self, config, name, values):
    for key, value in config.items(name):
      if key.startswith('internal'):
        continue
      key = key.upper()
      prefix = None
      if value.startswith('+'):
        value = value[1:].strip()
        if key in values:
          prefix = values[key]
        if type(prefix) is types.StringType: prefix += ' '
      value = js2c.ParseValue(value)
      if not prefix is None: value = prefix + value
      values[key] = value

  def ReadBuildOptions(self):
    option_map = { }
    for toolchain in TOOLCHAINS:
      config_file = join(self.workspace_path, 'SConstruct-%s.ini' % toolchain)
      config = js2c.LoadConfigFrom(config_file)
      mode_map = { }
      for mode in MODES:
        mode_map[mode] = { }
        standard_values = { }
        self.AppendConfig(config, 'COMMON', standard_values)
        self.AppendConfig(config, mode.upper(), standard_values)
        mode_map[mode]['default'] = standard_values

        jscre_values = { }
        self.AppendConfig(config, 'COMMON', jscre_values)
        self.AppendConfig(config, mode.upper(), jscre_values)
        self.AppendConfig(config, mode.upper() + ":pcre", jscre_values)
        mode_map[mode]['jscre'] = jscre_values

        dtoa_values = { }
        self.AppendConfig(config, 'COMMON', dtoa_values)
        self.AppendConfig(config, mode.upper(), dtoa_values)
        self.AppendConfig(config, mode.upper() + ":dtoa", dtoa_values)
        mode_map[mode]['dtoa'] = dtoa_values

      option_map[toolchain] = mode_map
    self.build_option_map = option_map
    return True

  def BuildSourceLists(self):
    self.copy_map = { }

    # Process js library (natives files)
    js_list = join(self.workspace_path, 'src', 'SOURCE-js')
    try:
      f = open(js_list)
      self.js_files = [js for js in [raw.strip() for raw in f] if js.endswith('.js')]
      f.close()
    except IOError, e:
      print "Error reading %s: %s" % (js_list, e)
      return False
    for js_file in self.js_files:
      self.copy_map[js_file] = js_file

    # Process c++ source files
    source_list = join(self.workspace_path, 'src', 'SOURCE')
    try:
      f = open(source_list)
      lines = f.read().split("\n")
      f.close()
    except IOError, e:
      print "Error reading %s: %s" % (source_list, e)
      return False
    self.independent_sources = [ ]
    self.headers = [ ]
    self.dependent_sources = { }
    for (line, is_internal) in GetSourceList(lines):
      was_dependent = False
      if line.endswith('.h'):
        self.headers.append(line)
        self.copy_map[line] = line
        continue
      if is_internal:
        self.copy_map[line] = line
        continue
      for name, values in CONFIGURATIONS.items():
        placeholder = "<%s>" % name
        if placeholder in line:
          for value in values:
            expanded_name = line.replace(placeholder, value)
            key = '%s:%s' % (name, value)
            if not key in self.dependent_sources:
              self.dependent_sources[key] = [ ]
            self.dependent_sources[key].append(expanded_name)
            self.copy_map[expanded_name] = expanded_name
          was_dependent = True
      if not was_dependent:
        self.independent_sources.append(line)
        self.copy_map[line] = line
    self.independent_sources.sort()

    # Process the 'extra' source files
    for f, t in EXTRA_SOURCE_FILES.items():
      if not t: t = f
      self.copy_map[f] = t

    # PCRE
    pcre_list = join(self.workspace_path, 'src', 'third_party', 'jscre', 'SOURCE')
    try:
      f = open(pcre_list)
      all_pcre = [raw.strip() for raw in f]
      f.close()
    except IOError, e:
      print "Error reading %s: %s" % (pcre_list, e)
      return False
    self.pcre_files = [ s for (s, i) in GetSourceList(all_pcre) if s.endswith('.cpp') and not i ]
    for raw_pcre_file in all_pcre:
      pcre_file = raw_pcre_file.split()[-1]
      source_name = join('third_party', 'jscre', pcre_file)
      target_name = join('third_party', 'jscre', pcre_file)
      self.copy_map[source_name] = target_name
    self.add_platform_sources = []
    for name in sorted(CONFIGURATIONS.keys()):
      self.add_platform_sources.append(ADD_SOURCES_TEMPLATE % { 'name': name })
    return True

  def CheckScrubs(self, name, s):
    for entry in self.scrub_list:
      match = re.search(entry, s, flags=re.IGNORECASE)
      if match:
        whole_line_pattern = '\n([^\n]*' + entry + '[^\n]*)\n'
        whole_lines = re.findall(whole_line_pattern, s, flags=re.IGNORECASE)
        for whole_line in whole_lines:
          print "Suspect line in %s: %s" % (basename(name), whole_line)

  def AutoScrub(self, s):
    for (pat, rep) in AUTO_REPLACE:
      s = re.sub(pat, rep, s)
    return s

  def ProcessExportedFile(self, name, s):
    result = self.InsertLicense(s)
    result = self.AutoScrub(result)
    self.CheckScrubs(name, result)
    return result

  def InsertLicense(self, s):
    match = re.search(LICENSE_PATTERN, s)
    if not match: return s
    comment_marker = "\n" + match.group(1)
    lines = [ (comment_marker + l).rstrip() for l in self.license_header ]
    header = "".join(lines) + "\n"
    return re.sub(LICENSE_PATTERN, header, s)

  def CopySourceFiles(self):
    # Read license header and strip trailing empty lines
    self.license_header = open(join(self.workspace_path, 'LICENSE-HEADER')).read().split("\n")
    self.scrub_list = js2c.ReadLines(join(self.workspace_path, 'tools', 'scrub.list'))
    self.scrub_list = [ e for e in self.scrub_list ]
    while len(self.license_header[-1].strip()) == 0:
      del self.license_header[-1]
    target_root = join(self.destination_path, 'src')
    for (f, t) in self.copy_map.items():
      source = join(self.workspace_path, 'src', f)
      target = join(target_root, t)
      try:
        source_file = open(source)
        source_str = source_file.read()
        source_str = self.ProcessExportedFile(source, source_str)
        source_file.close()
        target_file = open(target, "w")
        target_file.write(source_str)
        target_file.close()
      except IOError, e:
        print "Error copying %s to %s: %s" % (source, target, e)
        return False
    return True

  def MapToString(self, obj, indent=1):
    if type(obj) is dict:
      result = '{'
      def Indent(str, value):
        for i in xrange(0, value):
          str += '  '
        return str
      first = True
      keys = sorted(obj.keys())
      for key in keys:
        value = obj[key]
        if first: first = False
        else: result += ','
        result += '\n'
        result = Indent(result, indent)
        result += self.MapToString(key)
        result += ": "
        result += self.MapToString(value, indent + 1)
      result += '\n'
      result = Indent(result, indent - 1)
      return result + '}'
    elif type(obj) is str:
      return "'%s'" % obj
    else:
      return str(obj)

  def WriteSConstruct(self, out):
    out.write(self.ProcessExportedFile("generated SConstruct", SCONSTRUCT_TEMPLATE % {
    }))

  def WriteSConscript(self, out):
    independent_sources = "\n".join(self.independent_sources)
    native_sources = "\n".join(self.js_files)
    out.write(self.ProcessExportedFile("generated SConscript", SCONSCRIPT_TEMPLATE % {
      'platform_independent_sources': independent_sources,
      'native_source_files': native_sources,
      'build_option_map': self.MapToString(self.build_option_map),
      'platform_sources_map': self.MapToString(self.dependent_sources),
      'add_platform_sources': "".join(self.add_platform_sources),
      'pcre_source_files': "\n".join(self.pcre_files)
    }))

  """Creates and writes the SConstruct file"""
  def GenerateSConsFiles(self):
    # Write SConstruct
    sconstruct = join(self.destination_path, 'SConstruct')
    try:
      f = open(sconstruct, "w")
      self.WriteSConstruct(f)
      f.close()
    except IOError, e:
      print "Error writing %s: %s" % (sconstruct, e)
      return False

    # Write src/SConscript
    sconscript = join(self.destination_path, 'src', 'SConscript')
    try:
      f = open(sconscript, "w")
      self.WriteSConscript(f)
      f.close()
    except IOError, e:
      print "Error writing %s: %s" % (sconscript, e)
      return False

    return True

  def Run(self):
    if not self.EnsureDestination(): return False
    if not self.BuildSourceLists(): return False
    if not self.ReadBuildOptions(): return False
    if not self.CopySourceFiles(): return False
    if not self.GenerateSConsFiles(): return False
    return True


def WriteChangeLog(revision, workspace, destination):
  target_name = join(destination, "ChangeLog.raw")
  target = open(target_name, "w")
  command = "svn log -r %i:HEAD %s" % (revision, workspace)
  process = subprocess.Popen(command.split(), stdout=target)
  process.wait()
  return True


def Main():
  parser = BuildOptionParser()
  (options, args) = parser.parse_args()
  if not CheckOptions(options):
    parser.print_help()
    return 1
  exporter = Exporter(options)
  print "Exporting source code"
  if not exporter.Run():
    return 1
  print "Writing tentative ChangeLog section"
  if not WriteChangeLog(int(options.last), options.workspace, options.destination):
    return 1
  return 0


if __name__ == "__main__":
  sys.exit(Main())
