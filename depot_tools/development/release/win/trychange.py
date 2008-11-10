#!/usr/bin/python
#
# Copyright 2008 Google Inc.
#
# Locate the tryserver script and execute it. The try server script contains
# the repository-specific try server commands.

import gcl
import getpass
import optparse
import os
import sys
import traceback
import urllib


# Constants
HELP_STRING = "Sorry, Tryserver is not available."
SCRIPT_PATH = os.path.join('src', 'tools', 'tryserver', 'tryserver.py')


# Globals
checkout_root = ''


class ScriptNotFound(Exception):
  def __str__(self):
    return self.args[0] + ' was not found.\n' + HELP_STRING


class InvalidScript(Exception): 
  def __str__(self):
    return self.args[0] + '\n' + HELP_STRING


class NoTryServerAccess(Exception):
  def __str__(self):
    return self.args[0] + '\n' + HELP_STRING


def GetCheckoutRoot():
  """Retrieves the checkout root by finding the try script."""
  # Cache this value accross calls.
  global checkout_root
  if not checkout_root:
    checkout_root = os.getcwd()
    while True:
      if os.path.exists(os.path.join(checkout_root, SCRIPT_PATH)):
        break
      parent = os.path.dirname(checkout_root)
      if parent == checkout_root:
        checkout_root = ''
        raise ScriptNotFound(SCRIPT_PATH)
      checkout_root = parent
  return checkout_root


def PathDifference(root, subpath):
  """Returns the difference subpath minus root."""
  if subpath.find(root) != 0:
    return None
  # The + 1 is for the trailing / or \.
  return subpath[len(root) + len(os.sep):]


def ExecuteTryServerScript():
  """Execute the try server script and returns its dictionary."""
  script_path = os.path.join(GetCheckoutRoot(), SCRIPT_PATH)
  script_locals = {}
  try:
    exec(gcl.ReadFile(script_path), script_locals)
  except Exception, e:
    traceback.print_exc()
    raise InvalidScript(script_path + ' is invalid.')
  return script_locals


def EscapeDot(name):
  return name.replace('.', '-')


def _SendChangeNFS(options):
  """Send a change to the try server."""
  script_locals = ExecuteTryServerScript()
  if not options.nfs_path:
    # TODO(maruel): Use try_server_nfs instead.
    options.nfs_path = script_locals.get('try_server', None)
    if not options.nfs_path:
      raise NoTryServerAccess('Please use the --nfs_path option to specify the '
          'try server path to write into.')
  patch_name = EscapeDot(options.user) + '.' + EscapeDot(options.name)
  if options.bot:
    patch_name += '.' + EscapeDot(options.bot)
  patch_name += '.diff'
  patch_path = os.path.join(options.nfs_path, patch_name)
  try:
    gcl.WriteFile(patch_path, options.diff)
  except IOError, e:
    raise NoTryServerAccess('%s is unwritable.' % patch_path)
  return patch_name


def _SendChangeHTTP(options):
  """Send a change to the try server using the HTTP protocol."""
  script_locals = ExecuteTryServerScript()
  values = {}
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

  if options.email:
    values['email'] = options.email
  values['user'] = options.user
  values['name'] = options.name
  if options.bot:
    values['bot'] = options.bot
  values['patch'] = options.diff
  url = 'http://%s:%s/send_try_patch' % (options.host, options.port)
  connection = urllib.urlopen(url, urllib.urlencode(values))
  if not connection:
    raise NoTryServerAccess('%s is unaccessible.' % url)
  if connection.read() != 'OK':
    raise NoTryServerAccess('%s is unaccessible.' % url)
  return options.name


def TryChange(argv, name='Unnamed', file_list=None, swallow_exception=False,
              patchset=None):
  # Parse argv
  parser = optparse.OptionParser(usage="%prog [options]")
  parser.add_option("-n", "--name", default=name,
                    help="Name of the try change.")
  parser.add_option("-f", "--file_list", default=file_list, action="append",
                    help="List of files to include in the try.")
  parser.add_option("-b", "--bot", default=None,
                    help="Force a specific build bot.")
  parser.add_option("--use_nfs", action="store_true",
                    help="Use NFS to talk to the try server.")
  parser.add_option("--use_http", action="store_true",
                    help="Use HTTP to talk to the try server.")
  parser.add_option("--nfs_path", default=None,
                    help="NFS path to use to talk to the try server.")
  # TODO(maruel): Don't hardcode this value.
  parser.add_option("--host", default=None,
                    help="Host address to use to talk to the try server.")
  # TODO(maruel): Don't hardcode this value.
  parser.add_option("--port", default=None,
                    help="Port to use to talk to the try server.")
  parser.add_option("-p", "--patchset", default=patchset,
                    help="Define the Rietveld's patchset id.")
  parser.add_option("--diff", default=None,
                    help="Define the Rietveld's patchset id.")
  parser.add_option("--root", default=None,
                    help="Root to use for the patch.")
  parser.add_option("-u", "--user", default=getpass.getuser(),
                    help="User name to be used.")
  parser.add_option("-e", "--email", default=None,
                    help="Email address where to send the results.")
  options, args = parser.parse_args(argv)
  if not options.file_list:
    print "Nothing to try, changelist is empty."
    return

  try:
    if not options.diff:
      # Change the current working directory before generating the diff so that it
      # shows the correct base.
      os.chdir(gcl.GetRepositoryRoot())
      # Generate the diff and write it to the submit queue path. Fix the file list
      # according to the new path.
      subpath = PathDifference(GetCheckoutRoot(), os.getcwd())
      os.chdir(GetCheckoutRoot())
      options.diff = gcl.GenerateDiff([os.path.join(subpath, x)
                                      for x in options.file_list])
    # Defaults to HTTP.
    if not options.use_nfs or options.use_http:
      patch_name = _SendChangeHTTP(options)
    else:
      patch_name = _SendChangeNFS(options)
    print 'Patch \'%s\' sent to try server.' % patch_name
  except ScriptNotFound, e:
    if swallow_exception:
      return
    print e
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
 