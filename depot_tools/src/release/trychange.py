#!/usr/bin/python
#
# Copyright 2008 Google Inc.
#
# Thunk to locate the tryserver script and execute it.

import getpass
import os
import traceback

# Constants
HELP_STRING = ("Sorry, I couldn't retrieve or execute the tryserver script. "
               "Tryserver is not available.")
SCRIPT_PATH = os.path.join('src', 'tools', 'tryserver', 'tryserver.py')


#globals
checkout_root = ''


class ScriptNotFound(Exception): pass


class InvalidScript(Exception): 
  def __init__(self, script):
    self.script = script

  def __str__(self):
    return self.script + '\n' + HELP_STRING


def ReadFile(filename):
  """Returns the contents of a file."""
  file = open(filename, 'r')
  result = file.read()
  file.close()
  return result


def WriteFile(filename, contents):
  """Overwrites the file with the given contents."""
  file = open(filename, 'w')
  file.write(contents)
  file.close()


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
        raise ScriptNotFound(HELP_STRING)
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
    exec(ReadFile(script_path), script_locals)
  except Exception, e:
    #traceback.print_exception(e.__class__, e, None)
    traceback.print_exc()
    raise InvalidScript(script_path)
  return script_locals


def TryChange(change_info_name, diff):
  """Send a change to the try server."""
  script_locals = ExecuteTryServerScript()
  patch_name = getpass.getuser() + '-' + change_info_name + '.diff'
  patch_path = os.path.join(script_locals['try_server'], patch_name)
  WriteFile(patch_path, diff)
  return patch_name
