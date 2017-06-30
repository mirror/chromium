#!/usr/bin/env python
#
# Copyright 2017 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import argparse
import logging
import sys

import devil_chromium
from devil.android import device_errors
from devil.android import device_utils


def RunApkOperation(args, gn_args):
  command = args.command
  if command == 'install':
    _InstallApk(args, gn_args)


def _InstallApk(args, gn_args):
  def install(device):
    try:
      device.Install(gn_args.apk_path)
    except (device_errors.CommandFailedError,
            device_errors.DeviceUnreachableError):
      logging.exception('Failed to install %s', args.apk_path)
    except device_errors.CommandTimeoutError:
      logging.exception('Timed out while installing %s', gn_args.apk_path)

  # This will raise a NoDevicesError if no device is attached.
  devices = device_utils.DeviceUtils.HealthyDevices(default_retries=0,
                                                    device_arg=args.devices)
  device_num = len(devices)
  if device_num == 1:
    install(devices[0])
  elif args.all:
    device_utils.DeviceUtils.parallel(devices).pMap(install)
  else:
    raise device_errors.MultipleDevicesError(devices)


def _AddInstallOptions(parser):
  """Adds install options to the parser"""
  parser = parser.add_argument_group("install arguments")
  parser.add_argument('-a', '--all',
                      help='Install all connected devices.',
                      action='store_true')
  parser.add_argument('-d', '--device', dest='devices', action='append',
                      default=[],
                      help='Target device for apk to install on. Enter '
                      'multiple times for multiple devices.')


def main():
  # This is used to parse args passed by the gn template.
  gn_parser = argparse.ArgumentParser()
  gn_parser.add_argument('--apk-to-install', dest='apk_path',
                         help='Path or name of the apk to install')
  gn_args, sub_argv = gn_parser.parse_known_args()

  parser = argparse.ArgumentParser()
  command_parsers = parser.add_subparsers(title='Apk operations',
                                          dest='command')
  subp = command_parsers.add_parser('install', help='Install the specific apk.')
  _AddInstallOptions(subp)
  args = parser.parse_args(sub_argv)
  RunApkOperation(args, gn_args)


if __name__ == '__main__':
  sys.exit(main())
