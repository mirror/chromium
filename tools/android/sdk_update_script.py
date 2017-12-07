#!/usr/bin/env python
#
# Copyright 2017 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""A script to download/package/upload Android SDK packages.

   Downloading a new version of the SDK via sdkmanager
   Packaging an SDK directory into CIPD-compatible packages
   Uploading new packages via CIPD to chrome-infra-packages.appspot.com
"""

import argparse
import fileinput
import os
import re
import subprocess
import sys


_SRC_ROOT = os.path.realpath(
    os.path.join(os.path.dirname(__file__), '..', '..'))

_SDK_ROOT = os.path.join(_SRC_ROOT, 'third_party', 'android_sdk', 'public')

_SDKMANAGER_PATH = os.path.join(_SRC_ROOT, 'third_party', 'android_tools',
                                'sdk', 'tools', 'bin', 'sdkmanager')

_ANDROID_CONFIG_GNI_PATH = os.path.join(_SRC_ROOT, 'build', 'config',
                                        'android', 'config.gni')

_DEFAULT_DOWNLOAD_PACKAGES = [
  'build-tools;27.0.1',
  'platform-tools',
  'platforms;android-27',
  'tools'
]

_DEFAULT_PACKAGES_DICT = {
  'build-tools': 'build-tools;27.0.1',
  'platforms': 'platforms;android-27',
  'sources': 'sources;android-26',
}

_UPDATE_PACKAGE_IN_CONFIG_DICT = {
  'build-tools': 'default_android_sdk_build_tools_version',
  'tools': 'default_android_sdk_tools_version_suffix',
  'platforms': 'default_android_sdk_version',
}


def _DownloadSdk(arguments):
  """Download sdk package from sdkmanager.

  If package isn't provided, update build-tools, platform-tools, platforms,
  and tools.
  """
  for pkg in arguments.packages:
    # If package is not a sdk-style path, try to match a default path to it.
    if pkg in _DEFAULT_PACKAGES_DICT:
      print ('"%s" is not a sdk-style path; download the default ' % pkg +
             'package "%s" instead.' % _DEFAULT_PACKAGES_DICT[pkg])
      pkg = _DEFAULT_PACKAGES_DICT[pkg]

    download_sdk_cmd = [
        _SDKMANAGER_PATH,
        '--install',
        '--sdk_root=' + arguments.sdk_root,
        pkg
    ]
    if arguments.verbose:
      download_sdk_cmd.append('--verbose')

    subprocess.check_call(download_sdk_cmd)

def _UploadSdkPackage(arguments):
  UploadSdkPackage(arguments.service_url, arguments.package,
                   arguments.yaml_file, arguments.version, arguments.verbose)

def _ReplaceVersionInFile(file_path, pattern, version):
  for line in fileinput.input(file_path, inplace=1):
    line = re.sub(pattern, r'\g<1>%s\n' % version, line)
    sys.stdout.write(line)
  print 'Updated the package version to %s in file %s.' % (version, file_path)


def UploadSdkPackage(service_url, package, yaml_file, version, verbose):
  """Build and upload a package instance file to CIPD.

  This would also update gn and ensure files to the package version as
  uploading to CIPD.

  Args:
    service_url: The url of the CIPD service.
    package: The package to be uploaded to CIPD.
    yaml_file: Path to the yaml file that defines what to put into the package.
               Default as //third_pary/android_sdk/public/cipd_*.yaml
    version: The version of the package instance.
    verbose: Enable more logging.
  """
  if not yaml_file:
    yaml_file = os.path.join(_SDK_ROOT, 'cipd_' + package + '.yaml')
    if not os.path.exists(yaml_file):
      raise IOError('Cannot find .yaml file for package %s' % arguments.package)

  upload_sdk_cmd = [
    'cipd', 'create',
    '-pkg-def', yaml_file,
    '-tag', 'version:' + version,
    '-service-url', service_url
  ]

  if verbose:
    upload_sdk_cmd.extend(['-log-level', 'debug'])

  subprocess.check_call(upload_sdk_cmd)

  # Change the sdk package version in file //build/config/android/config.gni
  if package in _UPDATE_PACKAGE_IN_CONFIG_DICT:
    version_config_name = _UPDATE_PACKAGE_IN_CONFIG_DICT.get(package)
    pattern = re.compile(r'(^\s*%s\s*=\s*)[\w\s".-]+$' % version_config_name)
    if package == 'tools':
      version = "-" + version + "-dev"
    _ReplaceVersionInFile(_ANDROID_CONFIG_GNI_PATH,
                          pattern, '"' + version + '"')


def main():
  parser = argparse.ArgumentParser(
      description='A script to download Android SDK packages via sdkmanager ' +
                  'and upload to CIPD.')

  subparsers = parser.add_subparsers(title='commands')

  download_parser = subparsers.add_parser(
      'download',
      help='Download sdk package to the latest version from sdkmanager.')
  download_parser.set_defaults(func=_DownloadSdk)
  download_parser.add_argument(
      '-p',
      '--packages',
      nargs='+',
      default=_DEFAULT_DOWNLOAD_PACKAGES,
      help='The packages of the SDK needs to be installed/updated. ' +
           'Note that package name should be a sdk-style path e.g. ' +
           '"platforms;android-27" or "platform-tools". If package ' +
           'is not specified, update "build-tools;27.0.1", "tools" ' +
           '"platform-tools" and "platforms;android-27" by default.')
  download_parser.add_argument('--sdk-root',
                               default=_SDK_ROOT,
                               help='base path to the Android SDK root')
  download_parser.add_argument('-v', '--verbose',
                               action='store_true',
                               help='print debug information')

  package_parser = subparsers.add_parser(
      'package', help='Create and upload package instance file to CIPD.')
  package_parser.set_defaults(func=_UploadSdkPackage)
  package_parser.add_argument(
      '-p',
      '--package',
      required=True,
      help='The package to be uploaded to CIPD. Note that package ' +
           'name is a simple path e.g. "platforms" or "build-tools" ' +
           'which matches package name on CIPD service.')
  package_parser.add_argument(
      '--version',
      required=True,
      help='Version of the uploading package instance through CIPD.')
  package_parser.add_argument(
      '--yaml-file',
      help='Path to *.yaml file that defines what to put into the package.' +
           'Default as //third_pary/android_sdk/public/cipd_<package>.yaml')
  package_parser.add_argument('--service-url',
                              help='The url of the CIPD service.',
                              default='https://chrome-infra-packages.appspot.com')
  package_parser.add_argument('-v', '--verbose',
                              action='store_true',
                              help='print debug information')

  args = parser.parse_args()

  args.func(args)


if __name__ == '__main__':
  sys.exit(main())
