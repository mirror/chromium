#!/usr/bin/python
#
# Copyright 2008 Google Inc.  All Rights Reserved.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

"""A wrapper script to manage a set of client modules in different SCM.

This script is intended to be used to help basic management of client
program sources residing in one or more Subversion modules, along with
other modules it depends on, also in Subversion, but possibly on
multiple respositories, making a wrapper system apparently necessary.

Files
  .gclient      : Current client configuration, written by 'config' command.
                  Format is a Python script defining 'solutions', a list whose
                  entries each are maps binding the strings "name" and "url"
                  to strings specifying the name and location of the client
                  module, as well as "custom_deps" to a map similar to the DEPS
                  file below.
  .gclient_entries : A cache constructed by 'update' command.  Format is a
                  Python script defining 'entries', a list of the names
                  of all modules in the client
  <module>/DEPS : Python script defining var 'deps' as a map from each requisite
                  submodule name to a URL where it can be found (via one SCM)
"""

__author__ = "darinf@gmail.com (Darin Fisher)"
__version__ = "0.1"

import errno
import optparse
import os
import stat
import subprocess
import sys
import time
import urlparse
import xml.dom.minidom

def getText(nodelist):
  """
  Return the concatenated text for the children of a list of DOM nodes.
  """
  rc = []
  for node in nodelist:
    if node.nodeType == node.TEXT_NODE:
      rc.append(node.data)
    else:
      rc.append(getText(node.childNodes))
  return ''.join(rc)


SVN_COMMAND = "svn"


# default help text
DEFAULT_USAGE_TEXT = (
    """usage: %prog <subcommand> [options] [--] [svn options/args...]
a wrapper for managing a set of client modules in svn.

subcommands:
   config
   diff
   revert
   status
   sync
   update

Options and extra arguments can be passed to invoked svn commands by
appending them to the command line.  Note that if the first such
appended option starts with a dash (-) then the options must be
preceded by -- to distinguish them from gclient options.

For additional help on a subcommand or examples of usage, try
   %prog help <subcommand>
   %prog help files
""")

GENERIC_UPDATE_USAGE_TEXT = (
    """Perform a checkout/update of the modules specified by the gclient
configuration; see 'help config'.  Unless --revision is specified,
then the latest revision of the root solutions is checked out, with
dependent submodule versions updated according to DEPS files.
If --revision is specified, then the given revision is used in place
of the latest, either for a single solution or for all solutions.
Unless the --force option is provided, solutions and modules whose
local revision matches the one to update (i.e., they have not changed
in the repository) are *not* modified.
This a synonym for 'gclient %(alias)s'

usage: gclient %(cmd)s [options] [--] [svn update options/args]

Valid options:
  --force             : force update even for unchanged modules
  --revision REV      : update/checkout all solutions with specified revision
  --revision SOLUTION@REV : update given solution to specified revision
  --deps PLATFORM(S)  : sync deps for the given platform(s), or 'all'
  --verbose           : output additional diagnostics

Examples:
  gclient %(cmd)s
      update files from SVN according to current configuration,
      *for modules which have changed since last update or sync*
  gclient %(cmd)s --force
      update files from SVN according to current configuration, for
      all modules (useful for recovering files deleted from local copy)
""")

COMMAND_USAGE_TEXT = {
    "config": """Create a .gclient file in the current directory; this
specifies the configuration for further commands.  After update/sync,
top-level DEPS files in each module are read to determine dependent
modules to operate on as well.  If optional [url] parameter is
provided, then configuration is read from a specified Subversion server
URL.  Otherwise, a --spec option must be provided.

usage: config [option | url]

Valid options:
  --spec=GCLIENT_SPEC   : contents of .gclient are read from string parameter.
                          *Note that due to Cygwin/Python brokenness, it
                          probably can't contain any newlines.*

Examples:
  gclient config https://gclient.googlecode.com/svn/trunk/gclient
      configure a new client to check out gclient.py tool sources
  gclient config --spec='solutions=[{"name":"gclient","""
    '"url":"https://gclient.googlecode.com/svn/trunk/gclient",'
    '"custom_deps":{}}]',
    "diff": """Display the differences between two revisions of modules.
(Does 'svn diff' for each checked out module and dependences.)
Additional args and options to 'svn diff' can be passed after
gclient options.

usage: diff [options] [--] [svn args/options]

Valid options:
  --verbose            : output additional diagnostics

Examples:
  gclient diff
      simple 'svn diff' for configured client and dependences
  gclient diff -- -x -b
      use 'svn diff -x -b' to suppress whitespace-only differences
  gclient diff -- -r HEAD -x -b
      diff versus the latest version of each module
""",
    "revert":
    """Revert every file in every managed directory in the client view.

usage: revert
""",
    "status":
    """Show the status of client and dependent modules, using 'svn diff'
for each module.  Additional options and args may be passed to 'svn diff'.

usage: status [options] [--] [svn diff args/options]

Valid options:
  --verbose           : output additional diagnostics
""",
    "sync": GENERIC_UPDATE_USAGE_TEXT % {"cmd": "sync", "alias": "update"},
    "update": GENERIC_UPDATE_USAGE_TEXT % {"cmd": "update", "alias": "sync"},
    "help": """Describe the usage of this program or its subcommands.

usage: help [options] [subcommand]

Valid options:
  --verbose           : output additional diagnostics
""",
}

# parameterized by (solution_name, solution_url)
DEFAULT_CLIENT_FILE_TEXT = (
    """
# An element of this array (a \"solution\") describes a repository directory
# that will be checked out into your working copy.  Each solution may
# optionally define additional dependencies (via its DEPS file) to be
# checked out alongside the solution's directory.  A solution may also
# specify custom dependencies (via the \"custom_deps\" property) that
# override or augment the dependencies specified by the DEPS file.
solutions = [
  { \"name\"        : \"%s\",
    \"url\"         : \"%s\",
    \"custom_deps\" : {
      # To use the trunk of a component instead of what's in DEPS:
      #\"component\": \"https://svnserver/component/trunk/\",
      # To exclude a component from your working copy:
      #\"data/really_large_component\": None,
    }
  }
]
""")


## Generic utils


class Error(Exception):
  """gclient exception class."""

  def __init__(self, message):
    Exception.__init__(self)
    self.message = message
  def __str__(self):
    return 'Error: %s' % self.message


class PrintableObject(object):
  def __str__(self):
    output = ''
    for i in dir(self):
      if i.startswith('__'):
        continue
      output += '%s = %s\n' % (i, str(getattr(self, i, '')))
    return output


def FileRead(filename):
  content = None
  f = open(filename, "rU")
  try:
    content = f.read()
  finally:
    f.close()
  return content


def FileWrite(filename, content):
  f = open(filename, "w")
  try:
    f.write(content)
  finally:
    f.close()


def RemoveDirectory(*path):
  """Recursively removes a directory, even if it's marked read-only.

  Remove the directory located at *path, if it exists.

  shutil.rmtree() doesn't work on Windows if any of the files or directories
  are read-only, which svn repositories and some .svn files are.  We need to
  be able to force the files to be writable (i.e., deletable) as we traverse
  the tree.

  Even with all this, Windows still sometimes fails to delete a file, citing
  a permission error (maybe something to do with antivirus scans or disk
  indexing).  The best suggestion any of the user forums had was to wait a
  bit and try again, so we do that too.  It's hand-waving, but sometimes it
  works. :/

  On POSIX systems, things are a little bit simpler.  The modes of the files
  to be deleted doesn't matter, only the modes of the directories containing
  them are significant.  As the directory tree is traversed, each directory
  has its mode set appropriately before descending into it.  This should
  result in the entire tree being removed, with the possible exception of
  *path itself, because nothing attempts to change the mode of its parent.
  Doing so would be hazardous, as it's not a directory slated for removal.
  In the ordinary case, this is not a problem: for our purposes, the user
  will never lack write permission on *path's parent.
  """
  file_path = os.path.join(*path)
  if not os.path.exists(file_path):
    return

  if os.path.islink(file_path) or not os.path.isdir(file_path):
    raise Error("RemoveDirectory asked to remove non-directory %s" % file_path)

  has_win32api = False
  if sys.platform == 'win32':
    has_win32api = True
    # Some people don't have the APIs installed. In that case we'll do without.
    try:
      win32api = __import__('win32api')
      win32con = __import__('win32con')
    except ImportError:
      has_win32api = False
  else:
    # On POSIX systems, we need the x-bit set on the directory to access it,
    # the r-bit to see its contents, and the w-bit to remove files from it.
    # The actual modes of the files within the directory is irrelevant.
    os.chmod(file_path, stat.S_IRUSR | stat.S_IWUSR | stat.S_IXUSR)
  for fn in os.listdir(file_path):
    fullpath = os.path.join(file_path, fn)

    # If fullpath is a symbolic link that points to a directory, isdir will
    # be True, but we don't want to descend into that as a directory, we just
    # want to remove the link.  Check islink and treat links as ordinary files
    # would be treated regardless of what they reference.
    if os.path.islink(fullpath) or not os.path.isdir(fullpath):
      if sys.platform == 'win32':
        os.chmod(fullpath, stat.S_IWRITE)
        if has_win32api:
          win32api.SetFileAttributes(fullpath, win32con.FILE_ATTRIBUTE_NORMAL)
      try:
        os.remove(fullpath)
      except OSError, e:
        if e.errno != errno.EACCES or sys.platform != 'win32':
          raise
        print 'Failed to delete %s: trying again' % fullpath
        time.sleep(0.1)
        os.remove(fullpath)
    else:
      RemoveDirectory(fullpath)

  if sys.platform == 'win32':
    os.chmod(file_path, stat.S_IWRITE)
    if has_win32api:
      win32api.SetFileAttributes(file_path, win32con.FILE_ATTRIBUTE_NORMAL)
  try:
    os.rmdir(file_path)
  except OSError, e:
    if e.errno != errno.EACCES or sys.platform != 'win32':
      raise
    print 'Failed to remove %s: trying again' % file_path
    time.sleep(0.1)
    os.rmdir(file_path)


# -----------------------------------------------------------------------------
# SVN utils:


def RunSVN(options, args, in_directory):
  """Runs svn, sending output to stdout.

  Args:
    args: A sequence of command line parameters to be passed to svn.
    in_directory: The directory where svn is to be run.

  Raises:
    Error: An error occurred while running the svn command.
  """
  c = [SVN_COMMAND]
  c.extend(args)

  print >> options.stdout, ("\n________ running \'%s\' in \'%s\'"
         % (' '.join(c), os.path.realpath(in_directory)))

  # *Sigh*:  Windows needs shell=True, or else it won't search %PATH% for
  # the svn.exe executable, but shell=True makes subprocess on Linux fail
  # when it's called with a list because it only tries to execute the
  # first string ("svn").
  rv = subprocess.call(c, cwd=in_directory, shell=(sys.platform == 'win32'))

  if rv:
    raise Error("failed to run command: %s" % " ".join(c))
  return rv


def CaptureSVN(options, args, in_directory):
  """Runs svn, capturing output sent to stdout as a string.

  Args:
    args: A sequence of command line parameters to be passed to svn.
    in_directory: The directory where svn is to be run.

  Returns:
    The output sent to stdout as a string.
  """
  c = [SVN_COMMAND]
  c.extend(args)

  # *Sigh*:  Windows needs shell=True, or else it won't search %PATH% for
  # the svn.exe executable, but shell=True makes subprocess on Linux fail
  # when it's called with a list because it only tries to execute the
  # first string ("svn").
  return subprocess.Popen(c, cwd=in_directory, shell=(sys.platform == 'win32'),
                          stdout=subprocess.PIPE).communicate()[0]


def CaptureSVNInfo(options, relpath, in_directory):
  """Runs 'svn info' on an existing path.

  Args:
    relpath: The directory where the working copy resides relative to
      the directory given by in_directory.
    in_directory: The directory where svn is to be run.

  Returns:
    An object with fields corresponding to the output of 'svn info'
  """
  info = CaptureSVN(options, ["info", "--xml", relpath], in_directory)
  dom = xml.dom.minidom.parseString(info)

  # str() the getText() results because they may be returned as
  # Unicode, which interferes with the higher layers matching up
  # things in the deps dictionary.
  result = PrintableObject()
  result.root = str(getText(dom.getElementsByTagName('root')))
  result.url = str(getText(dom.getElementsByTagName('url')))
  result.uuid = str(getText(dom.getElementsByTagName('uuid')))
  result.revision = int(dom.getElementsByTagName('entry')[0].getAttribute(
                            'revision'))
  return result


class FileStatus:
  def __init__(self, path, text_status, props, locked, history, switched,
               repo_locked, out_of_date):
    self.path = path
    self.text_status = text_status
    self.props = props
    self.locked = locked
    self.history = history
    self.switched = switched
    self.repo_locked = repo_locked
    self.out_of_date = out_of_date

  def __str__(self):
    return (self.text_status + self.props + self.locked + self.history +
            self.switched + self.repo_locked + self.out_of_date +
            self.path)


def CaptureSVNStatus(options, path):
  """Runs 'svn status' on an existing path.

  Args:
    path: The directory to run svn status.

  Returns:
    An array of FileStatus corresponding to the output of 'svn status'
  """
  info = CaptureSVN(options, ["status"], path)
  result = []
  if not info:
    return result
  for line in info.splitlines():
    if line:
      new_item = FileStatus(line[7:], line[0:1], line[1:2], line[2:3],
                            line[3:4], line[4:5], line[5:6], line[6:7])
      result.append(new_item)
  return result


### SCM abstraction layer


class SCMWrapper(object):
  """Add necessary glue between all the supported SCM.
  
  This is the abstraction layer to bind to different SCM. Since currently only
  subversion is supported, a lot of subersionism remains. This can be sorted out
  once another SCM is supported."""
  def __init__(self, url=None, root_dir=None, relpath=None,
               scm_name='svn'):
    # TODO(maruel): Deduce the SCM from the url.
    self.scm_name = scm_name
    self.url = url
    self._root_dir = root_dir
    self.relpath = relpath

  def FullUrlForRelativeUrl(self, url):
    # Find the forth '/' and strip from there. A bit hackish.
    return '/'.join(self.url.split('/')[:4]) + url

  def RunCommand(self, command, options, args):
    if command == 'update':
      self.update(options, args)
    elif command == 'revert':
      self.revert(options, args)
    elif command == 'status':
      self.status(options, args)
    elif command == 'diff':
      self.diff(options, args)
    else:
      raise Error('Unknown command %s' % command)

  def diff(self, options, args):
    command = ['diff']
    command.extend(args)
    RunSVN(options, command, os.path.join(self._root_dir, self.relpath))

  def update(self, options, args):
    """Runs SCM to update or transparently checkout the working copy.

    Raises:
      Error: if can't get URL for relative path.
    """
    # Only update if git is not controlling the directory.
    git_path = os.path.join(self._root_dir, self.relpath, '.git')
    if options.path_exists(git_path):
      print >> options.stdout, (
          "________ found .git directory; skipping %s" % self.relpath)
      return

    if args:
      raise Error("Unsupported argument(s): %s" % ",".join(args))

    url = self.url
    components = url.split("@")
    revision = None
    forced_revision = False
    if options.revision:
      # Override the revision number.
      url = '%s@%s' % (components[0], str(options.revision))
      revision = int(options.revision)
      forced_revision = True
    elif len(components) == 2:
      revision = int(components[1])
      forced_revision = True

    rev_str = ""
    if revision:
      rev_str = ' at %d' % revision

    if not options.path_exists(os.path.join(self._root_dir, self.relpath)):
      # We need to checkout.
      command = ['checkout', url, os.path.join(self._root_dir, self.relpath)]
      RunSVN(options, command, self._root_dir)
      return

    # Get the existing scm url and the revision number of the current checkout.
    from_info = CaptureSVNInfo(options,
                               os.path.join(self._root_dir, self.relpath, '.'),
                               '.')

    if options.manually_grab_svn_rev:
      # Retrieve the current HEAD version because svn is slow at null updates.
      if not revision:
        from_info_live = CaptureSVNInfo(options, from_info.url, '.')
        revision = int(from_info_live.revision)
        rev_str = ' at %d' % revision

    if from_info.url != components[0]:
      to_info = CaptureSVNInfo(options, url, '.')
      if from_info.root != to_info.root:
        # We have different roots, so check if we can switch --relocate.
        # Subversion only permits this if the repository UUIDs match.
        if from_info.uuid != to_info.uuid:
          raise Error("Can't switch the checkout to %s; UUID don't match" % url)

        # Perform the switch --relocate, then rewrite the from_url
        # to reflect where we "are now."  (This is the same way that
        # Subversion itself handles the metadata when switch --relocate
        # is used.)  This makes the checks below for whether we
        # can update to a revision or have to switch to a different
        # branch work as expected.
        # TODO(maruel):  TEST ME !
        command = ["switch", "--relocate", from_info.root, to_info.root,
                   self.relpath]
        RunSVN(options, command, self._root_dir)
        from_info.url = from_info.url.replace(from_info.root, to_info.root)

    # If the provided url has a revision number that matches the revision
    # number of the existing directory, then we don't need to bother updating.
    if not options.force and from_info.revision == revision:
      if options.verbose or not forced_revision:
        print >>options.stdout, ("\n_____ %s%s" % (
            self.relpath, rev_str))
      return

    command = ["update", os.path.join(self._root_dir, self.relpath)]
    if revision:
      command.extend(['--revision', str(revision)])
    return RunSVN(options, command, self._root_dir)

  def revert(self, options, args):
    """Reverts local modifications. Subversion specific."""
    path = os.path.join(self._root_dir, self.relpath)
    if not os.path.isdir(path):
      # We can't revert path that doesn't exist.
      # TODO(maruel):  Should we update instead?
      if options.verbose:
        print >>options.stdout, ("\n_____ %s is missing, can't revert" %
                                 self.relpath)
      return

    files = CaptureSVNStatus(options, path)
    # Batch the command.
    files_to_revert = []
    for file in files:
      file_path = os.path.join(path, file.path)
      print >>options.stdout, file_path
      # Unversioned file or unexpected unversioned file.
      if file.text_status in ('?', '~'):
        # Remove extraneous file. Also remove unexpected unversioned
        # directories. svn won't touch them but we want to delete these.
        try:
          os.remove(file_path)
        except EnvironmentError:
          RemoveDirectory(file_path)

      if file.text_status != '?':
        # For any other status, svn revert will work.
        files_to_revert.append(file.path)

    # Revert them all at once.
    if files_to_revert:
      accumulated_paths = []
      accumulated_length = 0
      command = ['revert']
      for p in files_to_revert:
        # Some shell have issues with command lines too long.
        if accumulated_length and accumulated_length + len(p) > 3072:
          RunSVN(options, command + accumulated_paths,
                 os.path.join(self._root_dir, self.relpath))
          accumulated_paths = []
          accumulated_length = 0
        else:
          accumulated_paths.append(p)
          accumulated_length += len(p)
      if accumulated_paths:
        RunSVN(options, command + accumulated_paths,
               os.path.join(self._root_dir, self.relpath))

  def status(self, options, args):
    """Display status information."""
    command = ['status']
    command.extend(args)
    RunSVN(options, command, os.path.join(self._root_dir, self.relpath))


## GClient implementation.


class GClient(object):
  """Object that represent a gclient checkout."""

  supported_commands = ['diff', 'revert', 'status', 'update']
  
  def __init__(self, root_dir, options):
    self._root_dir = root_dir
    self._options = options
    self._config_content = None
    self._config_dict = {}

  def SetConfig(self, content):
    self._config_dict = {}
    self._config_content = content
    exec(content, self._config_dict)

  def SaveConfig(self):
    FileWrite(os.path.join(self._root_dir, self._options.config_filename),
              self._config_content)

  def _LoadConfig(self):
    client_source = FileRead(os.path.join(self._root_dir,
                                          self._options.config_filename))
    self.SetConfig(client_source)

  def ConfigContent(self):
    return self._config_content
  
  def GetVar(self, key, default=None):
    return self._config_dict.get(key, default)

  @staticmethod
  def LoadCurrentConfig(options, from_dir=None):
    """Searches for and loads a .gclient file relative to the current working
    dir.

    Returns:
      A dict representing the contents of the .gclient file or an empty dict if
      the .gclient file doesn't exist.
    """
    if not from_dir:
      from_dir = os.curdir
    path = os.path.realpath(from_dir)
    while not options.path_exists(os.path.join(path, options.config_filename)):
      next = os.path.split(path)
      if not next[1]:
        return None
      path = next[0]
    client = options.gclient(path, options)
    client._LoadConfig()
    return client

  def SetDefaultConfig(self, solution_name, solution_url):
    self.SetConfig(DEFAULT_CLIENT_FILE_TEXT % (solution_name, solution_url))

  def _SaveEntries(self, entries):
    """Creates a .gclient_entries file to record the list of unique checkouts.

    The .gclient_entries file lives in the same directory as .gclient.
    
    Args:
      entries: A sequence of solution names.
    """
    text = "entries = [\n"
    for entry in entries:
      text += "  \"%s\",\n" % entry
    text += "]\n"
    FileWrite(os.path.join(self._root_dir, self._options.entries_filename),
              text)

  def _ReadEntries(self):
    """Read the .gclient_entries file for the given client.

    Args:
      client: The client for which the entries file should be read.

    Returns:
      A sequence of solution names, which will be empty if there is the
      entries file hasn't been created yet.
    """
    scope = {}
    filename = os.path.join(self._root_dir, self._options.entries_filename)
    if not self._options.path_exists(filename):
      return []
    exec(FileRead(filename), scope)
    return scope["entries"]

  class FromImpl:
    """Used to implement the From syntax."""

    def __init__(self, module_name):
      self.module_name = module_name

    def __str__(self):
      return 'From("%s")' % self.module_name

  class _VarImpl:
    def __init__(self, custom_vars, local_scope):
      self._custom_vars = custom_vars
      self._local_scope = local_scope

    def Lookup(self, var_name):
      """Implements the Var syntax."""
      if var_name in self._custom_vars:
        return self._custom_vars[var_name]
      elif var_name in self._local_scope.get("vars", {}):
        return self._local_scope["vars"][var_name]
      raise Error("Var is not defined: %s" % var_name)

  def _GetDefaultSolutionDeps(self, solution_name, custom_vars):
    """Fetches the DEPS file for the specified solution.

    Args:
      solution_name: The name of the solution to query.
      vars: A dict of vars to override any vars defined in the DEPS file.

    Returns:
      A dict mapping module names (as relative paths) to URLs or an empty
      dict if the solution does not have a DEPS file.
    """
    deps_file = os.path.join(self._root_dir, solution_name,
                             self._options.deps_file)

    local_scope = {}
    var = self._VarImpl(custom_vars, local_scope)
    global_scope = {"From": self.FromImpl, "Var": var.Lookup, "deps_os": {}}
    try:
      exec(FileRead(deps_file), global_scope, local_scope)
    except EnvironmentError:
      print >> self._options.stdout, (
          "\nWARNING: DEPS file not found for solution: %s\n" % solution_name)
      return {}
    deps = local_scope.get("deps", {})

    # load os specific dependencies if defined.  these dependencies may
    # override or extend the values defined by the 'deps' member.
    if "deps_os" in local_scope:
      deps_os_choices = {
          "win32": "win",
          "win": "win",
          "cygwin": "win",
          "darwin": "mac",
          "mac": "mac",
          "unix": "unix",
          "linux": "unix",
          "linux2": "unix",
         }

      if self._options.deps_os is not None:
        deps_to_include = self._options.deps_os.split(",")
        if "all" in deps_to_include:
          deps_to_include = deps_os_choices.values()
      else:
        deps_to_include = [deps_os_choices.get(self._options.platform, "unix")]

      deps_to_include = set(deps_to_include)
      for deps_os_key in deps_to_include:
        os_deps = local_scope["deps_os"].get(deps_os_key, {})
        if len(deps_to_include) > 1:
          # Ignore any overrides when including deps for more than one
          # platform, so we collect the broadest set of dependencies available.
          # We may end up with the wrong revision of something for our
          # platform, but this is the best we can do.
          deps.update([x for x in os_deps.items() if not x[0] in deps])
        else:
          deps.update(os_deps)

    return deps

  def _GetAllDeps(self, solution_urls):
    """Get the complete list of dependencies for the client.

    Args:
      solution_urls: A dict mapping module names (as relative paths) to URLs
        corresponding to the solutions specified by the client.  This parameter
        is passed as an optimization.

    Returns:
      A dict mapping module names (as relative paths) to URLs corresponding
      to the entire set of dependencies to checkout for the given client.

    Raises:
      Error: If a dependency conflicts with another dependency or of a solution.
    """
    deps = {}
    for solution in self.GetVar("solutions"):
      custom_vars = solution.get("custom_vars", {})
      solution_deps = self._GetDefaultSolutionDeps(solution["name"],
                                                   custom_vars)

      for d in solution_deps:
        if "custom_deps" in solution and d in solution["custom_deps"]:
          # Dependency is overriden.
          url = solution["custom_deps"][d]
          if url is None:
            continue
        else:
          url = solution_deps[d]
          # if we have a From reference dependent on another solution, then
          # just skip the From reference. When we pull deps for the solution,
          # we will take care of this dependency.
          #
          # If multiple solutions all have the same From reference, then we
          # should only add one to our list of dependencies.
          if type(url) != str:
            if url.module_name in solution_urls:
              # Already parsed.
              continue
            if d in deps and type(deps[d]) != str:
              if url.module_name == deps[d].module_name:
                continue
          else:
            parsed_url = urlparse.urlparse(url)
            scheme = parsed_url[0]
            if not scheme:
              # A relative url. Fetch the real base.
              path = parsed_url[2]
              if path[0] != "/":
                raise Error(
                    "relative DEPS entry \"%s\" must begin with a slash" % d)
              # Create a scm just to query the full url.
              scm = self._options.scm_wrapper(solution["url"], self._root_dir,
                                              None)
              url = scm.FullUrlForRelativeUrl(url)
        if d in deps and deps[d] != url:
          raise Error(
              "Solutions have conflicting versions of dependency \"%s\"" % d)
        if d in solution_urls and solution_urls[d] != url:
          raise Error(
              "Dependency \"%s\" conflicts with specified solution" % d)
        # Grab the dependency.
        deps[d] = url
    return deps

  def RunOnDeps(self, command, args):
    """Runs an command on each dependency in a client and its dependencies.

    The module's dependencies are specified in its top-level DEPS files.

    Args:
      command: The command to use (e.g., 'status' or 'diff')
      args: list of str - extra arguments to add to the command line.

    Raises:
      Error: If the client has conflicting entries.
    """
    if not command in self.supported_commands:
      raise Error("'%s' is an unsupported command" % command)

    # Check for revision overrides.
    revision_overrides = {}
    for revision in self._options.revisions:
      if revision.find("@") == -1:
        raise Error(
            "Specify the full dependency when specifying a revision number.")
      # Check if we want to update this solution.
      revision_elem = revision.split("@")
      revision_overrides[revision_elem[0]] = revision_elem[1]

    solutions = self.GetVar("solutions")
    if not solutions:
      raise Error("No solution specified")

    entries = {}
    # Run on the base solutions first.
    for solution in solutions:
      name = solution["name"]
      if name in entries:
        raise Error("solution %s specified more than once" % name)
      url = solution["url"]
      entries[name] = url
      self._options.revision = revision_overrides.get(name)
      scm = self._options.scm_wrapper(url, self._root_dir, name)
      scm.RunCommand(command, self._options, args)

    # Process the dependencies next (sort alphanumerically to ensure that
    # containing directories get populated first and for readability)
    deps = self._GetAllDeps(entries)
    deps_to_process = deps.keys()
    deps_to_process.sort()
    
    # First pass for direct dependencies.
    for d in deps_to_process:
      if type(deps[d]) == str:
        url = deps[d]
        entries[d] = url
        self._options.revision = revision_overrides.get(d)
        scm = self._options.scm_wrapper(url, self._root_dir, d)
        scm.RunCommand(command, self._options, args)

    # Second pass for inherited deps (via the From keyword)
    for d in deps_to_process:
      if type(deps[d]) != str:
        sub_deps = self._GetDefaultSolutionDeps(deps[d].module_name)
        url = sub_deps[d]
        entries[d] = url
        self._options.revision = revision_overrides.get(d)
        scm = self._options.scm_wrapper(url, self._root_dir, d)
        scm.RunCommand(command, self._options, args)

    if command == 'update':
      # notify the user if there is an orphaned entry in their working copy.
      # TODO(darin): we should delete this directory manually if it doesn't
      # have any changes in it.
      prev_entries = self._ReadEntries()
      for entry in prev_entries:
        e_dir = os.path.join(self._root_dir, entry)
        if entry not in entries and self._options.path_exists(e_dir):
          if CaptureSVNStatus(self._options, e_dir):
            # There are modified files in this entry
            entries[entry] = None  # Keep warning until removed.
            print >> self._options.stdout, (
                "\nWARNING: \"%s\" is no longer part of this client.  "
                "It is recommended that you manually remove it.\n") % entry
          else:
            # Delete the entry
            print >> self._options.stdout, ("\n________ deleting \'%s\' " +
                "in \'%s\'") % (entry, self._root_dir)
            RemoveDirectory(e_dir)
      # record the current list of entries for next time
      self._SaveEntries(entries)


## gclient commands.


def DoConfig(options, args):
  """Handle the config subcommand.

  Args:
    options: If options.spec set, a string providing contents of config file.
    args: The command line args.  If spec is not set,
          then args[0] is a string URL to get for config file.

  Raises:
    Error: on usage error
  """
  if len(args) < 1 and not options.spec:
    raise Error("required argument missing; see 'gclient help config'")
  if options.path_exists(options.config_filename):
    raise Error("%s file already exists in the current directory" %
                options.config_filename)
  client = options.gclient('.', options)
  if options.spec:
    client.SetConfig(options.spec)
  else:
    # TODO(darin): it would be nice to be able to specify an alternate relpath
    # for the given URL.
    base_url = args[0]
    name = args[0].split("/")[-1]
    client.SetDefaultConfig(name, base_url)
  client.SaveConfig()


def DoHelp(options, args):
  """Handle the help subcommand giving help for another subcommand.

  Raises:
    Error: if the command is unknown.
  """
  if len(args) == 1 and args[0] in COMMAND_USAGE_TEXT:
    print >>options.stdout, COMMAND_USAGE_TEXT[args[0]]
  else:
    raise Error("unknown subcommand; see 'gclient help'")


def DoStatus(options, args):
  """Handle the status subcommand.

  Raises:
    Error: if client isn't configured properly.
  """
  client = options.gclient.LoadCurrentConfig(options)
  if not client:
    raise Error("client not configured; see 'gclient config'")
  if options.verbose:
    # Print out the .gclient file.  This is longer than if we just printed the
    # client dict, but more legible, and it might contain helpful comments.
    print >>options.stdout, client.ConfigContent()
  options.verbose = True
  return client.RunOnDeps('status', args)


def DoUpdate(options, args):
  """Handle the update and sync subcommands.

  Raises:
    Error: if client isn't configured properly.
  """
  client = options.gclient.LoadCurrentConfig(options)
  if not client:
    raise Error("client not configured; see 'gclient config'")
  if options.verbose:
    # Print out the .gclient file.  This is longer than if we just printed the
    # client dict, but more legible, and it might contain helpful comments.
    print >>options.stdout, client.ConfigContent()
  return client.RunOnDeps('update', args)


def DoDiff(options, args):
  """Handle the diff subcommand.

  Raises:
    Error: if client isn't configured properly.
  """
  client = options.gclient.LoadCurrentConfig(options)
  if not client:
    raise Error("client not configured; see 'gclient config'")
  if options.verbose:
    # Print out the .gclient file.  This is longer than if we just printed the
    # client dict, but more legible, and it might contain helpful comments.
    print >>options.stdout, client.ConfigContent()
  options.verbose = True
  return client.RunOnDeps('diff', args)


def DoRevert(options, args):
  """Handle the revert subcommand.

  Raises:
    Error: if client isn't configured properly.
  """
  client = options.gclient.LoadCurrentConfig(options)
  if not client:
    raise Error("client not configured; see 'gclient config'")
  return client.RunOnDeps('revert', args)


gclient_command_map = {
    "config": DoConfig,
    "diff": DoDiff,
    "help": DoHelp,
    "status": DoStatus,
    "sync": DoUpdate,
    "update": DoUpdate,
    "revert": DoRevert,
    }


def DispatchCommand(command, options, args, command_map=None):
  """Dispatches the appropriate subcommand based on command line arguments."""
  if command_map is None:
    command_map = gclient_command_map

  if command in command_map:
    return command_map[command](options, args)
  else:
    raise Error("unknown subcommand; see 'gclient help'")


def Main(argv):
  """Parse command line arguments and dispatch command."""

  option_parser = optparse.OptionParser(usage=DEFAULT_USAGE_TEXT,
                                        version=__version__)
  option_parser.disable_interspersed_args()
  option_parser.add_option("", "--force", action="store_true", default=False,
                           help=("(update/sync only) force update even "
                                 "for modules which haven't changed"))
  option_parser.add_option("", "--revision", action="append", dest="revisions",
                           metavar="REV", default=[],
                           help=("(update/sync only) sync to a specific "
                                 "revision, can be used multiple times for "
                                 "each solution, e.g. --revision=src@123, "
                                 "--revision=internal@32"))
  option_parser.add_option("", "--deps", default=None, dest="deps_os",
                           metavar="OS_LIST",
                           help=("(update/sync only) sync deps for the "
                                 "specified (comma-separated) platform(s); "
                                 "'all' will sync all platforms"))
  option_parser.add_option("", "--spec", default=None,
                           help=("(config only) create a gclient file "
                                 "containing the provided string"))
  option_parser.add_option("", "--verbose", action="store_true", default=False,
                           help="produce additional output for diagnostics")
  option_parser.add_option("", "--manually_grab_svn_rev", action="store_true",
                           default=False,
                           help="Skip svn up whenever possible by requesting "
                           "actual HEAD revision from the repository")

  if len(argv) < 2:
    # Users don't need to be told to use the 'help' command.
    option_parser.print_help()
    return 1

  command = argv[1]
  options, args = option_parser.parse_args(argv[2:])

  if len(argv) < 3 and command == "help":
    option_parser.print_help()
    return 0

  # Files used for configuration and state saving.
  options.config_filename = os.environ.get("GCLIENT_FILE", ".gclient")
  options.entries_filename = ".gclient_entries"
  options.deps_file = "DEPS"

  # These are overridded when testing. They are not externally visible.
  options.stdout = sys.stdout
  options.path_exists = os.path.exists
  options.gclient = GClient
  options.scm_wrapper = SCMWrapper
  options.platform = sys.platform
  return DispatchCommand(command, options, args)


if "__main__" == __name__:
  try:
    result = Main(sys.argv)
  except Error, e:
    print "Error: %s" % e.message
    result = 1
  sys.exit(result)
