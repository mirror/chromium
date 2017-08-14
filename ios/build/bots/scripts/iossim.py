# Copyright 2017 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Simulator test runner for iOS."""

import argparse
import json
import os
import plistlib
import subprocess
import select
import sys
import tempfile

def PrintDeviceHome(device_name, sdk_version):
  simctl_list = _GetSimulatorList()
  udid = _GetDeviceBySDKAndName(simctl_list, device_name, sdk_version)
  home_dir = subprocess.check_output(['xcrun', 'simctl', 'getenv', udid, 'HOME'])
  return home_dir

def WipeDevice(device_name, sdk_version):
  simctl_list = _GetSimulatorList()
  udid = _GetDeviceBySDKAndName(simctl_list, device_name, sdk_version)
  _WipeDeviceByUdid(udid)

def RunTestOnSimulator(application_path, xctest_path, device_name, sdk_version, 
                       environment_variables, cmd_args, tests_filter):
  xctestrun_file_path = _GenerateXCTestRunFile(application_path, xctest_path, 
                                               environment_variables, cmd_args, 
                                               tests_filter)

  simctl_list = _GetSimulatorList()
  udid = _GetDeviceBySDKAndName(simctl_list, device_name, sdk_version)
  test_cmd = ['xcrun', 'xcodebuild', '-xctestrun', xctestrun_file_path, 
    '-destination', 'platform=iOS Simulator,id=' + udid, 
    'test-without-building']

  process = subprocess.Popen(test_cmd, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
  stdout, stderr = process.communicate()
  status = process.wait()

  stdout = ''.join(stdout).split('\n')
  stderr = ''.join(stderr).split('\n')

  for stderr_line in stderr:
    if 'IDETestOperationsObserverErrorDomain' in stderr_line:
      continue
    if '** TEST EXECUTE FAILED **' in stderr_line:
      continue

    print stderr_line
    sys.stdout.flush()
  
  for stdout_line in stdout:
    print stdout_line
    sys.stdout.flush()

  return status, stdout

def _GenerateXCTestRunFile(application_path, xctest_path, environment_variables, 
                           cmd_args, tests_filter):
  testing_environment_variables = {}
  testing_environment_variables['IDEiPhoneInternalTestBundleName'] = \
      os.path.basename(application_path)
  framework_path = \
      '__PLATFORMS__/iPhoneSimulator.platform/Developer/Library/Frameworks'
  testing_environment_variables['DYLD_FRAMEWORK_PATH'] = framework_path
  library_path = \
      '__PLATFORMS__/iPhoneSimulator.platform/Developer/Library'
  testing_environment_variables['DYLD_LIBRARY_PATH'] = library_path
  if xctest_path:
    inject_library_path = \
        '__PLATFORMS__/iPhoneSimulator.platform/Developer/Library/PrivateFrameworks/IDEBundleInjection.framework/IDEBundleInjection'
    testing_environment_variables['DYLD_INSERT_LIBRARIES'] = inject_library_path

  test_target_name = {}
  test_target_name['TestHostPath'] = application_path
  if xctest_path:
    test_target_name['TestBundlePath'] = xctest_path    
  else:
    test_target_name['TestBundlePath'] = application_path

  if len(environment_variables) > 0:
    dict_environment_variables = {}
    for env in environment_variables:
      key = env.split('=')[0]
      value = env.split('=')[1]
      dict_environment_variables[key] = value

    test_target_name['EnvironmentVariables'] = dict_environment_variables

  if cmd_args:
    test_target_name['CommandLineArguments'] = cmd_args

  if len(tests_filter) > 0:
    test_target_name['OnlyTestIdentifiers'] = tests_filter

  test_target_name['TestingEnvironmentVariables'] = testing_environment_variables
  xctestrun = {}
  xctestrun['TestTargetName'] = test_target_name

  with tempfile.NamedTemporaryFile(delete=False) as temp_file:
    plistlib.writePlist(xctestrun, temp_file)

  return temp_file.name

def _WipeDeviceByUdid(udid):
  subprocess.call(['xcrun', 'simctl', 'shutdown', udid])
  subprocess.call(['xcrun', 'simctl', 'erase', udid])

def _GetDeviceBySDKAndName(simctl_list, device_name, sdk_version):
  sdk = "iOS " + str(sdk_version)
  if not sdk in simctl_list['devices']:
    return None

  devices = simctl_list['devices'][sdk]
  for device in devices:
    if device['name'] == device_name:
      return device['udid']

  return None

def _KillSimulator():
  try:
    subprocess.check_call(['xcrun', 'killall',
        'com.apple.CoreSimulator.CoreSimulatorService', 'Simulator'])
  except subprocess.CalledProcessError as e:
    if e.returncode != 1:
      # Ignore a 1 exit code (which means there were no simulators to kill).
      raise

def _GetDefaultSDKVersion(simctl_list):
  runtimes = _GetAvailableiOSRuntimes(simctl_list)
  default_sdk = 0.0
  for runtime in runtimes:
    default_sdk = max(default_sdk, float(runtime['version']))

  return default_sdk

def _PrintSupportedDevices(simctl_list):
  print '\niOS devices:\n'
  for device_type in _GetiOSDeviceTypes(simctl_list):
    print device_type['name'] + '\n'

  print '\nRuntimes\n'
  for runtime in _GetAvailableiOSRuntimes(simctl_list):
    print runtime['version'] + '\n'

def _GetiOSDeviceTypes(simctl_list):
  device_tyepes = []
  for device_type in simctl_list['devicetypes']:
    is_ipad = device_type['identifier'].startswith(
        'com.apple.CoreSimulator.SimDeviceType.iPad')
    is_iphone = device_type['identifier'].startswith(
        'com.apple.CoreSimulator.SimDeviceType.iPhone')
    if is_ipad or is_iphone:
      device_tyepes.append(device_type)

def _GetAvailableiOSRuntimes(simctl_list):
  runtimes = []
  for runtime in simctl_list['runtimes']:
    if runtime['availability'] != '(available)':
      continue
    if not runtime['identifier'].startswith(
        'com.apple.CoreSimulator.SimRuntime.iOS'):
      continue

    runtimes.append(runtime)

  return runtimes

def _GetSimulatorList():
  simctl_list_str = subprocess.check_output(['xcrun', 'simctl', 'list', '-j'])
  simctl_list = json.loads(simctl_list_str)
  return simctl_list

def _InstallTestArguments(arg_parser):
  arg_parser.add_argument('-s',
                          '--sdk',
                          type=float,
                          help="Specifies the SDK version to use (e.g '9.3')."
                               "Will use system default if not specified.")

  arg_parser.add_argument('-d',
                          '--device',
                          type=str,
                          help="Specifies the device (must be one of the "
                               "values from the iOS Simulator's Hardware -> "
                               "Device menu. Defaults to 'iPhone 6s'.")

  arg_parser.add_argument('-e',
                          '--environment-variables',
                          action='append',
                          help="Specifies an environment key=value pair that "
                               "will be set in the simulated application's "
                               "environment.")

  arg_parser.add_argument('-w',
                          '--wipe',
                          action='store_true',
                          help='todo')

  arg_parser.add_argument('-t',
                          '--tests-filter',
                          action='append',
                          help="todo")

  arg_parser.add_argument('-c',
                          '--cmd-args',
                          action='append',
                          help='TODO')

  arg_parser.add_argument('-p',
                          '--print-home',
                          action='store_true',
                          help='TODO')

def Main():
  arg_parser = argparse.ArgumentParser(
      description='Run test on simulator for iOS')

  _InstallTestArguments(arg_parser)

  arg_parser.add_argument('application_path',
    help='the path to the .app directory.')
  arg_parser.add_argument('xctest_path', nargs='?',
    help='path to an optional xctest bundle.')
  args = arg_parser.parse_args()

  # When the last running simulator is from Xcode 9, an Xcode 10 run will yeild
  # a failure to "unload a stale CoreSimulatorService job" message.  Sending a
  # hidden simctl to do something simple (list devices) helpfully works around
  # this issue.
  subprocess.check_output(['xcrun', 'simctl', 'list', '-j'])

  application_path = os.path.abspath(args.application_path)
  xctest_path = os.path.abspath(args.xctest_path) if args.xctest_path else None
  simctl_list = _GetSimulatorList()

  sdk_version = _GetDefaultSDKVersion(simctl_list)
  if args.sdk:
    sdk_version = args.sdk

  device_name = 'iPhone 6s'
  if args.device:
    device_name = args.device

  if args.wipe:
    WipeDevice(device_name, sdk_version)
    return # TODO

  if args.print_home:
    PrintDeviceHome(device_name, sdk_version)
    return # TODO

  environment_variables = []
  if args.environment_variables:
    environment_variables = args.environment_variables

  cmd_args = []
  if args.cmd_args:
    for cmd_arg in args.cmd_args:
      cmd_args.append(str.lstrip(cmd_arg))

  tests_filter = []
  if args.tests_filter:
    tests_filter = args.tests_filter

  _KillSimulator()
  RunTestOnSimulator(application_path, xctest_path, device_name, sdk_version, 
                     environment_variables, cmd_args, tests_filter)
  _KillSimulator()
  
if __name__ == '__main__':
  sys.exit(Main())
