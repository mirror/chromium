#!/usr/bin/env python
# Copyright 2017 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import argparse
import json
import logging
import os
import pipes
import posixpath
import re
import shlex
import sys

import devil_chromium
from devil import devil_env
from devil.android import apk_helper
from devil.android import device_errors
from devil.android import device_utils
from devil.android import flag_changer
from devil.android.sdk import adb_wrapper
from devil.android.sdk import intent
from devil.android.sdk import version_codes
from devil.utils import run_tests_helper

with devil_env.SysPath(os.path.join(os.path.dirname(__file__), '..', '..',
                                    'third_party', 'colorama', 'src')):
  import colorama

from incremental_install import installer
from pylib import constants


def _Colorize(color, text):
  # |color| as a string to avoid pylint's no-member warning :(.
  # pylint: disable=no-member
  return getattr(colorama.Fore, color) + text + colorama.Fore.RESET


def _InstallApk(devices, apk, install_dict):
  def install(device):
    if install_dict:
      installer.Install(device, install_dict, apk=apk)
    else:
      device.Install(apk)

  logging.info('Installing %sincremental apk.', '' if install_dict else 'non-')
  device_utils.DeviceUtils.parallel(devices).pMap(install)


def _UninstallApk(devices, install_dict, package_name):
  def uninstall(device):
    if install_dict:
      installer.Uninstall(device, package_name)
    else:
      device.Uninstall(package_name)
  device_utils.DeviceUtils.parallel(devices).pMap(uninstall)


def _LaunchUrl(devices, input_args, device_args_file, url, apk):
  if input_args and device_args_file is None:
    raise Exception('This apk does not support any flags.')
  if url:
    view_activity = apk.GetViewActivityName()
    if not view_activity:
      raise Exception('APK does not support launching with URLs.')

  def launch(device):
    # The flags are first updated with input args.
    changer = flag_changer.FlagChanger(device, device_args_file)
    flags = []
    if input_args:
      flags = shlex.split(input_args)
    changer.ReplaceFlags(flags)
    # Then launch the apk.
    if url is None:
      # Simulate app icon click if no url is present.
      cmd = ['monkey', '-p', apk.GetPackageName(), '-c',
             'android.intent.category.LAUNCHER', '1']
      device.RunShellCommand(cmd, check_return=True)
    else:
      launch_intent = intent.Intent(action='android.intent.action.VIEW',
                                    activity=view_activity, data=url,
                                    package=apk.GetPackageName())
      device.StartActivity(launch_intent)
  device_utils.DeviceUtils.parallel(devices).pMap(launch)


def _ChangeFlags(devices, input_args, device_args_file):
  if input_args is None:
    _DisplayArgs(devices, device_args_file)
  else:
    flags = shlex.split(input_args)
    def update(device):
      flag_changer.FlagChanger(device, device_args_file).ReplaceFlags(flags)
    device_utils.DeviceUtils.parallel(devices).pMap(update)


def _TargetCpuToTargetArch(target_cpu):
  if target_cpu == 'x64':
    return 'x86_64'
  if target_cpu == 'mipsel':
    return 'mips'
  return target_cpu


def _RunGdb(device, package_name, output_directory, target_cpu, extra_args,
            verbose):
  gdb_script_path = os.path.dirname(__file__) + '/adb_gdb'
  cmd = [
      gdb_script_path,
      '--package-name=%s' % package_name,
      '--output-directory=%s' % output_directory,
      '--adb=%s' % adb_wrapper.AdbWrapper.GetAdbPath(),
      '--device=%s' % device.serial,
      # Use one lib dir per device so that changing between devices does require
      # refetching the device libs.
      '--pull-libs-dir=/tmp/adb-gdb-libs-%s' % device.serial,
  ]
  # Enable verbose output of adb_gdb if it's set for this script.
  if verbose:
    cmd.append('--verbose')
  if target_cpu:
    cmd.append('--target-arch=%s' % _TargetCpuToTargetArch(target_cpu))
  cmd.extend(extra_args)
  logging.warning('Running: %s', ' '.join(pipes.quote(x) for x in cmd))
  print _Colorize('YELLOW', 'All subsequent output is from adb_gdb script.')
  os.execv(gdb_script_path, cmd)


def _PrintPerDeviceOutput(devices, results, single_line=False):
  for d, result in zip(devices, results):
    if not single_line and d is not devices[0]:
      sys.stdout.write('\n')
    sys.stdout.write(
          _Colorize('YELLOW', '%s (%s):' % (d, d.build_description)))
    sys.stdout.write(' ' if single_line else '\n')
    yield result


def _RunMemUsage(devices, package_name):
  def mem_usage_helper(d):
    ret = []
    proc_map = d.GetPids(package_name)
    for name, pids in proc_map.iteritems():
      for pid in pids:
        ret.append((name, pid, d.GetMemoryUsageForPid(pid)))
    return ret

  parallel_devices = device_utils.DeviceUtils.parallel(devices)
  all_results = parallel_devices.pMap(mem_usage_helper).pGet(None)
  for result in _PrintPerDeviceOutput(devices, all_results):
    if not result:
      print 'No processes found.'
    else:
      for name, pid, usage in sorted(result):
        print '%s(%s):' % (name, pid)
        for k, v in sorted(usage.iteritems()):
          print '    %s=%d' % (k, v)
        print


def _DuHelper(device, path_spec, run_as=None):
  """Runs "du -s -k |path_spec|" on |device| and returns parsed result.

  Args:
    device: A DeviceUtils instance.
    path_spec: The list of paths to run du on. May contain shell expansions
        (will not be escaped).
    run_as: Package name to run as, or None to run as shell user. If not None
        and app is not android:debuggable (run-as fails), then command will be
        run as root.

  Returns:
    A dict of path->size in kb containing all paths in |path_spec| that exist on
    device. Paths that do not exist are silently ignored.
  """
  # Example output for: du -s -k /data/data/org.chromium.chrome/{*,.*}
  # 144     /data/data/org.chromium.chrome/cache
  # 8       /data/data/org.chromium.chrome/files
  # <snip>
  # du: .*: No such file or directory

  # The -d flag works differently across android version, so use -s instead.
  cmd_str = 'du -s -k ' + path_spec
  lines = device.RunShellCommand(cmd_str, run_as=run_as, shell=True,
                                 check_return=False)
  output = '\n'.join(lines)
  # run-as: Package 'com.android.chrome' is not debuggable
  if output.startswith('run-as:'):
    # check_return=False needed for when some paths in path_spec do not exist.
    lines = device.RunShellCommand(cmd_str, as_root=True, shell=True,
                                   check_return=False)
  ret = {}
  try:
    for line in lines:
      # du: .*: No such file or directory
      if line.startswith('du:'):
        continue
      size, subpath = line.split(None, 1)
      ret[subpath] = int(size)
    return ret
  except ValueError:
    logging.error('Failed to parse du output:\n%s', output)


def _RunDiskUsage(devices, package_name, verbose):
  # Measuring dex size is a bit complicated:
  # https://source.android.com/devices/tech/dalvik/jit-compiler
  #
  # For KitKat and below:
  #   dumpsys package contains:
  #     dataDir=/data/data/org.chromium.chrome
  #     codePath=/data/app/org.chromium.chrome-1.apk
  #     resourcePath=/data/app/org.chromium.chrome-1.apk
  #     nativeLibraryPath=/data/app-lib/org.chromium.chrome-1
  #   To measure odex:
  #     ls -l /data/dalvik-cache/data@app@org.chromium.chrome-1.apk@classes.dex
  #
  # For Android L and M (and maybe for N+ system apps):
  #   dumpsys package contains:
  #     codePath=/data/app/org.chromium.chrome-1
  #     resourcePath=/data/app/org.chromium.chrome-1
  #     legacyNativeLibraryDir=/data/app/org.chromium.chrome-1/lib
  #   To measure odex:
  #     # Option 1:
  #  /data/dalvik-cache/arm/data@app@org.chromium.chrome-1@base.apk@classes.dex
  #  /data/dalvik-cache/arm/data@app@org.chromium.chrome-1@base.apk@classes.vdex
  #     ls -l /data/dalvik-cache/profiles/org.chromium.chrome
  #         (these profiles all appear to be 0 bytes)
  #     # Option 2:
  #     ls -l /data/app/org.chromium.chrome-1/oat/arm/base.odex
  #
  # For Android N+:
  #   dumpsys package contains:
  #     dataDir=/data/user/0/org.chromium.chrome
  #     codePath=/data/app/org.chromium.chrome-UuCZ71IE-i5sZgHAkU49_w==
  #     resourcePath=/data/app/org.chromium.chrome-UuCZ71IE-i5sZgHAkU49_w==
  #     legacyNativeLibraryDir=/data/app/org.chromium.chrome-GUID/lib
  #     Instruction Set: arm
  #       path: /data/app/org.chromium.chrome-UuCZ71IE-i5sZgHAkU49_w==/base.apk
  #       status: /data/.../oat/arm/base.odex[status=kOatUpToDate, compilation_f
  #       ilter=quicken]
  #     Instruction Set: arm64
  #       path: /data/app/org.chromium.chrome-UuCZ71IE-i5sZgHAkU49_w==/base.apk
  #       status: /data/.../oat/arm64/base.odex[status=..., compilation_filter=q
  #       uicken]
  #   To measure odex:
  #     ls -l /data/app/.../oat/arm/base.odex
  #     ls -l /data/app/.../oat/arm/base.vdex (optional)
  #   To measure the correct odex size:
  #     cmd package compile -m speed org.chromium.chrome  # For webview
  #     cmd package compile -m speed-profile org.chromium.chrome  # For others
  def disk_usage_helper(d):
    package_output = '\n'.join(d.RunShellCommand(
        ['dumpsys', 'package', package_name], check_return=True))
    # Prints a message but does not return error when apk is not installed.
    if 'Unable to find package:' in package_output:
      return None
    # Ignore system apks.
    idx = package_output.find('Hidden system packages:')
    if idx != -1:
      package_output = package_output[:idx]

    try:
      data_dir = re.search(r'dataDir=(.*)', package_output).group(1)
      code_path = re.search(r'codePath=(.*)', package_output).group(1)
      lib_path = re.search(r'(?:legacyN|n)ativeLibrary(?:Dir|Path)=(.*)',
                           package_output).group(1)
    except AttributeError:
      raise Exception('Error parsing dumpsys output: ' + package_output)
    compilation_filters = set()
    # Match "compilation_filter=value", where a line break can occur at any spot
    # (refer to examples above).
    awful_wrapping = r'\s*'.join('compilation_filter=')
    for m in re.finditer(awful_wrapping + r'([\s\S]+?)[\],]', package_output):
      compilation_filters.add(re.sub(r'\s+', '', m.group(1)))
    compilation_filter = ','.join(sorted(compilation_filters))

    data_dir_sizes = _DuHelper(d, '%s/{*,.*}' % data_dir, run_as=package_name)
    # Measure code_cache separately since it can be large.
    code_cache_sizes = {}
    code_cache_dir = next(
        (k for k in data_dir_sizes if k.endswith('/code_cache')), None)
    if code_cache_dir:
      data_dir_sizes.pop(code_cache_dir)
      code_cache_sizes = _DuHelper(d, '%s/{*,.*}' % code_cache_dir,
                                   run_as=package_name)

    apk_path_spec = code_path
    if not apk_path_spec.endswith('.apk'):
      apk_path_spec += '/*.apk'
    apk_sizes = _DuHelper(d, apk_path_spec)
    if lib_path.endswith('/lib'):
      # Shows architecture subdirectory.
      lib_sizes = _DuHelper(d, '%s/{*,.*}' % lib_path)
    else:
      lib_sizes = _DuHelper(d, lib_path)

    # Look at all possible locations for odex files.
    odex_paths = []
    for apk_path in apk_sizes:
      mangled_apk_path = apk_path[1:].replace('/', '@')
      apk_basename = posixpath.basename(apk_path)[:-4]
      for ext in ('dex', 'odex', 'vdex', 'art'):
        # Easier to check all architectures than to determine active ones.
        for arch in ('arm', 'arm64', 'x86', 'x86_64', 'mips', 'mips64'):
          odex_paths.append(
              '%s/oat/%s/%s.%s' % (code_path, arch, apk_basename, ext))
          # No app could possibly have more than 6 dex files.
          for suffix in ('', '2', '3', '4', '5'):
            odex_paths.append('/data/dalvik-cache/%s/%s@classes%s.%s' % (
                arch, mangled_apk_path, suffix, ext))
            # This path does not have |arch|, so don't repeat it for every arch.
            if arch == 'arm':
              odex_paths.append('/data/dalvik-cache/%s@classes%s.dex' % (
                  mangled_apk_path, suffix))

    odex_sizes = _DuHelper(d, ' '.join(pipes.quote(p) for p in odex_paths))

    return (data_dir_sizes, code_cache_sizes, apk_sizes, lib_sizes, odex_sizes,
            compilation_filter)

  def print_sizes(desc, sizes):
    print '%s: %dkb' % (desc, sum(sizes.itervalues()))
    if verbose:
      for path, size in sorted(sizes.iteritems()):
        print '    %s: %skb' % (path, size)

  parallel_devices = device_utils.DeviceUtils.parallel(devices)
  all_results = parallel_devices.pMap(disk_usage_helper).pGet(None)
  for result in _PrintPerDeviceOutput(devices, all_results):
    if not result:
      print 'APK is not installed.'
      continue

    (data_dir_sizes, code_cache_sizes, apk_sizes, lib_sizes, odex_sizes,
     compilation_filter) = result
    total = sum(sum(sizes.itervalues()) for sizes in result[:-1])

    print_sizes('Apk', apk_sizes)
    print_sizes('App Data (non-code cache)', data_dir_sizes)
    print_sizes('App Data (code cache)', code_cache_sizes)
    print_sizes('Native Libs', lib_sizes)
    show_warning = compilation_filter and 'speed' not in compilation_filter
    compilation_filter = compilation_filter or 'n/a'
    print_sizes('odex (compilation_filter=%s)' % compilation_filter, odex_sizes)
    if show_warning:
      logging.warning('For a more realistic odex size, run:')
      logging.warning('    %s compile-dex [speed|speed-profile]', sys.argv[0])
    print 'Total: %skb (%.1fmb)' % (total, total / 1024.0)


def _RunPs(devices, package_name):
  parallel_devices = device_utils.DeviceUtils.parallel(devices)
  all_pids = parallel_devices.GetPids(package_name).pGet(None)
  for proc_map in _PrintPerDeviceOutput(devices, all_pids):
    if not proc_map:
      print 'No processes found.'
    else:
      for name, pids in sorted(proc_map.items()):
        print name, ','.join(pids)


def _RunShell(devices, package_name, cmd):
  if cmd:
    parallel_devices = device_utils.DeviceUtils.parallel(devices)
    outputs = parallel_devices.RunShellCommand(
        cmd, run_as=package_name).pGet(None)
    for output in _PrintPerDeviceOutput(devices, outputs):
      for line in output:
        print line
  else:
    adb_path = adb_wrapper.AdbWrapper.GetAdbPath()
    cmd = [adb_path, '-s', devices[0].serial, 'shell']
    # Pre-N devices do not support -t flag.
    if devices[0].build_version_sdk >= version_codes.NOUGAT:
      cmd += ['-t', 'run-as', package_name]
    else:
      print 'Upon entering the shell, run:'
      print 'run-as', package_name
      print
    os.execv(adb_path, cmd)


def _RunCompileDex(devices, package_name, compilation_filter):
  cmd = ['cmd', 'package', 'compile', '-f', '-m', compilation_filter,
         package_name]
  parallel_devices = device_utils.DeviceUtils.parallel(devices)
  outputs = parallel_devices.RunShellCommand(cmd).pGet(None)
  for output in _PrintPerDeviceOutput(devices, outputs):
    for line in output:
      print line


# TODO(agrieve):add "--all" in the MultipleDevicesError message and use it here.
def _GenerateMissingAllFlagMessage(devices):
  devices_obj = device_utils.DeviceUtils.parallel(devices)
  descriptions = devices_obj.pMap(lambda d: d.build_description).pGet(None)
  msg = ('More than one device available. Use --all to select all devices, '
         'or use --device to select a device by serial.\n\nAvailable '
         'devices:\n')
  for d, desc in zip(devices, descriptions):
    msg += '  %s (%s)\n' % (d, desc)
  return msg


def _DisplayArgs(devices, device_args_file):
  def flags_helper(d):
    changer = flag_changer.FlagChanger(d, device_args_file)
    return changer.GetCurrentFlags()

  parallel_devices = device_utils.DeviceUtils.parallel(devices)
  outputs = parallel_devices.pMap(flags_helper).pGet(None)
  print 'Existing flags per-device (via /data/local/tmp/%s):' % device_args_file
  for flags in _PrintPerDeviceOutput(devices, outputs, single_line=True):
    quoted_flags = ' '.join(pipes.quote(f) for f in flags)
    print quoted_flags or 'No flags set.'


def _DeviceCachePath(device, output_directory):
  file_name = 'device_cache_%s.json' % device.serial
  return os.path.join(output_directory, file_name)


def _LoadDeviceCaches(devices, output_directory):
  if not output_directory:
    return
  for d in devices:
    cache_path = _DeviceCachePath(d, output_directory)
    if os.path.exists(cache_path):
      logging.debug('Using device cache: %s', cache_path)
      with open(cache_path) as f:
        d.LoadCacheData(f.read())
      # Delete the cached file so that any exceptions cause it to be cleared.
      os.unlink(cache_path)
    else:
      logging.debug('No cache present for device: %s', d)


def _SaveDeviceCaches(devices, output_directory):
  if not output_directory:
    return
  for d in devices:
    cache_path = _DeviceCachePath(d, output_directory)
    with open(cache_path, 'w') as f:
      f.write(d.DumpCacheData())
      logging.info('Wrote device cache: %s', cache_path)


class _Command(object):
  def __init__(self, name, description, needs_package_name=False,
               needs_output_directory=False, needs_apk_path=False,
               supports_incremental=False, accepts_command_line_flags=False,
               accepts_args=False, accepts_url=False,
               all_devices_by_default=False, custom_arg=None):
    self.name = name
    self.description = description
    self.needs_package_name = needs_package_name
    self.needs_output_directory = needs_output_directory
    self.needs_apk_path = needs_apk_path
    self.supports_incremental = supports_incremental
    self.accepts_command_line_flags = accepts_command_line_flags
    self.accepts_args = accepts_args
    self.accepts_url = accepts_url
    self.all_devices_by_default = all_devices_by_default
    self.custom_arg = custom_arg
    self._parser = None
    self.apk_helper = None
    self.install_dict = None

  def RegisterArgs(self, from_wrapper_script, parser):
    subp = parser.add_parser(self.name, help=self.description)
    self._parser = subp
    subp.add_argument('--all',
                        action='store_true',
                        default=self.all_devices_by_default,
                        help='Operate on all connected devices.',)
    subp.add_argument('-d',
                        '--device',
                        action='append',
                        default=[],
                        dest='devices',
                        help='Target device for script to work on. Enter '
                             'multiple times for multiple devices.')
    subp.add_argument('-v',
                        '--verbose',
                        action='count',
                        default=0,
                        dest='verbose_count',
                        help='Verbose level (multiple times for more)')
    group = subp.add_argument_group('%s arguments' % self.name)

    if self.needs_package_name:
      # Always gleaned from apk when using wrapper scripts.
      group.add_argument('--package-name',
          help=argparse.SUPPRESS if from_wrapper_script else (
              'App\'s package name.'))

    if self.needs_apk_path or self.needs_package_name:
      group.add_argument('--apk-path',
          required=self.needs_apk_path and not from_wrapper_script,
          help=argparse.SUPPRESS if from_wrapper_script else 'Path to .apk')

    # Do not support incremental install outside the context of wrapper scripts.
    if not from_wrapper_script:
      self.supports_incremental = False
    if self.supports_incremental:
      group.add_argument('--incremental',
                          action='store_true',
                          default=False,
                          help='Always install an incremental apk.')
      group.add_argument('--non-incremental',
                          action='store_true',
                          default=False,
                          help='Always install a non-incremental apk.')
    if self.accepts_command_line_flags:
      group.add_argument('--args', help='Command-line flags.')
    elif self.accepts_args:
      group.add_argument('--args', help='Extra arguments.')
    if self.accepts_url:
      group.add_argument('url', nargs='?', help='A URL to launch with.')
    if self.custom_arg:
      self.custom_arg(group)

    group.add_argument('--incremental-json', help=argparse.SUPPRESS)

    suppress = from_wrapper_script or not self.accepts_command_line_flags
    group.add_argument('--command-line-flags-file-name',
        help=argparse.SUPPRESS if suppress else (
            'Name of the command-line flags file'))

    group.add_argument('--target-cpu', help=argparse.SUPPRESS)


  def ProcessArgs(self, args, output_directory=None):
    devices = device_utils.DeviceUtils.HealthyDevices(
        device_arg=args.devices,
        enable_device_files_cache=bool(output_directory),
        default_retries=0)
    _LoadDeviceCaches(devices, output_directory)
    calls_exec = self.name in ('gdb', 'logcat') or (
        self.name == 'shell' and not args.cmd)

    try:
      if len(devices) > 1:
        if calls_exec:
          self._parser.error(device_errors.MultipleDevicesError(devices))
        if not args.all and not args.devices:
          self._parser.error(_GenerateMissingAllFlagMessage(devices))
    except:
      _SaveDeviceCaches(devices, output_directory)
      raise

    if self.supports_incremental:
      if args.incremental and args.non_incremental:
        self._parser.error('Invalid to use --incremental and --non-incremental')
      elif args.non_incremental:
        if not args.apk_path:
          self._parser.error('Apk has not been built.')
        args.incremental_json = None
      elif args.incremental:
        if not args.incremental_json:
          self._parser.error('Incremental apk has not been built.')
        args.apk_path = None

      if args.apk_path and args.incremental_json:
        self._parser.error('Both incremental and non-incremental apks exist. '
                           'Select using --incremental or --non-incremental')
      elif not args.apk_path and not args.incremental_json:
        self._parser.error(
            'Neither incremental nor non-incremental apk is built.')

    if self.needs_apk_path or args.apk_path:
      if args.incremental_json:
        with open(args.incremental_json) as f:
          self.install_dict = json.load(f)
        self.apk_helper = apk_helper.ToHelper(
            os.path.join(output_directory, self.install_dict['apk_path']))
      else:
        self.apk_helper = apk_helper.ToHelper(args.apk_path)

    if self.needs_package_name and not args.package_name:
      if self.apk_helper:
        args.package_name = self.apk_helper.GetPackageName()
      else:
        self._parser.error('One of --package-name or --apk-path is required.')

    # Save cache now if command will not get a chance to afterwards.
    if calls_exec:
      _SaveDeviceCaches(devices, output_directory)

    return devices


_COMMANDS = [
    _Command('install', 'Installs the APK to one or more devices.',
             needs_apk_path=True, supports_incremental=True),
    _Command('uninstall', 'Removes the APK to one or more devices.',
             needs_package_name=True),
    # TODO(agrieve): Launch could be changed to require only package name by
    #     parsing "dumpsys package" for launch & view activities.
    _Command('launch',
             'Sends a launch intent for the APK after first writing the '
             'command-line flags file.',
             needs_apk_path=True, accepts_command_line_flags=True,
             accepts_url=True, all_devices_by_default=True),
    _Command('run', 'Install and then launch.',
             needs_apk_path=True, supports_incremental=True,
             needs_package_name=True, accepts_command_line_flags=True,
             accepts_url=True),
    _Command('stop', 'Force-stops the app.',
             needs_package_name=True, all_devices_by_default=True),
    _Command('clear-data', 'Clears all app data.',
             needs_package_name=True, all_devices_by_default=True),
    _Command('argv', 'Display and optionally update command-line flags file.',
             needs_package_name=True, accepts_command_line_flags=True,
             all_devices_by_default=True),
    _Command('gdb', 'Runs //build/android/adb_gdb with apk-specific args.',
             needs_package_name=True, needs_output_directory=True,
             accepts_args=True),
    _Command('logcat', 'Runs "adb logcat"'),
    _Command('ps', 'Show PIDs of any APK processes currently running.',
             needs_package_name=True, all_devices_by_default=True),
    _Command('disk-usage',
             'Show how much device storage is being consumed by the app.',
             needs_package_name=True, all_devices_by_default=True),
    _Command('mem-usage',
             'Show memory usage of currently running APK processes.',
             needs_package_name=True, all_devices_by_default=True),
    _Command('shell',
             'Same as "adb shell <command>", but runs as the apk\'s uid (via '
             'run-as). Useful for inspecting the app\'s data directory.',
             needs_package_name=True,
             custom_arg=(
                 lambda g: g.add_argument('cmd', nargs=argparse.REMAINDER,
                                          help='Command to run.'))),
    _Command('compile-dex',
             'Applicable only for Android N+. Forces .odex files to be '
             'compiled with the given compilation filter. To see existing '
             'filter, use "disk-usage" command.',
             needs_package_name=True,
             all_devices_by_default=True,
             # Allow only the most useful subset of filters.
             custom_arg=(
                 lambda g: g.add_argument(
                     'compilation_filter',
                     choices=['verify', 'quicken', 'space-profile', 'space',
                              'speed-profile', 'speed'],
                     help='For WebView/Monochrome, use "speed". '
                          'For other apks, use "speed-profile".'))),
]


def _PerformCommand(command, devices, args, output_directory=None,
                    target_cpu=None):
  # Dispatch.
  if command.name == 'install':
    _InstallApk(devices, command.apk_helper, command.install_dict)
  elif command.name == 'uninstall':
    _UninstallApk(devices, command.install_dict, args.package_name)
  elif command.name == 'launch':
    _LaunchUrl(devices, args.args, args.command_line_flags_file,
               args.url, command.apk_helper)
  elif command.name == 'run':
    logging.warning('Installing...')
    _InstallApk(devices, command.apk_helper, command.install_dict)
    logging.warning('Sending launch intent...')
    _LaunchUrl(devices, args.args, args.command_line_flags_file,
               args.url, command.apk_helper)
  elif command.name == 'stop':
    device_utils.DeviceUtils.parallel(devices).ForceStop(args.package_name)
  elif command.name == 'clear-data':
    device_utils.DeviceUtils.parallel(devices).ClearApplicationState(
        args.package_name)
  elif command.name == 'argv':
    _ChangeFlags(devices, args.args, args.command_line_flags_file)
  elif command.name == 'gdb':
    extra_args = shlex.split(args.args or '')
    _RunGdb(devices[0], args.package_name, output_directory, target_cpu,
            extra_args, bool(args.verbose_count))
  elif command.name == 'logcat':
    adb_path = adb_wrapper.AdbWrapper.GetAdbPath()
    cmd = [adb_path, '-s', devices[0].serial, 'logcat']
    os.execv(adb_path, cmd)
  elif command.name == 'mem-usage':
    _RunMemUsage(devices, args.package_name)
  elif command.name == 'disk-usage':
    _RunDiskUsage(devices, args.package_name, bool(args.verbose_count))
  elif command.name == 'ps':
    _RunPs(devices, args.package_name)
  elif command.name == 'shell':
    _RunShell(devices, args.package_name, args.cmd)
  elif command.name == 'compile-dex':
    _RunCompileDex(devices, args.package_name, args.compilation_filter)

  # Save back to the cache.
  _SaveDeviceCaches(devices, output_directory)


def _ParseArgs(from_wrapper_script):
  parser = argparse.ArgumentParser()
  subparsers = parser.add_subparsers(title='Supported commands', dest='command')
  for command in _COMMANDS:
    if from_wrapper_script or not command.needs_output_directory:
      command.RegisterArgs(from_wrapper_script, subparsers)

  # Show extended help when no command is passed.
  argv = sys.argv[1:]
  if not argv:
    argv = ['--help']

  args = parser.parse_args(argv)
  run_tests_helper.SetLogLevel(args.verbose_count)
  command = next(x for x in _COMMANDS if x.name == args.command)
  return command, args


# TODO(agrieve): Remove =None from target_cpu on or after October 2017.
#     It exists only so that stale wrapper scripts continue to work.
def Run(output_directory, apk_path, incremental_install_json_path,
        command_line_flags_file, target_cpu=None):
  colorama.init()
  constants.SetOutputDirectory(output_directory)
  devil_chromium.Initialize(output_directory=output_directory)
  command, args = _ParseArgs(True)

  args.command_line_flags_file = command_line_flags_file
  if apk_path and os.path.exists(apk_path):
    args.apk_path = apk_path
  if (incremental_install_json_path and
      os.path.exists(incremental_install_json_path)):
    args.incremental_json = incremental_install_json_path

  devices = command.ProcessArgs(args, output_directory=output_directory)
  _PerformCommand(command, devices, args, output_directory=output_directory,
                  target_cpu=target_cpu)


def main():
  colorama.init()
  devil_chromium.Initialize()
  command, args = _ParseArgs(False)
  devices = command.ProcessArgs(args)
  _PerformCommand(command, devices, args)


if __name__ == '__main__':
  sys.exit(main())
