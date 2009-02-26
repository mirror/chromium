#!/usr/bin/python
#
# Copyright 2008 Google Inc.
#
# Locate the tryserver script and execute it. The try server script contains
# the repository-specific try server commands.

import datetime
import getpass
import optparse
import os
import shutil
import sys
import tempfile
import traceback
import urllib

import gcl


# Constants
HELP_STRING = "Sorry, Tryserver is not available."
SCRIPT_PATH = os.path.join('tools', 'tryserver', 'tryserver.py')


class InvalidScript(Exception): 
  def __str__(self):
    return self.args[0] + '\n' + HELP_STRING


class NoTryServerAccess(Exception):
  def __str__(self):
    return self.args[0] + '\n' + HELP_STRING


def PathDifference(root, subpath):
  """Returns the difference subpath minus root."""
  if subpath.find(root) != 0:
    return None
  # The + 1 is for the trailing / or \.
  return subpath[len(root) + len(os.sep):]


def GetSourceRoot():
  """Returns the absolute directory one level up from the repository root."""
  return os.path.abspath(os.path.join(gcl.GetRepositoryRoot(), '..'))


def ExecuteTryServerScript():
  """Execute the try server script and returns its dictionary."""
  script_path = os.path.join(gcl.GetRepositoryRoot(), SCRIPT_PATH)
  script_locals = {}
  if os.path.exists(script_path):
    try:
      exec(gcl.ReadFile(script_path), script_locals)
    except Exception, e:
      # TODO(maruel):  Need to specialize the exception trapper.
      traceback.print_exc()
      raise InvalidScript('%s is invalid.' % script_path)
  return script_locals


def EscapeDot(name):
  return name.replace('.', '-')


def RunCommand(command):
  output, retcode = gcl.RunShellWithReturnCode(command)
  if retcode:
    raise NoTryServerAccess(' '.join(command) + '\nOuput:\n' + output)
  return output


def GenerateDiff(files, root=None):
  """Returns a string containing the diff for the given file list.

  The files in the list should either be absolute paths or relative to the
  given root. If no root directory is provided, the repository root will be
  used.
  """
  previous_cwd = os.getcwd()
  if root is None:
    os.chdir(gcl.GetRepositoryRoot())
  else:
    os.chdir(root)

  diff = []
  for file in files:
    # Use svn info output instead of os.path.isdir because the latter fails
    # when the file is deleted.
    if gcl.GetSVNFileInfo(file).get("Node Kind") == "directory":
      continue
    # If the user specified a custom diff command in their svn config file,
    # then it'll be used when we do svn diff, which we don't want to happen
    # since we want the unified diff.  Using --diff-cmd=diff doesn't always
    # work, since they can have another diff executable in their path that
    # gives different line endings.  So we use a bogus temp directory as the
    # config directory, which gets around these problems.
    if sys.platform.startswith("win"):
      parent_dir = tempfile.gettempdir()
    else:
      parent_dir = sys.path[0]  # tempdir is not secure.
    bogus_dir = os.path.join(parent_dir, "temp_svn_config")
    if not os.path.exists(bogus_dir):
      os.mkdir(bogus_dir)
    # Grabs the diff data.
    data = gcl.RunShell(["svn", "diff", "--config-dir", bogus_dir, file])
    # Does it contain a mergeinfo? Note that we could have used the '+' status
    # information too.
    if 'svn:mergeinfo' in data:
      # We know the diff will be incorrectly formatted. Fix it.
      file_infos = gcl.GetSVNFileInfo(file)
      if (file_infos.get('Copied From URL') and
          file_infos.get('Copied From Rev')):
        # The file is "new" in the patch sense. Generate a homebrew diff.
        # We can't use ReadFile() since it's not using binary mode.
        file_handle = open(file, 'rb')
        file_content = file_handle.read()
        file_handle.close()
        file_content = gcl.ReadFile(file)
        # Prepend '+ ' to every lines.
        file_content = ['+ ' + i for i in file_content.splitlines(True)]
        nb_lines = len(file_content)
        data = "Index: %s\n" % file
        data += ("============================================================="
                 "======\n")
        # Note: Should we use /dev/null instead?
        data += "--- %s\n" % file
        data += "+++ %s\n" % file
        data += "@@ -0,0 +1,%d @@\n" % nb_lines
        data += ''.join(file_content)
    diff.append(data)
  os.chdir(previous_cwd)
  return "".join(diff)


def _SendChangeHTTP(options):
  """Send a change to the try server using the HTTP protocol."""
  script_locals = ExecuteTryServerScript()

  if not options.host:
    options.host = script_locals.get('try_server_http_host', None)
    if not options.host:
      raise NoTryServerAccess('Please use the --host option to specify the try '
          'server host to connect to.')
  if not options.port:
    options.port = script_locals.get('try_server_http_port', None)
    if not options.port:
      raise NoTryServerAccess('Please use the --port option to specify the try '
          'server port to connect to.')

  values = {}
  if options.email:
    values['email'] = options.email
  values['user'] = options.user
  values['name'] = options.name
  if options.bot:
    values['bot'] = ','.join(options.bot)
  if options.revision:
    values['revision'] = options.revision
  if options.tests:
    values['tests'] = ','.join(options.tests)
  if options.root:
    values['root'] = options.root
  values['patch'] = options.diff
  
  url = 'http://%s:%s/send_try_patch' % (options.host, options.port)
  proxies = None
  if options.proxy:
    if options.proxy.lower() == 'none':
      # Effectively disable HTTP_PROXY or Internet settings proxy setup.
      proxies = {}
    else:
      proxies = {'http': options.proxy, 'https': options.proxy}
  connection = urllib.urlopen(url, urllib.urlencode(values), proxies=proxies)
  if not connection:
    raise NoTryServerAccess('%s is unaccessible.' % url)
  if connection.read() != 'OK':
    raise NoTryServerAccess('%s is unaccessible.' % url)
  return options.name


def _SendChangeSVN(options):
  """Send a change to the try server by committing a diff file on a subversion
  server."""
  script_locals = ExecuteTryServerScript()
  if not options.svn_repo:
    options.svn_repo = script_locals.get('try_server_svn', None)
    if not options.svn_repo:
      raise NoTryServerAccess('Please use the --svn_repo option to specify the'
                              ' try server svn repository to connect to.')

  values = {}
  if options.email:
    values['email'] = options.email
  values['user'] = options.user
  values['name'] = options.name
  if options.bot:
    values['bot'] = ','.join(options.bot)
  if options.revision:
    values['revision'] = options.revision
  if options.tests:
    values['tests'] = ','.join(options.tests)
  if options.root:
    values['root'] = options.root
  
  description = ''
  for (k,v) in values.iteritems():
    description += "%s=%s\n" % (k,v)

  # Do an empty checkout.
  temp_dir = tempfile.mkdtemp()
  temp_file = tempfile.NamedTemporaryFile()
  temp_file_name = temp_file.name
  try:
    RunCommand(['svn', 'checkout', '--depth', 'empty', '--non-interactive',
                options.svn_repo, temp_dir])
    # TODO(maruel): Use a subdirectory per user?
    current_time = str(datetime.datetime.now()).replace(':', '.')
    file_name = (EscapeDot(options.user) + '.' + EscapeDot(options.name) +
                 '.%s.diff' % current_time)
    full_path = os.path.join(temp_dir, file_name)
    full_url = options.svn_repo + '/' + file_name
    file_found = False
    try:
      RunCommand(['svn', 'ls', '--non-interactive', full_url])
      file_found = True
    except NoTryServerAccess:
      pass
    if file_found:
      # The file already exists in the repo. Note that commiting a file is a
      # no-op if the file's content (the diff) is not modified. This is why the
      # file name contains the date and time.
      RunCommand(['svn', 'update', '--non-interactive', full_path])
      file = open(full_path, 'wb')
      file.write(options.diff)
      file.close()
    else:
      # Add the file to the repo
      file = open(full_path, 'wb')
      file.write(options.diff)
      file.close()
      RunCommand(["svn", "add", '--non-interactive', full_path])
    temp_file.write(description)
    temp_file.flush()
    RunCommand(["svn", "commit", '--non-interactive', full_path, '--file',
                temp_file_name])
  finally:
    temp_file.close()
    shutil.rmtree(temp_dir, True)
  return options.name


def TryChange(argv, name='Unnamed', file_list=None, swallow_exception=False,
              issue=None, patchset=None):
  # Parse argv
  parser = optparse.OptionParser(usage="%prog [options]", version="%prog 0.8a")

  group = optparse.OptionGroup(parser, "Result and status options")
  group.add_option("-u", "--user", default=getpass.getuser(),
                   help="User name to be used.")
  group.add_option("-e", "--email", default=os.environ.get('EMAIL_ADDRESS'),
                   help="Email address where to send the results. Use the "
                        "EMAIL_ADDRESS environment variable to set the default "
                        "email address")
  group.add_option("-n", "--name", default=name,
                   help="Name of the try change.")
  parser.add_option_group(group)

  group = optparse.OptionGroup(parser, "Try run options")
  group.add_option("-b", "--bot", action="append",
                    help="Force the use specifics build slave")
  group.add_option("-r", "--revision", default=None, type='int',
                    help="Revision to use for testing.")
  # group.add_option("-t", "--tests", action="append",
  #                   help="Override the list of tests to run, use multiple"
  #                        "times to list many tests (or comma separated)")
  parser.add_option_group(group)

  group = optparse.OptionGroup(parser, "Which patch to run")
  group.add_option("-f", "--file", default=file_list, dest="files",
                   metavar="FILE", action="append",
                   help="Use many times to list the files to include in the "
                        "try, relative to the repository root.")
  # group.add_option("-i", "--issue", default=issue,
  #                  help="Rietveld's issue id to use instead of a local"
  #                       " diff.")
  # group.add_option("-p", "--patchset", default=patchset,
  #                  help="Rietveld's patchset id to use instead of a local"
  #                       " diff.")
  group.add_option("--diff", default=None,
                   help="File containing the diff to try.")
  group.add_option("--url", default=None,
                   help="Url where to grab a patch.")
  group.add_option("--root", default=None,
                   help="Root to use for the patch.")
  parser.add_option_group(group)

  group = optparse.OptionGroup(parser, "How to access the try server")
  group.add_option("--use_http", action="store_true",
                   help="Use HTTP to talk to the try server.")
  group.add_option("--host", default=None,
                   help="Host address to use to talk to the try server.")
  group.add_option("--port", default=None,
                   help="HTTP port to use to talk to the try server.")
  group.add_option("--proxy", default=None,
                   help="HTTP proxy.")
  group.add_option("--use_svn", action="store_true",
                   help="Use SVN to talk to the try server.")
  group.add_option("--svn_repo", default=None,
                   help="SVN url to use to write the changes in.")
  parser.add_option_group(group)

  options, args = parser.parse_args(argv)
  # TODO(maruel): Enable these flags:
  options.tests = None
  options.issue = None
  options.patchset = None

  if len(args) == 1 and args[0] == 'help':
    parser.print_help()
  if (not options.files and (not options.issue and options.patchset) and
      not options.diff and not options.url):
    print "Nothing to try, changelist is empty."
    return

  # Grab the current revision by default. That fixes the issue when a user does
  # a gcl upload and a gcl commit right away.
  if options.revision == 0:
    options.revision = None
  elif not options.revision:
    svn_url = gcl.GetSVNFileInfo(os.getcwd())["Repository Root"]
    options.revision = int(gcl.GetSVNFileInfo(svn_url)["Revision"])

  try:
    # Convert options.diff into the content of the diff.
    if options.url:
      options.diff = urllib.urlopen(options.url).read()
    elif options.issue and options.patchset:
      # Retrieve the patch from Rietveld.
      # TODO(maruel): Actually, the user don't need to be signed in to grab the
      # patch but SendToRietveld() enforces this.
      path = "/download/issue%s_%s.diff" % (options.issue, options.patchset)
      options.diff = gcl.SendToRietveld(path)
    elif options.diff:
      options.diff = gcl.ReadFile(options.diff)
    else:
      if not options.files:
        parser.error('Please specify some files!')
      # Generate the diff with svn and write it to the submit queue path.  The
      # files are relative to the repository root, but we need patches relative
      # to one level up from there (i.e., 'src'), so adjust both the file
      # paths and the root of the diff.
      source_root = GetSourceRoot()
      prefix = PathDifference(source_root, gcl.GetRepositoryRoot())
      adjusted_paths = [os.path.join(prefix, x) for x in options.files]
      options.diff = GenerateDiff(adjusted_paths, root=source_root)

    # Send the patch. Defaults to HTTP.
    if not options.use_svn or options.use_http:
      patch_name = _SendChangeHTTP(options)
    else:
      patch_name = _SendChangeSVN(options)
    print 'Patch \'%s\' sent to try server.' % patch_name
    if patch_name == 'Unnamed':
      print "Note: use --name NAME to change the try's name."
  except InvalidScript, e:
    if swallow_exception:
      return
    print e
  except NoTryServerAccess, e:
    if swallow_exception:
      return
    print e

if __name__ == "__main__":
  TryChange(sys.argv)
 