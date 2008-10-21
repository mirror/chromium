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
import traceback

# Constants
HELP_STRING = ("Sorry, I couldn't retrieve or execute the tryserver script. "
               "Tryserver is not available.")
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
    return self.args[0] + ' is unaccessible.\n' + HELP_STRING


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
  if not 'try_server' in script_locals or not script_locals['try_server']:
    raise InvalidScript(script_path + ": 'try_server' variable was not found.")
  return script_locals


def EscapeDot(name):
  return name.replace('.', '-')


def _SendChangeNFS(try_name, diff, bot):
  """Send a change to the try server."""
  script_locals = ExecuteTryServerScript()
  patch_name = EscapeDot(getpass.getuser()) + '.' + EscapeDot(try_name)
  if bot:
    patch_name += '.' + EscapeDot(bot)
  patch_name += '.diff'
  patch_path = os.path.join(script_locals['try_server'], patch_name)
  try:
    gcl.WriteFile(patch_path, diff)
  except IOError, e:
    raise NoTryServerAccess(script_locals['try_server'])
  return patch_name


def TryChange(argv, name='Unnamed', file_list=None, swallow_exception=False,
              patchset=None):
  # Parse argv
  parser = optparse.OptionParser(usage="%prog [options]")
  parser.add_option("-n", "--name", default=name,
                    help="Name of the try change.")
  parser.add_option("-f", "--file_list", default=file_list,
                    help="List of files to include in the try.")
  parser.add_option("-b", "--bot", default=None,
                    help="Force a specific build bot.")
  parser.add_option("-p", "--patchset", default=patchset,
                    help="Define the Rietveld's patchset id.")
  options, args = parser.parse_args(argv)
  if not options.file_list:
    print "Nothing to try, changelist is empty."
    return

  try:
    # Change the current working directory before generating the diff so that it
    # shows the correct base.
    os.chdir(gcl.GetRepositoryRoot())

    # Generate the diff and write it to the submit queue path. Fix the file list
    # according to the new path.
    subpath = PathDifference(GetCheckoutRoot(), os.getcwd())
    os.chdir(GetCheckoutRoot())
    diff = gcl.GenerateDiff([os.path.join(subpath, x)
                             for x in options.file_list])
    patch_name = _SendChangeNFS(options.name, diff, options.bot)
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
