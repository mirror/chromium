#!/usr/bin/env python
# Copyright 2018 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""
If should_use_hermetic_xcode.py emits "1", and the current toolchain is out of
date:
  * Downloads the hermetic mac/ios toolchain
    * Requires CIPD authentication. Run `cipd auth-login`, use Google account.
  * Accepts the license.
    * If xcode-select and xcodebuild are not passwordless in sudoers, requires
      user interaction.

The toolchain version can be overridden by setting IOS_TOOLCHAIN_REVISION or
MAC_TOOLCHAIN_REVISION with the full revision, e.g. 9A235.
"""

import os
import platform
import shutil
import subprocess
import sys


# This can be changed after running:
#    mac_toolchain upload -xcode-path path/to/Xcode.app
MAC_TOOLCHAIN_VERSION = '8E2002'

# The toolchain will not be downloaded if the minimum OS version is not met.
# 16 is the major version number for macOS 10.12.
MAC_MINIMUM_OS_VERSION = 16

IOS_TOOLCHAIN_VERSION = '9C40b'

MAC_TOOLCHAIN_INSTALLER = 'mac_toolchain'

# Absolute path to src/ directory.
REPO_ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))

# Absolute path to a file with gclient solutions.
GCLIENT_CONFIG = os.path.join(os.path.dirname(REPO_ROOT), '.gclient')

BASE_DIR = os.path.abspath(os.path.dirname(__file__))
TOOLCHAIN_ROOT = os.path.join(BASE_DIR, '%s_files')
TOOLCHAIN_BUILD_DIR = os.path.join(TOOLCHAIN_ROOT, 'Xcode.app')
STAMP_FILE = os.path.join(TOOLCHAIN_ROOT, 'toolchain_build_revision')


def PlatformMeetsHermeticXcodeRequirements(target_os):
  if target_os == 'ios':
    return True
  return int(platform.release().split('.')[0]) >= MAC_MINIMUM_OS_VERSION


def GetPlatforms():
  target_os = set(['mac'])
  try:
    env = {}
    execfile(GCLIENT_CONFIG, env, env)
    target_os |= set(env.get('target_os', target_os))
  except:
    pass
  return target_os


def _UseHermeticToolchain(target_os):
  current_dir = os.path.dirname(os.path.realpath(__file__))
  script_path = os.path.join(current_dir, 'mac/should_use_hermetic_xcode.py')
  proc = subprocess.Popen([script_path, target_os], stdout=subprocess.PIPE)
  return '1' in proc.stdout.readline()


def RequestCipdAuthentication():
  """Requests that the user authenticate to access Xcode CIPD packages."""

  print 'Access to Xcode CIPD package requires authentication.'
  print '-----------------------------------------------------------------'
  print
  print 'You appear to be a Googler.'
  print
  print 'I\'m sorry for the hassle, but you may need to do a one-time manual'
  print 'authentication. Please run:'
  print
  print '    cipd auth-login'
  print
  print 'and follow the instructions.'
  print
  print 'NOTE: Use your google.com credentials, not chromium.org.'
  print
  print '-----------------------------------------------------------------'
  print
  sys.stdout.flush()


def PrintError(message):
  # Flush buffers to ensure correct output ordering.
  sys.stdout.flush()
  sys.stderr.write(message + '\n')
  sys.stderr.flush()


def InstallXcode(xcode_build_version, installer_cmd, xcode_app_path, target_os):
  """Installs the requested Xcode build version.

  Args:
    xcode_build_version: (string) Xcode build version to install.
    installer_cmd: (string) Path to mac_toolchain command to install Xcode.
      See https://chromium.googlesource.com/infra/infra/+/master/go/src/infra/cmd/mac_toolchain/
    xcode_app_path: (string) Path to install the contents of Xcode.app.
    target_os: (string) 'mac' or 'ios', the kind of Xcode toolchain to install.

  Returns:
    True if installation was successful. False otherwise.
  """
  args = [
      installer_cmd, 'install',
      '-kind', target_os,
      '-xcode-version', xcode_build_version.lower(),
      '-output-dir', xcode_app_path,
  ]

  # Buildbot slaves need to use explicit credentials.
  creds = os.environ.get('MAC_TOOLCHAIN_CREDS')
  if creds:
    args.extend('--service-account-json', creds)

  try:
    subprocess.check_call(args)
  except subprocess.CalledProcessError as e:
    PrintError('Xcode build version %s failed to install: %s\n' % (
        xcode_build_version, e))
    RequestCipdAuthentication()
    return False
  except OSError as e:
    PrintError(('Xcode installer "%s" failed to execute'
                ' (not on PATH or not installed).') % installer_cmd)
    return False

  return True


def main():
  if sys.platform != 'darwin':
    return 0

  for target_os in GetPlatforms():
    if not _UseHermeticToolchain(target_os):
      print 'Skipping Mac toolchain installation for %s' % target_os
      continue

    if not PlatformMeetsHermeticXcodeRequirements(target_os):
      print 'OS version does not support toolchain.'
      continue

    if target_os == 'ios':
      toolchain_version = os.environ.get('IOS_TOOLCHAIN_REVISION',
                                          IOS_TOOLCHAIN_VERSION)
      toolchain_filename = 'ios-toolchain-%s.tgz'
    else:
      toolchain_version = os.environ.get('MAC_TOOLCHAIN_REVISION',
                                          MAC_TOOLCHAIN_VERSION)
      toolchain_filename = 'toolchain-%s.tgz'

    # On developer machines, mac_toolchain tool is provided by
    # depot_tools. On the bots, the recipe is responsible for installing
    # it and providing the path to the executable.
    installer_cmd = os.environ.get('MAC_TOOLCHAIN_INSTALLER',
                                   MAC_TOOLCHAIN_INSTALLER)

    toolchain_root = TOOLCHAIN_ROOT % target_os
    xcode_app_path = TOOLCHAIN_BUILD_DIR % target_os
    stamp_file = STAMP_FILE % target_os

    # Delete the old "hermetic" installation if detected.
    # TODO(sergeyberezin): remove this once the old "hermetic" solution is no
    # longer in use.
    if os.path.exists(stamp_file):
      print 'Detected old hermetic installation at %s. Deleting.' % (
        toolchain_root)
      shutil.rmtree(toolchain_root)

    success = InstallXcode(toolchain_version, installer_cmd, xcode_app_path,
                           target_os)
    if not success:
      return 1

  return 0


if __name__ == '__main__':
  sys.exit(main())
