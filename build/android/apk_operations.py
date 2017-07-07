#!/usr/bin/env python
#
# Copyright 2017 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import argparse
import imp
import logging
import pipes
import os
import sys

import devil_chromium
from devil.android import apk_helper
from devil.android import device_utils
from devil.android import flag_changer
from devil.android.sdk import intent
from devil.utils import run_tests_helper

from pylib import constants


def InstallApk(install_incremental, inc_install_script, devices_obj,
               apk_to_install):
  if install_incremental:
    helper = apk_helper.ApkHelper(apk_to_install)
    try:
      install_wrapper = imp.load_source('install_wrapper', inc_install_script)
    except IOError:
      raise Exception('Incremental install script not found: %s\n' %
                      inc_install_script)
    params = install_wrapper.GetInstallParameters()

    def install_incremental_apk(device):
      from incremental_install import installer
      installer.Install(device, helper, split_globs=params['splits'],
                        native_libs=params['native_libs'],
                        permissions=None)
    devices_obj.pMap(install_incremental_apk)
    return
  # Install the regular apk on devices.
  def install(device):
    device.Install(apk_to_install)
  devices_obj.pMap(install)


def LaunchURL(devices_obj, input_args, device_args_file, url, apk_to_launch):
  if input_args is not None and device_args_file is None:
    raise Exception("This apk does not support any flags.")
  apk_package = apk_helper.GetPackageName(apk_to_launch)

  def launch(device):
    # The flags are first updated with input args.
    changer = flag_changer.FlagChanger(device, device_args_file)
    flags = []
    if input_args is not None:
      flags = input_args.split()
    changer.ReplaceFlags(flags)

    # Then launch the apk.
    if url is None:
      # Simulate app icon click if no url is present.
      cmd = ['monkey', '-p', apk_package, '-c',
             'android.intent.category.LAUNCHER', '1']
      device.RunShellCommand(cmd, check_return=True)
    else:
      launch_intent = intent.Intent(
          action='android.intent.action.VIEW', package=apk_package, data=url)
      device.StartActivity(launch_intent)
  devices_obj.pMap(launch)


def StopApk(devices_obj, apk_to_stop):
  apk_pkg = apk_helper.GetPackageName(apk_to_stop)
  devices_obj.ForceStop(apk_pkg)


def ChangeFlags(devices, devices_obj, input_args, device_args_file):
  if input_args is None:
    _DisplayArgs(devices, device_args_file)
  else:
    flags = input_args.split()
    def update(device):
      flag_changer.FlagChanger(device, device_args_file).ReplaceFlags(flags)
    devices_obj.pMap(update)


# TODO(Yipengw):add "--all" in the MultipleDevicesError message and use it here.
def SetAllErrorMsg(devices, devices_obj):
  descriptions = devices_obj.pMap(lambda d: d.build_description).pGet(None)
  msg = ('More than one device available. Use --all to select all devices, '
         'or use --device to select a device by serial.\n\nAvailable '
         'devices:\n')
  for d, desc in zip(devices, descriptions):
    msg += '  %s (%s)\n' % (d, desc)
  return msg


def _DisplayArgs(devices, device_args_file):
  print 'Current flags (in {%s}):' % device_args_file
  for d in devices:
    changer = flag_changer.FlagChanger(d, device_args_file)
    flags = changer.GetCurrentFlags()
    if flags:
      quoted_flags = ' '.join(pipes.quote(f) for f in sorted(flags))
    else:
      quoted_flags = '( empty )'
    print '  %s (%s): %s' % (d, d.build_description, quoted_flags)


def AddCommonOptions(parser):
  parser.add_argument('--all',
                      action='store_true',
                      default=False,
                      help='Operate on all connected devices.',)
  parser.add_argument('--device',
                      action='append',
                      default=[],
                      dest='devices',
                      help='Target device for script to work on. Enter '
                           'multiple times for multiple devices.')
  parser.add_argument('--incremental',
                      action='store_true',
                      default=False,
                      help='Always install an incremental apk.')
  parser.add_argument('-v',
                      '--verbose',
                      action='count',
                      default=0,
                      dest='verbose_count',
                      help='Verbose level (multiple times for more)')


def AddLaunchOptions(parser):
  parser = parser.add_argument_group("launch arguments")
  parser.add_argument('url',
                      nargs='?',
                      help='The URL this command launches.')
  parser.add_argument('--args',
                      help='Flags to set in the flags file for the apk. '
                           'Available only for android_apk targets that set '
                           '"command_line_flags_file".')


def AddArgvOptions(parser):
  parser = parser.add_argument_group("argv arguments")
  parser.add_argument('--args',
                      nargs='?',
                      help='The flags set by the user.')


def _DeviceCachePath(device):
  file_name = 'device_cache_%s.json' % device.adb.GetDeviceSerial()
  return os.path.join(constants.GetOutDirectory(), file_name)


def main():
  # This is used to parse args passed by the gn template.
  gn_parser = argparse.ArgumentParser()
  gn_parser.add_argument('--apk-path',
                         help='The path to the apk.')
  gn_parser.add_argument('--incremental-apk-path',
                         help='The path to the incremental apk.')
  gn_parser.add_argument('--incremental-install-script',
                         help='The path to the incremental install script.')
  gn_parser.add_argument('--output-directory', required=True,
                         help='Path to the directory where build files are'
                         ' located.')

  gn_parser.add_argument('--command-line-flags-file',
                         help='The file storing flags on the device.')

  gn_args, sub_argv = gn_parser.parse_known_args()
  if gn_args.output_directory:
    constants.SetOutputDirectory(gn_args.output_directory)

  parser = argparse.ArgumentParser()
  command_parsers = parser.add_subparsers(title='Apk operations',
                                          dest='command')
  subp = command_parsers.add_parser('install', help='Install the specific apk.')
  AddCommonOptions(subp)

  subp = command_parsers.add_parser('launch',
                                    help='Launches the apk and optionally '
                                    'includes a URL in the intent.')
  AddCommonOptions(subp)
  AddLaunchOptions(subp)

  subp = command_parsers.add_parser('run', help='Install and launch.')
  AddCommonOptions(subp)
  AddLaunchOptions(subp)

  subp = command_parsers.add_parser('stop', help='Stop chrome on all devices')
  AddCommonOptions(subp)

  subp = command_parsers.add_parser('argv', help='Dispaly flags on all devices')
  AddCommonOptions(subp)
  AddArgvOptions(subp)

  args = parser.parse_args(sub_argv)
  run_tests_helper.SetLogLevel(args.verbose_count)
  command = args.command
  devices = device_utils.DeviceUtils.HealthyDevices(device_arg=args.devices,
                                                    default_retries=0)
  devices_obj = device_utils.DeviceUtils.parallel(devices)

  if command in {'argv', 'stop'} or len(args.devices) > 0:
    args.all = True
  if len(devices) > 1 and not args.all:
    raise Exception(SetAllErrorMsg(devices, devices_obj))

  # Use the cache if possible.
  for d in devices:
    cache_path = _DeviceCachePath(d)
    if os.path.exists(cache_path):
      logging.info('Using device cache: %s', cache_path)
      with open(cache_path) as f:
        d.LoadCacheData(f.read())
      # Delete the cached file so that any exceptions cause it to be cleared.
      os.unlink(cache_path)
    else:
      logging.info('No cache present in the device: %s', d)

  apk_path = gn_args.apk_path
  if apk_path is not None and not os.path.exists(apk_path):
    apk_path = None

  inc_apk_path = gn_args.incremental_apk_path
  if inc_apk_path is not None and not os.path.exists(inc_apk_path):
    inc_apk_path = None
  inc_install_script = gn_args.incremental_install_script

  if args.incremental and inc_apk_path is None:
    raise Exception("No incremental apk is available.")
  install_incremental = False
  active_apk = apk_path
  if apk_path is None or args.incremental or (
      os.path.getmtime(apk_path) < os.path.getmtime(inc_apk_path)):
    install_incremental = True
    active_apk = inc_apk_path
    logging.info('Use the incremental apk')

  print 'use incremental apk: %s' % install_incremental

  if command == 'install':
    InstallApk(install_incremental, inc_install_script, devices_obj, active_apk)
  elif command == 'launch':
    LaunchURL(devices_obj, args.args, gn_args.command_line_flags_file,
              args.url, active_apk)
  elif command == 'run':
    InstallApk(install_incremental, inc_install_script, devices_obj, active_apk)
    LaunchURL(devices_obj, args.args, gn_args.command_line_flags_file,
              args.url, active_apk)
  elif command == 'stop':
    StopApk(devices_obj, active_apk)
  elif command == 'argv':
    ChangeFlags(devices, devices_obj, args.args,
                gn_args.command_line_flags_file)

  # Save back to the cache.
  for d in devices:
    cache_path = _DeviceCachePath(d)
    with open(cache_path, 'w') as f:
      f.write(d.DumpCacheData())
      logging.info('Wrote device cache: %s', cache_path)


if __name__ == '__main__':
  sys.exit(main())
