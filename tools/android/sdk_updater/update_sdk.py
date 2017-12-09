#!/usr/bin/env python
#
# Copyright 2017 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""This script downloads / packages & uploads Android SDK packages.

   It could be run when we need to update sdk packages to latest version.
   It has 2 usages:
     1) download: downloading a new version of the SDK via sdkmanager
     2) package: wrapping SDK directory into CIPD-compatible packages and
                 uploading the new packages via CIPD to server.

   Both downloading and uploading allows to either specify a package, or
   deal with default packages (build-tools, platform-tools, platforms and
   tools).

   Example usage:
     1) updating default packages:
        $ update_sdk.py download
        $ update_sdk.py package
     2) updating a specified package:
        $ update_sdk.py download -p build-tools;27.0.1
        $ update_sdk.py package -p build-tools --version 27.0.1

   Note that `package` could update the package argument to the checkout
   version in .gn file //build/config/android/config.gni. If having git
   changes, please prepare to upload a CL that updates the SDK version.
"""

import argparse
import fileinput
import os
import re
import subprocess
import sys


_SRC_ROOT = os.path.realpath(
    os.path.join(os.path.dirname(__file__), '..', '..', '..'))

_SDK_ROOT = os.path.join(_SRC_ROOT, 'third_party', 'android_sdk', 'public')

# TODO(shenghuazhang): Update sdkmanager path when gclient can download SDK
# via CIPD: crug/789809
_SDKMANAGER_PATH = os.path.join(_SRC_ROOT, 'third_party', 'android_tools',
                                'sdk', 'tools', 'bin', 'sdkmanager')

_ANDROID_CONFIG_GNI_PATH = os.path.join(_SRC_ROOT, 'build', 'config',
                                        'android', 'config.gni')

_TOOLS_LIB_PATH = os.path.join(_SDK_ROOT, 'tools', 'lib')

_DEFAULT_DOWNLOAD_PACKAGES = [
  'build-tools',
  'platform-tools',
  'platforms',
  'tools'
]

_DEFAULT_PACKAGES_DICT = {
  'build-tools': 'build-tools;27.0.1',
  'platforms': 'platforms;android-27',
  'sources': 'sources;android-26',
}

_GN_ARGUMENTS_TO_UPDATE = {
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
      print 'Coercing %s to %s' % (pkg, _DEFAULT_PACKAGES_DICT[pkg])
      pkg = _DEFAULT_PACKAGES_DICT[pkg]

    download_sdk_cmd = [
        _SDKMANAGER_PATH,
        '--install',
        '--sdk_root=%s' % arguments.sdk_root,
        pkg
    ]
    if arguments.verbose:
      download_sdk_cmd.append('--verbose')

    subprocess.check_call(download_sdk_cmd)


def _FindPackageVersion(package):
  """Find sdk package version

  Two options for package version:
    1) Use the version in name if package name contains ';version'
    2) For simple name package, search its version from 'Installed packages'
       via `sdkmanager --list`
  """
  sdkmanager_list_cmd = [
      _SDKMANAGER_PATH,
      '--list'
  ]
  if package in _DEFAULT_PACKAGES_DICT:
  # get the version after ';' from package name
    package = _DEFAULT_PACKAGES_DICT[package]
    return package.split(';')[1]
  else:
  # get the version via `sdkmanager --list`
    output = subprocess.check_output(sdkmanager_list_cmd)
    for line in output.splitlines():
      # if re.search('\s%s\s' % package, line):
      if package in line:
      # if found package path, catch its version which in the first '|...|'
        return line.split('|')[1].strip()
      if line == '\n': # Reaches the end of 'Installed packages' list
        raise Exception('Cannot find the version of package %s' % package)


def _ReplaceVersionInFile(file_path, pattern, version):
  """Replace the version of sdk package argument in file.

  Args:
    file_path: Path to the file to update the version of sdk package argument.
    pattern: Pattern for the sdk package argument. Must contain a capture
             group for the argument line excluding the version.
    version: The new version of the package.
  """
  for line in fileinput.input(file_path, inplace=1):
    line = re.sub(pattern, r'\g<1>%s\n' % version, line)
    sys.stdout.write(line)
  print 'Updated the package version to %s in file %s.' % (version, file_path)


def UploadSdkPackage(service_url, packages, yaml_file, version, verbose):
  """Build and upload a package instance file to CIPD.

  This would also update gn and ensure files to the package version as
  uploading to CIPD.

  Args:
    service_url: The url of the CIPD service.
    packages: The packages to be uploaded to CIPD.
    yaml_file: Path to the yaml file that defines what to put into the package.
               Default as //third_pary/android_sdk/public/cipd_*.yaml
    version: The version of the package instance.
    verbose: Enable more logging.
  """
  for package in packages:
    if not yaml_file:
      yaml_file = os.path.join(_SDK_ROOT, 'cipd_' + package + '.yaml')
      if not os.path.exists(yaml_file):
        raise IOError('Cannot find .yaml file for package %s' % packages)

    pkg_version = version
    if not pkg_version:
      pkg_version = _FindPackageVersion(package)
    upload_sdk_cmd = [
      'cipd', 'create',
      '-pkg-def', yaml_file,
      '-tag', 'version:%s' % pkg_version,
      '-service-url', service_url
    ]

    if verbose:
      upload_sdk_cmd.extend(['-log-level', 'debug'])

    subprocess.check_call(upload_sdk_cmd)

    # Change the sdk package version in file //build/config/android/config.gni
    if package in _GN_ARGUMENTS_TO_UPDATE:
      version_config_name = _GN_ARGUMENTS_TO_UPDATE.get(package)
      # Regex to parse the line of sdk package version gn argument, e.g.
      # '  default_android_sdk_version = "27"'. Capture a group for the line
      # excluding the version.
      pattern = re.compile(
          # Match the argument with '=' and whitespaces. Capture a group for it.
          r'(^\s*%s\s*=\s*)' % version_config_name +
          # version number with double quote. E.g. "27", "27.0.1", "-26.0.0-dev"
          r'[-\w\s."]+'
          # End of string
          r'$'
      )
      # Remove all chars except for digits and dots in version
      pkg_version = re.sub(r'[^\d\.]','', pkg_version)
      if package == 'tools':
      # Check jar names with "-dev" in tools/lib.
      # If any, change version name of 'tools'.
        tools_lib_jars_list = os.listdir(_TOOLS_LIB_PATH)
        dev_file_pattern = re.compile(
            r'-'       # '-' in front
            r'[\d\.]+' # Only digits and dots
            r'-dev'    # '-dev' following
        )
        for file_name in tools_lib_jars_list:
          found = dev_file_pattern.findall(file_name)
          if found:
            pkg_version = found[0]
            break
      _ReplaceVersionInFile(_ANDROID_CONFIG_GNI_PATH,
                            pattern, '"' + pkg_version + '"')


def _UploadSdkPackage(arguments):
  packages = arguments.package
  if not packages:
    packages = _DEFAULT_DOWNLOAD_PACKAGES
    if arguments.version or arguments.yaml_file:
      raise IOError('Don\'t use --version/--yaml-file for default packages.')

  UploadSdkPackage(arguments.service_url, packages,
                   arguments.yaml_file, arguments.version, arguments.verbose)


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
      nargs=1,
      help='The package to be uploaded to CIPD. Note that package ' +
           'name is a simple path e.g. "platforms" or "build-tools" ' +
           'which matches package name on CIPD service. Default by ' +
           'build-tools, platform-tools, platforms and tools')
  package_parser.add_argument(
      '--version',
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
