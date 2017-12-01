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
import os
import subprocess
import sys

_SRC_ROOT = os.path.realpath(
    os.path.join(os.path.dirname(__file__), '..', '..', '..', '..'))

_SDK_ROOT = os.path.join(_SRC_ROOT, 'third_party', 'android_sdk', 'public')

_SDKMANAGER_PATH = os.path.join(_SRC_ROOT, 'third_party', 'android_tools',
                                'sdk', 'tools', 'bin', 'sdkmanager')

_DEFAULT_DOWNLOAD_PACKAGES = [
  'build-tools;27.0.1',
  'platform-tools',
  'platforms;android-27',
  'tools'
]

_DEFAULT_PACKAGES_DICT = {
  'build-tools': 'build-tools;27.0.1',
  'platforms': 'platforms;android-27',
  'sources': 'sources;android-25',
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
        '--install', pkg,
        '--sdk_root', arguments.sdk_root
    ]
    if arguments.verbose:
      download_sdk_cmd.append('--verbose')

    p = subprocess.Popen(download_sdk_cmd,
                         stdout=subprocess.PIPE,
                         stdin=subprocess.PIPE,
                         stderr=subprocess.PIPE)
    # input 'y' when asked to accept the license
    (_, error) = p.communicate(input='y')
    if not p.returncode:
      print '\n=== Downloaded package %s successfully ===\n' % pkg
    else:
      print error
      print '\n=== Failed to download package %s ===\n' % pkg
      print 'Return code: ', p.returncode

# Upload SDK to CIPD:
# $ cipd create -pkg-def ./third_party/android_sdk/public/cipd_build-tools.yaml\
# -tag version:26.0.2

def _UploadSdkPackage(arguments):
  """Build and upload a package instance file to CIPD.

  This would also update gn and ensure files to the package version as
  uploading to CIPD.

  Args:
    service_url: The url of the CIPD service.
    yaml_file: Path to the yaml file that defines what to put into the package.
    version: The version of the package instance.
    verbose
  """
  yaml_file = arguments.yaml_file
  if not yaml_file:
    if not arguments.package:
      raise IOError('Either argument --package or --yaml-file is needed.')
    #throw exception
    yaml_file = os.path.join(_SDK_ROOT, 'cipd_' + arguments.package + '.yaml')
    if not os.path.exists(yaml_file):
      raise IOError('Cannot find .yaml file for package %s' % arguments.package)

  upload_sdk_cmd = [
    'cipd create',
    '-pkg-def', yaml_file,
    '-tag', 'version:' + argument.version,
    '-service-url', argument.service_url
  ]
  if arguments.verbose:
    upload_sdk_cmd.append('--verbose')
  p = subprocess.Popen(upload_sdk_cmd,
                       stdout=subprocess.PIPE,
                       stdin=subprocess.PIPE,
                       stderr=subprocess.PIPE)
  (_, error) = p.communicate()
  if not p.returncode:
    print '\n=== Uploaded package to CIPD successfully ===\n'
  else:
    print error
    print '\n=== Failed to upload package ===\n'
    print 'Return code: ', p.returncode


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
           'is not specified, update "build-tools;27.0.1", "tools"' +
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
      help='The package to be uploaded to CIPD (unused with ' +
           '--yaml-file). Note that package name is a simple ' +
           'path e.g. "platforms" or "build-tools" which ' +
           'matches package name on CIPD service.')
  package_parser.add_argument(
      '--version',
      required=True,
      help='Version of the uploading package instance through CIPD.')
  package_parser.add_argument(
      '--yaml-file',
      help='Path to *.yaml file that defines what to put into the package.')
  package_parser.add_argument('--service-url',
                              help='The url of the CIPD service.',
                              default='https://chrome-infra-packages.appspot.com')
  package_parser.add_argument('-v', '--verbose',
                              action='store_true',
                              help='print debug information')

  args = parser.parse_args()

  # if args.package and args.yaml_file:
  #   parser.error("--package and --yaml-file cannot be used together")

  args.func(args)


if __name__ == '__main__':
  sys.exit(main())
