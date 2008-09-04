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

"""A wrapper script to manage a set of client modules in (initially) svn.

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
                  submodule name to a URL where it can be found (via svn)
"""

__author__ = "darinf@gmail.com (Darin Fisher)"

import optparse
import os
import subprocess
import sys
import urlparse

SVN_COMMAND = "svn"
CLIENT_FILE = os.environ.get("GCLIENT_FILE", ".gclient")
CLIENT_ENTRIES_FILE = ".gclient_entries"
DEPS_FILE = "DEPS"

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
modules to operate on as well.  If optional [svnurl] parameter is
provided, then configuration is read from a specified Subversion server
URL.  Otherwise, a --spec option must be provided.

usage: config [option | svnurl]

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

# parameterized by (solution_name, solution_svnurl)
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

# -----------------------------------------------------------------------------
# generic utils:


class Error(Exception):
  """gclient exception class."""

  def __init__(self, message):
    Exception.__init__(self)
    self.message = message

# -----------------------------------------------------------------------------
# SVN utils:


def RunSVN(args, in_directory,
           output_stream=sys.stdout,
           call=subprocess.call,
           realpath=os.path.realpath):
  """Runs svn, sending output to stdout.

  Args:
    args: A sequence of command line parameters to be passed to svn.
    in_directory: The directory where svn is to be run.

    Dependencies (for testing)
    output_stream: stream for messages to the user
    call: "subprocess.call" function
    realpath: "os.path.realpath" function

  Raises:
    Error: An error occurred while running the svn command.
  """
  c = [SVN_COMMAND]
  c.extend(args)
  print >> output_stream, (
      "\n________ running \'%s\' in \'%s\'" % (" ".join(c),
                                               realpath(in_directory)))
  output_stream.flush()  # flush our stdout so it shows up first.

  # *Sigh*:  Windows needs shell=True, or else it won't search %PATH% for
  # the svn.exe executable, but shell=True makes subprocess on Linux fail
  # when it's called with a list because it only tries to execute the
  # first string ("svn").
  rv = call(c, cwd=in_directory, shell=(sys.platform == 'win32'))

  if rv:
    raise Error("failed to run command: %s" % " ".join(c))
  return rv


def CaptureSVN(args, in_directory, verbose):
  """Runs svn, capturing output sent to stdout as a string.

  Args:
    args: A sequence of command line parameters to be passed to svn.
    in_directory: The directory where svn is to be run.
    verbose: Enables verbose output if true.

  Returns:
    The output sent to stdout as a string.
  """
  c = [SVN_COMMAND]
  c.extend(args)
  if verbose:
    print ("\n________ running \'%s\' in \'%s\'"
           % (" ".join(c), os.path.realpath(in_directory)))
    sys.stdout.flush()  # flush our stdout so it shows up first.

  # *Sigh*:  Windows needs shell=True, or else it won't search %PATH% for
  # the svn.exe executable, but shell=True makes subprocess on Linux fail
  # when it's called with a list because it only tries to execute the
  # first string ("svn").
  return subprocess.Popen(c, cwd=in_directory, shell=(sys.platform == 'win32'),
                          stdout=subprocess.PIPE).communicate()[0]


def CaptureSVNInfo(relpath, in_directory, verbose):
  """Runs 'svn info' on an existing path.

  Args:
    relpath: The directory where the working copy resides relative to
      the directory given by in_directory.
    in_directory: The directory where svn is to be run.
    verbose: Enables verbose output if true.

  Returns:
    A dict of fields corresponding to the output of 'svn info'
  """
  info = CaptureSVN(["info", relpath], in_directory, verbose)
  result = {}
  for line in info.splitlines():
    fields = line.split(": ")
    if len(fields) > 1:
      result[fields[0]] = fields[1]
  return result


def RunSVNCommandForModule(command, relpath, root_dir, args):
  """Runs an svn command for a single subversion module.

  Args:
    command: The svn command to use (e.g., "status" or "diff")
    relpath: The directory where the working copy should reside relative
      to the given root_dir.
    root_dir: The directory from which relpath is relative.
    args: list of str - extra arguments to add to the svn command line.
  """
  c = [command]
  c.extend(args)
  return RunSVN(c, os.path.join(root_dir, relpath))


def UpdateToURL(relpath, svnurl, root_dir, options, args,
                output_stream=sys.stdout,
                path_exists=os.path.exists,
                capture_svn_info=CaptureSVNInfo,
                run_svn=RunSVN):
  """Runs svn to checkout or update the working copy.

  Args:
    relpath: The directory where the working copy should reside relative
      to the given root_dir.
    svnurl: The svn URL to checkout or update the relpath to.
    root_dir: The directory from which relpath is relative.
    options: The Options object; attributes we care about:
      verbose - If true, then print diagnostic output.
      force - If true, also update modules with unchanged repository version.
    args: list of str - extra arguments to add to the svn command line.

    output_stream: stream for user messages
    path_exists: os.path.exists (for testing)
    capture_svn_info: CaptureSVNInfo (for testing)
    run_svn: RunSVN (for testing)

  Raises:
    Error: if can't get URL for relative path.
  """
  comps = svnurl.split("@")
  # by default, we run the svn command at the root directory level
  run_dir = root_dir
  if path_exists(os.path.join(root_dir, relpath)):
    # get the existing svn url and revision number:
    from_info = capture_svn_info(relpath, root_dir, options.verbose)
    from_url = from_info.get("URL", None)
    if from_url is None:
      raise Error(
          "Couldn't get URL for relative path: '%s' under root directory: %s.\n"
          "\tSVN URL was:\n\t\t%s\n"
          "\tInfo dict was:\n\t\t%s"
          % (relpath, root_dir, svnurl, from_info))

    if comps[0] != from_url:

      to_info = capture_svn_info(svnurl, root_dir, options.verbose)
      from_repository_root = from_info.get("Repository Root", None)
      to_repository_root = to_info.get("Repository Root", None)

      if from_repository_root and from_repository_root != to_repository_root:

        # We have different roots, so check if we can switch --relocate.
        # Subversion only permits this if the repository UUIDs match.
        from_repository_uuid = from_info.get("Repository UUID", None)
        to_repository_uuid = to_info.get("Repository UUID", None)
        if from_repository_uuid != to_repository_uuid:
          print >>output_stream, ("Skipping update to %s;\n"
                                  "\tcan not relocate to URL with different"
                                  " Repository UUID.\n"
                                  % (svnurl))
          return

        if not options.relocate:
          print >>output_stream, ("Skipping update to %s;\n"
                                  "\tuse the --relocate option to switch\n"
                                  "\tfrom %s\n"
                                  "\tto   %s.\n"
                                  % (svnurl, from_url, comps[0]))
          return

        # Perform the switch --relocate, then rewrite the from_url
        # to reflect where we "are now."  (This is the same way that
        # Subversion itself handles the metadata when switch --relocate
        # is used.)  This makes the checks below for whether we
        # can update to a revision or have to switch to a different
        # branch work as expected.
        run_svn(["switch", "--relocate",
                 from_repository_root, to_repository_root, relpath], root_dir)
        from_url = from_url.replace(from_repository_root, to_repository_root)

    # by default, we assume that we cannot just use 'svn update'
    can_update = False

    # if the provided svn url has a revision number that matches the revision
    # number of the existing directory, then we don't need to bother updating.
    if comps[0] == from_url:
      can_update = True
      if (not options.force and
          len(comps) > 1 and comps[1] == from_info["Revision"]):
        if options.verbose:
          print >>output_stream, ("\n_____ %s at %s" %
                                  (from_info["URL"], from_info["Revision"]))
        return

    if can_update:
      # ok, we can update; adjust run_dir accordingly
      c = ["update"]
      if len(comps) > 1:
        c.extend(["-r", comps[1]])
      run_dir = os.path.join(root_dir, relpath)
    else:
      # otherwise, switch to the new svn url
      c = ["switch"]
      if len(comps) > 1:
        c.extend(["-r", comps[1]])
      c.extend([comps[0], relpath])
  else:
    c = ["checkout", svnurl, relpath]
  if args:
    c.extend(args)

  return run_svn(c, run_dir)

# -----------------------------------------------------------------------------
# gclient ops:


def CreateClientFileFromText(text):
  """Creates a .gclient file in the current directory from the given text.

  Args:
    text: The text of the .gclient file.
  """
  try:
    f = open(CLIENT_FILE, "w")
    f.write(text)
  finally:
    f.close()


def CreateClientFile(solution_name, solution_url):
  """Creates a default .gclient file in the current directory.

  Args:
    solution_name: The name of the solution.
    solution_url: The svn URL of the solution.
  """
  text = DEFAULT_CLIENT_FILE_TEXT % (solution_name, solution_url)
  CreateClientFileFromText(text)


def CreateClientEntriesFile(client, entries):
  """Creates a .gclient_entries file to record the list of unique svn checkouts.

  The .gclient_entries file lives in the same directory as .gclient.
  
  Args:
    client: The client for which the entries file should be written.
    entries: A sequence of solution names.
  """
  text = "entries = [\n"
  for entry in entries:
    text += "  \"%s\",\n" % entry
  text += "]\n"
  f = open("%s/%s" % (client["root_dir"], CLIENT_ENTRIES_FILE), "w")
  f.write(text)
  f.close()


def ReadClientEntriesFile(client):
  """Read the .gclient_entries file for the given client.

  Args:
    client: The client for which the entries file should be read.

  Returns:
    A sequence of solution names, which will be empty if there is the
    entries file hasn't been created yet.
  """
  path = os.path.join(client["root_dir"], CLIENT_ENTRIES_FILE)
  if not os.path.exists(path):
    return []
  scope = {}
  execfile(path, scope)
  return scope["entries"]


def GetClient():
  """Searches for and loads a .gclient file relative to the current working dir.

  Returns:
    A dict representing the contents of the .gclient file or an empty dict if
    the .gclient file doesn't exist.
  """
  path = os.path.realpath(os.curdir)
  client_file = os.path.join(path, CLIENT_FILE)
  while not os.path.exists(client_file):
    next = os.path.split(path)
    if not next[1]:
      return {}
    path = next[0]
    client_file = os.path.join(path, CLIENT_FILE)
  client = {}
  client_fo = open(client_file)
  try:
    client_source = client_fo.read()
    exec(client_source, client)
  finally:
    client_fo.close()
  # record the root directory and client source for later use
  client["root_dir"] = path
  client["source"] = client_source
  return client


class FromImpl:
  """Used to implement the From syntax."""

  def __init__(self, module_name):
    self.module_name = module_name

  def __str__(self):
    return 'From("%s")' % self.module_name


def GetDefaultSolutionDeps(client, solution_name, platform=None,
                           execf=execfile,
                           logger=sys.stdout):
  """Fetches the DEPS file for the specified solution.

  Args:
    client: The client containing the specified solution.
    solution_name: The name of the solution to query.
    platform: os platform (i.e. the output of sys.platform)
    execf: execfile function for testing
    logger: stream for user output

  Returns:
    A dict mapping module names (as relative paths) to svn URLs or an empty
    dict if the solution does not have a DEPS file.
  """
  deps_file = os.path.join(client["root_dir"], solution_name, DEPS_FILE)
  scope = {"From": FromImpl, "deps_os": {}}
  try:
    execf(deps_file, scope)
  except EnvironmentError:
    print >> logger, (
        "\nWARNING: DEPS file not found for solution: %s\n" % solution_name)
    return {}
  deps = scope.get("deps", {})
  # load os specific dependencies if defined.  these dependencies may override
  # or extend the values defined by the 'deps' member.
  if platform is None:
    platform = sys.platform
  deps_os_key = {
      "win32": "win",
      "win": "win",
      "darwin": "mac",
      "mac": "mac",
      "unix": "unix",
  }.get(platform, "unix")
  deps.update(scope["deps_os"].get(deps_os_key, {}))
  return deps


def GetAllDeps(client, solution_urls,
               get_default_solution_deps=GetDefaultSolutionDeps,
               capture_svn_info=CaptureSVNInfo):
  """Get the complete list of dependencies for the client.

  Args:
    client: The client for which to gather dependencies.
    solution_urls: A dict mapping module names (as relative paths) to svn URLs
      corresponding to the solutions specified by the client.  This parameter
      is passed as an optimization.

    get_default_solution_deps: GetDefaultSolutionDeps (for testing)
    capture_svn_info: CaptureSVNInfo (for testing)

  Returns:
    A dict mapping module names (as relative paths) to svn URLs corresponding
    to the entire set of dependencies to checkout for the given client.

  Raises:
    Error: If a dependency conflicts with another dependency or of a solution.
  """
  deps = {}
  for solution in client["solutions"]:
    solution_deps = get_default_solution_deps(client, solution["name"])
    for d in solution_deps:
      if "custom_deps" in solution and d in solution["custom_deps"]:
        url = solution["custom_deps"][d]
        if url is None:
          continue
      else:
        url = solution_deps[d]
        #
        # if we have a From reference dependent on another solution, then just
        # skip the From reference.  when we pull deps for the solution, we will
        # take care of this dependency.
        #
        # If multiple solutions all have the same From reference, then we
        # should only add one to our list of dependencies.
        #
        if type(url) != str:
          if url.module_name in solution_urls:
            continue
          if d in deps and type(deps[d]) != str:
            if url.module_name == deps[d].module_name:
              continue
        else:
          parsed_url = urlparse.urlparse(url)
          scheme = parsed_url[0]
          if not scheme:
            path = parsed_url[2]
            if path[0] != "/":
              raise Error(
                  "relative DEPS entry \"%s\" must begin with a slash" % d)
            info = capture_svn_info(solution["url"], client["root_dir"], False)
            url = info["Repository Root"] + url
      if d in deps and deps[d] != url:
        raise Error(
            "solutions have conflicting versions of dependency \"%s\"" % d)
      if d in solution_urls and solution_urls[d] != url:
        raise Error(
            "dependency \"%s\" conflicts with specified solution" % d)
      deps[d] = url
  return deps


def RunSVNCommandForClientModules(
    command, client, verbose, args,
    run_svn_command_for_module=RunSVNCommandForModule,
    get_all_deps=GetAllDeps):
  """Runs an svn command on each svn module in a client and its dependencies.

  The module's dependencies are specified in its top-level DEPS files.

  Args:
    command: The svn command to use (e.g., "status" or "diff")
    client: The client for which to run the commands.
    verbose: If true, then print diagnostic output.
    args: list of str - extra arguments to add to the svn command line.
    run_svn_command_for_module: RunSVNCommandForModule (for testing)
    get_all_deps: GetAllDeps (for testing)

  Raises:
    Error: If the client has conflicting entries.
  """
  verbose = verbose  # Suppress lint warning.
  entries = {}

  # run on the base solutions first
  for s in client["solutions"]:
    name = s["name"]
    if name in entries:
      raise Error("solution specified more than once")
    entries[name] = s["url"]
    run_svn_command_for_module(command, name, client["root_dir"], args)

  # do the module dependencies next (sort alphanumerically for
  # readability)
  deps_to_show = get_all_deps(client, entries).keys()
  deps_to_show.sort()
  for d in deps_to_show:
    run_svn_command_for_module(command, d, client["root_dir"], args)


def UpdateAll(client, options, args,
              update_to_url=UpdateToURL,
              get_all_deps=GetAllDeps,
              create_client_entries_file=CreateClientEntriesFile,
              read_client_entries_file=ReadClientEntriesFile,
              get_default_solution_deps=GetDefaultSolutionDeps,
              path_exists=os.path.exists,
              logger=sys.stdout):
  """Update all solutions and their dependencies.

  Args:
    client: The client to update.
    options: Options object; attributes we care about:
      verbose - If true, then print diagnostic output.
      force - If true, then also update modules with unchanged repo version.
      revision - If specified, a string SOLUTION@REV or just REV
    args: list of str - extra arguments to add to the svn command line.

    update_to_url: dependency (for testing)
    get_all_deps: dependency (for testing)
    create_client_entries_file: dependency (for testing)
    read_client_entries_file: dependency (for testing)
    get_default_solution_deps: dependency (for testing)
    path_exists: dependency (for testing)
    logger: dependency (for testing)

  Raises:
    Error: If the client has conflicting entries.
  """
  entries = {}
  result = 0
  # update the solutions first so we can read their dependencies
  for s in client["solutions"]:
    name = s["name"]
    if name in entries:
      raise Error("solution specified more than once")
    url = s["url"]

    # Check if we should sync to a given revision
    if options.revision:
      url_elem = url.split("@")
      if options.revision.find("@") == -1:
        # We want to update all solutions.
        url = url_elem[0] + "@" + options.revision
      else:
        # Check if we want to update this solution.
        revision_elem = options.revision.split("@")
        if revision_elem[0] == name:
          url = url_elem[0] + "@" + revision_elem[1]

    entries[name] = url
    r = update_to_url(name, url, client["root_dir"], options, args)
    if r and result == 0:
      result = r

  # update the dependencies next (sort alphanumerically to ensure that
  # containing directories get populated first)
  deps = get_all_deps(client, entries)
  deps_to_update = deps.keys()
  deps_to_update.sort()
  # first pass for explicit deps
  for d in deps_to_update:
    if type(deps[d]) == str:
      entries[d] = deps[d]
      r = update_to_url(d, deps[d], client["root_dir"], options, args)
      if r and result == 0:
        result = r
  # first pass for inherited deps (via the From keyword)
  for d in deps_to_update:
    if type(deps[d]) != str:
      sub_deps = get_default_solution_deps(client, deps[d].module_name)
      entries[d] = sub_deps[d]
      r = update_to_url(d, sub_deps[d], client["root_dir"], options, args)
      if r and result == 0:
        result = r

  # notify the user if there is an orphaned entry in their working copy.
  # TODO(darin): we should delete this directory manually if it doesn't
  # have any changes in it.
  prev_entries = read_client_entries_file(client)
  for entry in prev_entries:
    e_dir = "%s/%s" % (client["root_dir"], entry)
    if entry not in entries and path_exists(e_dir):
      entries[entry] = None  # keep warning until removed
      print >> logger, (
          "\nWARNING: \"%s\" is no longer part of this client.  "
          "It is recommended that you manually remove it.\n") % entry

  # record the current list of entries for next time
  create_client_entries_file(client, entries)

  return result

# -----------------------------------------------------------------------------


def DoConfig(options, args, client_file=CLIENT_FILE,
             path_exists=os.path.exists,
             create_client_file_from_text=CreateClientFileFromText,
             create_client_file=CreateClientFile):
  """Handle the config subcommand.

  Args:
    options: If options.spec set, a string providing contents of config file.
    args: The command line args.  If spec is not set,
          then args[0] is a string URL to get for config file.
    client_file: name of gclient file.

    path_exists: dependency (for testing)
    create_client_file_from_text: dependency (for testing)
    create_client_file: dependency (for testing)

  Raises:
    Error: on usage error
  """
  if len(args) < 1 and not options.spec:
    raise Error("required argument missing; see 'gclient help config'")
  if path_exists(client_file):
    raise Error(".gclient file already exists in the current directory")
  if options.spec:
    create_client_file_from_text(options.spec)
  else:
    # TODO(darin): it would be nice to be able to specify an alternate relpath
    # for the given svn URL.
    svnurl = args[0]
    name = args[0].split("/")[-1]
    create_client_file(name, svnurl)


def DoHelp(options, args,
           output_stream=sys.stdout):
  """Handle the help subcommand giving help for another subcommand.

  Args:
    options: The command line options.
    args: The command line args.
    output_stream: dependency (for testing).

  Raises:
    Error: if the command is unknown.
  """
  options = options   # suppress lint warning
  if len(args) == 1 and args[0] in COMMAND_USAGE_TEXT:
    print >>output_stream, COMMAND_USAGE_TEXT[args[0]]
  else:
    raise Error("unknown subcommand; see 'gclient help'")


def DoStatus(options, args,
             get_client=GetClient,
             run_svn_command_for_client_modules=RunSVNCommandForClientModules):
  """Handle the status subcommand.

  Args:
    options: set options.verbose for extra logging
    args: list of str - extra arguments to add to the svn command line.
    get_client: dependency (for testing).
    run_svn_command_for_client_modules: dependency (for testing)

  Raises:
    Error: if client isn't configured properly.
  """
  client = get_client()
  if not client:
    raise Error("client not configured; see 'gclient config'")
  return run_svn_command_for_client_modules("status", client,
                                            options.verbose, args)


def DoUpdate(options, args,
             get_client=GetClient,
             update_all=UpdateAll,
             output_stream=sys.stdout):
  """Handle the update and sync subcommands."""
  client = get_client()
  if not client:
    raise Error("client not configured; see 'gclient config'")
  if options.verbose:
    # Print out the .gclient file.  This is longer than if we just printed the
    # client dict, but more legible, and it might contain helpful comments.
    print >>output_stream, client["source"]
  return update_all(client, options, args)


def DoDiff(options, args,
           get_client=GetClient,
           run_svn_command_for_client_modules=RunSVNCommandForClientModules,
           output_stream=sys.stdout):
  """Handle the diff subcommand."""
  client = get_client()
  if not client:
    raise Error("client not configured; see 'gclient config'")
  if options.verbose:
    # Print out the .gclient file.  This is longer than if we just printed the
    # client dict, but more legible, and it might contain helpful comments.
    print >>output_stream, client["source"]
  return run_svn_command_for_client_modules("diff", client,
                                            options.verbose, args)

def DoRevert(options, args,
             get_client=GetClient,
             run_svn_command_for_client_modules=RunSVNCommandForClientModules):
  """Handle the revert subcommand."""
  client = get_client()
  if not client:
    raise Error("client not configured; see 'gclient config'")
  args.append("--recursive")
  args.append("*.*")
  return run_svn_command_for_client_modules("revert", client,
                                            options.verbose, args)


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
  if len(argv) < 2:
    raise Error("required subcommand missing; see 'gclient help'")

  command = argv[1]

  option_parser = optparse.OptionParser()
  option_parser.disable_interspersed_args()
  option_parser.set_usage(DEFAULT_USAGE_TEXT)
  option_parser.add_option("", "--force", action="store_true", default=False,
                           help=("(update/sync only) force update even "
                                 "for modules which haven't changed"))
  option_parser.add_option("", "--relocate", action="store_true",
                           default=False,
                           help="relocate")
  option_parser.add_option("", "--revision", default=None,
                           help=("(update/sync only) sync to a specific "
                                 "revision"))
  option_parser.add_option("", "--spec", default=None,
                           help=("(config only) create a gclient file "
                                 "containing the provided string"))
  option_parser.add_option("", "--verbose", action="store_true", default=False,
                           help="produce additional output for diagnostics")

  options, args = option_parser.parse_args(argv[2:])

  if len(argv) < 3 and command == "help":
    option_parser.print_help()
    sys.exit(0)

  return DispatchCommand(command, options, args)

if "__main__" == __name__:
  try:
    result = Main(sys.argv)
  except Error, e:
    print "Error: %s" % e.message
    result = 1
  sys.exit(result)
