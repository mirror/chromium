#!/usr/bin/python
# Copyright (c) 2006-2008 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Functions specific to build slaves, shared by several buildbot scripts.
"""

import os
import re
import signal
import subprocess
import sys
import tempfile
import time


import chromium_utils


# Local errors.
class PageHeapError(Exception): pass


# Cache the path to gflags.exe.
_gflags_exe = None

def SubversionExe():
  # TODO(pamg): move this into platform_utils to support Mac and Linux.
  if sys.platform in ['cygwin', 'win32']:
    return 'svn.bat' # Find it in the user's path.
  elif sys.platform in ['linux', 'linux2', 'darwin']:
    return 'svn' # Find it in the user's path.
  else:
    raise chromium_utils.NotImplemented(
          'Platform "%s" is not currently supported.' % sys.platform)


def SubversionRevision(wc_dir):
  """Finds the last svn revision of a working copy dir by running 'svn info',
  and returns it as an integer.
  """
  svn_regexp = re.compile(r'.*Revision: (\d+).*', re.DOTALL)
  svn_info = chromium_utils.GetCommandOutput([SubversionExe(), 'info', wc_dir])
  return int(re.sub(svn_regexp, r'\1', svn_info))


def SlaveBuildName(chrome_dir):
  """Extracts the build name of this slave (e.g., 'chrome-release') from the
  leaf subdir of its build directory.
  """
  return os.path.basename(SlaveBaseDir(chrome_dir))


def SlaveBaseDir(chrome_dir):
  """Finds the full path to the build slave's base directory (e.g.
  'c:/b/chrome/chrome-release').  This is assumed to be the parent of the
  shallowest 'build' directory in the chrome_dir path.

  Raises chromium_utils.PathNotFound if there is no such directory.
  """
  result = ''
  prev_dir = ''
  curr_dir = chrome_dir
  while prev_dir != curr_dir:
    (parent, leaf) = os.path.split(curr_dir)
    if leaf == 'build':
      # Remember this one and keep looking for something shallower.
      result = parent
    prev_dir = curr_dir
    curr_dir = parent
  if not result:
    raise chromium_utils.PathNotFound('Unable to find slave base dir above %s' %
                                    chrome_dir)
  return result


def GetStagingDir(start_dir):
  """Creates a chrome_staging dir in the starting directory. and returns its
  full path.
  """
  staging_dir = os.path.join(SlaveBaseDir(start_dir), 'chrome_staging')
  chromium_utils.MaybeMakeDirectory(staging_dir)
  return staging_dir


def SetPageHeap(chrome_dir, exe, enable):
  """Enables or disables page-heap checking in the given executable, depending
  on the 'enable' parameter.  gflags_exe should be the full path to gflags.exe.
  """
  global _gflags_exe
  if _gflags_exe is None:
    _gflags_exe = chromium_utils.FindUpward(chrome_dir,
                                          'tools', 'memory', 'gflags.exe')
  command = [_gflags_exe]
  if enable:
    command.extend(['/p', '/enable', exe, '/full'])
  else:
    command.extend(['/p', '/disable', exe])
  result = chromium_utils.RunCommand(command)
  if result:
    description = {True: 'enable', False:'disable'}
    raise PageHeapError('Unable to %s page heap for %s.' %
                        (description[enable], exe))


def LongSleep(secs):
  """A sleep utility for long durations that avoids appearing hung.

  Sleeps for the specified duration.  Prints output periodically so as not to
  look hung in order to avoid being timed out.  Since this function is meant
  for long durations, it assumes that the caller does not care about losing a
  small amount of precision.

  Args:
    secs: The time to sleep, in seconds.
  """
  secs_per_iteration = 60
  time_slept = 0

  # Make sure we are dealing with an integral duration, since this function is
  # meant for long-lived sleeps we don't mind losing floating point precision.
  secs = int(round(secs))

  remainder = secs % secs_per_iteration
  if remainder > 0:
    time.sleep(remainder)
    time_slept += remainder
    sys.stdout.write('.')
    sys.stdout.flush()

  while time_slept < secs:
    time.sleep(secs_per_iteration)
    time_slept += secs_per_iteration
    sys.stdout.write('.')
    sys.stdout.flush()

  sys.stdout.write('\n')


def _XvfbPidFilename(slave_build_name):
  """Returns the filename to the Xvfb pid file.  This name is unique for each
  builder. This is used by the linux builders."""
  return os.path.join(tempfile.gettempdir(),
                      'xvfb-' + slave_build_name  + '.pid')


def StartVirtualX(slave_build_name):
  """Start a virtual X server and set the DISPLAY environment variable so sub
  processes will use the virtual X server.  This only works on linux and
  assumes that xvfb is installed."""
  # We use a pid file to make sure we don't have any xvfb processes running
  # from a previous test run.
  StopVirtualX(slave_build_name)

  # Start a virtual X server that we run the tests in.  This makes it so we can
  # run the tests even if we didn't start the tests from an X session.
  devnull = open("/dev/null", "w")
  proc = subprocess.Popen(["Xvfb", ":9", "-screen", "0", "1024x768x24", "-ac"],
                          stdout=devnull, stderr=devnull)
  xvfb_pid_filename = _XvfbPidFilename(slave_build_name)
  open(xvfb_pid_filename, 'w').write(str(proc.pid))
  os.environ['DISPLAY'] = ":9"

def StopVirtualX(slave_build_name):
  """Try and stop the virtual X server if one was started with StartVirtualX.
  If a virtual x server is not running, this method does nothing."""
  xvfb_pid_filename = _XvfbPidFilename(slave_build_name)
  if os.path.exists(xvfb_pid_filename):
    try:
      # If the process doesn't exist, we raise an exception that we can ignore.
      os.kill(open(xvfb_pid_filename).read(), signal.SIGKILL)
    except:
      pass
    os.remove(xvfb_pid_filename)
