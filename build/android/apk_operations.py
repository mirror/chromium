#!/usr/bin/env python
#
# Copyright 2017 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import argparse
import sys

import devil_chromium
from devil.android import apk_helper
from devil.android import device_utils
from devil.android import flag_changer
from devil.android.sdk import intent


def _InstallApk(devices, install_all, apk_to_install):
  def install(device):
    device.Install(apk_to_install)

  devices_obj = device_utils.DeviceUtils.parallel(devices)
  if len(devices) == 1:
    install(devices[0])
  elif install_all:
    devices_obj.pMap(install)
  else:
    raise Exception(_GetErrorMsg(devices, devices_obj))


def _LaunchURL(devices, input_args, device_args_file, launch_all, url,
               apk_to_install):
  apk_package = apk_helper.GetPackageName(apk_to_install)
  launch_intent = intent.Intent(
      action='android.intent.action.VIEW', package=apk_package, data=url)

  def launch(device):
    if url is None:
      # Simulate app icon click if no url is present.
      cmd = ['monkey', '-p', apk_package, '-c',
             'android.intent.category.LAUNCHER', '1']
      device.RunShellCommand(cmd, check_return=True)
    else:
      device.StartActivity(launch_intent)

  devices_obj = device_utils.DeviceUtils.parallel(devices)
  _UpdateFlags(devices_obj, input_args, device_args_file)
  if len(devices) == 1:
    launch(devices[0])
  elif launch_all:
    devices_obj.pMap(launch)
  else:
    raise Exception(_GetErrorMsg(devices, devices_obj))


def _UpdateFlags(devices_obj, input_args, device_args_file):
  def update_flags(device):
    changer = flag_changer.FlagChanger(device, device_args_file)
    flags = []
    if input_args is not None:
      flags = input_args.split()
    changer.ReplaceFlags(flags)

  devices_obj.pMap(update_flags)


def _GetErrorMsg(devices, devices_obj):
  descriptions = devices_obj.pMap(lambda d: d.build_description).pGet(None)
  msg = ('More than one device available. Use --all to action on all devices, '
         'or use --device to select a device by serial.\n\nAvailable '
         'devices:\n')
  for d, desc in zip(devices, descriptions):
    msg += '  %s (%s)\n' % (d, desc)
  return msg


def _AddCommonOptions(parser):
  """Add shared options to the parser"""

  parser.add_argument('--all', help='Operate on all connected devices.',
                      action='store_true')
  parser.add_argument('--device', dest='devices', action='append', default=[],
                      help='Target device for script to work on. Enter '
                      'multiple times for multiple devices.')


def _AddLaunchOptions(parser):
  """Add launch options to the parser"""

  parser = parser.add_argument_group("launch arguments")
  parser.add_argument('url', nargs='?',
                      help='The URL this command launches.')
  parser.add_argument('--args', help='The args passed by the user.')


def main():
  # This is used to parse args passed by the gn template.
  gn_parser = argparse.ArgumentParser()
  gn_parser.add_argument('--apk-to-install',
                         help='Path or name of the apk to install.')
  gn_parser.add_argument('--command-line-flags-file',
                         help='The file storing flags on the device.')
  gn_args, sub_argv = gn_parser.parse_known_args()

  parser = argparse.ArgumentParser()
  command_parsers = parser.add_subparsers(title='Apk operations',
                                          dest='command')
  subp = command_parsers.add_parser('install', help='Install the specific apk.')
  _AddCommonOptions(subp)

  subp = command_parsers.add_parser('launch', help='Launch a given URL.')
  _AddCommonOptions(subp)
  _AddLaunchOptions(subp)

  args = parser.parse_args(sub_argv)

  command = args.command
  devices = device_utils.DeviceUtils.HealthyDevices(device_arg=args.devices,
                                                    default_retries=0)
  if command == 'install':
    _InstallApk(devices, args.all, gn_args.apk_to_install)
  elif command == 'launch':
    _LaunchURL(devices, args.args, gn_args.command_line_flags_file, args.all,
               args.url, gn_args.apk_to_install)


if __name__ == '__main__':
  sys.exit(main())
