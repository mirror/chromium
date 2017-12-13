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
                 Providing '--dry-run' option to show what packages to be
                 created and uploaded without actually doing either.

   Both downloading and uploading allows to either specify a package, or
   deal with default packages (build-tools, platform-tools, platforms and
   tools).

   Example usage:
     1) updating default packages:
        $ update_sdk.py download
        (optional) $ update_sdk.py package --dry-run
        $ update_sdk.py package
     2) updating a specified package:
        $ update_sdk.py download -p build-tools;27.0.1
        (optional) $ update_sdk.py package --dry-run -p build-tools \
                     --version 27.0.1
        $ update_sdk.py package -p build-tools --version 27.0.1

   Note that `package` could update the package argument to the checkout
   version in .gn file //build/config/android/config.gni. If having git
   changes, please prepare to upload a CL that updates the SDK version.
"""

import argparse
import os
import re
import shutil
import subprocess
import sys
import tempfile


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

_COMMON_JAR_SUFFIX_PATTERN= re.compile(
    r'^common'            # file name begins with 'common'
    r'(-[\d\.]+(-dev)?)'  # group of suffix e.g.'-26.0.0-dev', '-25.3.2'
    r'\.jar$'             # ends with .jar
)


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
    # Get the version after ';' from package name
    package = _DEFAULT_PACKAGES_DICT[package]
    return package.split(';')[1]
  else:
    # Get the package version via `sdkmanager --list`. The logic is:
    # Check through 'Installed packages' which is at the first section of
    # `sdkmanager --list` output, example:
    #   Installed packages:=====================] 100% Computing updates...
    #     Path                        | Version | Description
    #     -------                     | ------- | -------
    #     build-tools;27.0.1          | 27.0.1  | Android SDK Build-Tools 27.0.1
    #     emulator                    | 26.0.3  | Android Emulator
    #     platforms;android-27        | 1       | Android SDK Platform 27
    #     tools                       | 26.1.1  | Android SDK Tools
    #
    #   Available Packages:
    #   ....
    # When found a line containing the package path, grap its version between
    # the first and second '|'. Since the 'Installed packages' list section ends
    # by the first new line, the check loop should be ended when reaches a '\n'.
    output = subprocess.check_output(sdkmanager_list_cmd)
    for line in output.splitlines():
      if package in line:
        # if found package path, catch its version which in the first '|...|'
        return line.split('|')[1].strip()
      if line == '\n': # Reaches the end of 'Installed packages' list
        raise Exception('Cannot find the version of package %s' % package)


def _CheckSameVersionInFile(file_path, pattern, version):
  """Check whether the version in file is the same as the new version

  Args:
    file_path: Path to the file to update the version of sdk package argument.
    pattern: Pattern for the sdk package argument. Must contain the second
             captured group of the version.
    version: The new version of the package.
  """
  with open(file_path, 'r') as f:
    for line in f.readlines():
      m = re.match(pattern, line)
      if m:
        origin_version = m.group(2).replace('"', '').strip()
        if origin_version == version:
          return True
  return False


def _ReplaceVersionInFile(file_path, pattern, version):
  """Replace the version of sdk package argument in file.

  Args:
    file_path: Path to the file to update the version of sdk package argument.
    pattern: Pattern for the sdk package argument. Must contain the first
             captured group for the argument line excluding the version.
    version: The new version of the package.
  """
  temp_file = tempfile.NamedTemporaryFile(delete=False)
  with open(file_path) as f:
    for line in f:
      new_line = re.sub(pattern, r'\g<1>%s\n' % version, line)
      temp_file.write(new_line)
  temp_file.close()
  shutil.move(temp_file.name, file_path)
  print 'Updated the package version to %s in file %s.' % (version, file_path)


def UploadSdkPackage(dry_run, service_url, packages, yaml_file, version,
                     verbose):
  """Build and upload a package instance file to CIPD.

  This would also update gn and ensure files to the package version as
  uploading to CIPD.

  Args:
    dry_run: Bool. To show what packages would be created and packages, without
             actually doing either.
    service_url: The url of the CIPD service.
    packages: The packages to be uploaded to CIPD.
    yaml_file: Path to the yaml file that defines what to put into the package.
               Default as //third_pary/android_sdk/public/cipd_*.yaml
    version: The version of the package instance.
    verbose: Enable more logging.
  """
  for package in packages:
    if not yaml_file:
      pkg_yaml_file = os.path.join(_SDK_ROOT, 'cipd_' + package + '.yaml')
      if not os.path.exists(pkg_yaml_file):
        raise IOError('Cannot find .yaml file for package %s' % packages)

    pkg_version = version
    if not pkg_version:
      pkg_version = _FindPackageVersion(package)

    if dry_run:
      print 'This `package` command (without -n/--dry-run) would create and',
      print 'upload the package %s version:%s to CIPD.' % (package, pkg_version)
    else:
      upload_sdk_cmd = [
        'cipd', 'create',
        '-pkg-def', pkg_yaml_file,
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
      gn_arg_pattern = re.compile(
          # Match the argument with '=' and whitespaces. Capture a group for it.
          r'(^\s*%s\s*=\s*)' % version_config_name +
          # version number with double quote. E.g. "27", "27.0.1", "-26.0.0-dev"
          r'([-\w\s."]+)'
          # End of string
          r'$'
      )
      # Remove all chars except for digits and dots in version
      pkg_version = re.sub(r'[^\d\.]','', pkg_version)

      if package == 'tools':
        pkg_version = '-%s' % pkg_version

        # Check jar file name of 'common*.jar' in tools/lib,
        # which could be 'common-<version>-dev.jar' or 'common-<version>.jar'.
        # Change 'tools' package version to the suffix if exists.
        tools_lib_jars_list = os.listdir(_TOOLS_LIB_PATH)
        for file_name in tools_lib_jars_list:
          found = re.match(_COMMON_JAR_SUFFIX_PATTERN, file_name)
          if found:
            pkg_version = found.group(1)
            break

      if not _CheckSameVersionInFile(_ANDROID_CONFIG_GNI_PATH,
                                     gn_arg_pattern, pkg_version):
        if dry_run:
          print '    Note: file %s argument %s would be updated to %s.' % (
                _ANDROID_CONFIG_GNI_PATH, version_config_name, pkg_version)
        else:
          _ReplaceVersionInFile(_ANDROID_CONFIG_GNI_PATH,
                                gn_arg_pattern, '"%s"' % pkg_version)


def _UploadSdkPackage(arguments):
  packages = arguments.package
  if not packages:
    packages = _DEFAULT_DOWNLOAD_PACKAGES
    if arguments.version or arguments.yaml_file:
      raise IOError('Don\'t use --version/--yaml-file for default packages.')

  UploadSdkPackage(arguments.dry_run, arguments.service_url, packages,
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
      '-n',
      '--dry-run',
      action='store_true',
      help='Dry run won\'t trigger creating instances or uploading packages. ' +
           'It shows what packages would be created and uploaded to CIPD. ' +
           'It also shows the possible updates of sdk version on files.')
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
