#!/usr/bin/env python
# Copyright 2017 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import argparse
import errno
import logging
import os
import re
import shutil
import sys
import tempfile

import devil_chromium
from devil.utils import cmd_helper
from pylib.utils import logging_utils

from pylib.constants import host_paths


def _MakeDirsIfAbsent(path):
  try:
    os.makedirs(path)
  except OSError as err:
    if err.errno != errno.EEXIST or not os.path.isdir(path):
      raise


class MavenDownloader:
  '''
  Downloads and installs the requested artifacts from the Google Maven repo.
  The artifacts are expected to be specified in the format
  "group_id:artifact_id:version:file_type", as the default file type is JAR
  but most Android libraries are provided as AARs, which would otherwise fail
  downloading. See Install()
  '''

  # Template for a Maven settings.xml which instructs Maven to download to the
  # given directory.
  MAVEN_SETTINGS_TEMPLATE = '''\
  <settings
      xmlns="http://maven.apache.org/SETTINGS/1.0.0"
      xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance"
      xsi:schemaLocation="http://maven.apache.org/SETTINGS/1.0.0
                          https://maven.apache.org/xsd/settings-1.0.0.xsd" >
    <localRepository>{}</localRepository>
  </settings>
  '''

  # Default Maven repository.
  DEFAULT_DOWNLOAD_CACHE = os.path.join(os.getenv("HOME"), '.m2', 'repository')

  def __init__(self):
    self.download_cache = MavenDownloader.DEFAULT_DOWNLOAD_CACHE
    self.settings_path = None

  def SetCustomSettings(self, directory):
    self.download_cache = directory
    self.settings_path = os.path.join(directory, 'settings.xml')
    _MakeDirsIfAbsent(directory)
    with open(self.settings_path, 'w') as settings:
      settings.write(MavenDownloader.MAVEN_SETTINGS_TEMPLATE.format(directory))

  def Install(self, target_repo, artifacts):
    for artifact in artifacts:
      logging.debug('Processing %s', artifact)
      download_relpath = self.DownloadArtifact(artifact)
      logging.debug('Downloaded.')
      install_path = self.ImportArtifact(target_repo, download_relpath)
      logging.debug('Installed to %s', install_path)

  def DownloadArtifact(self, artifact):
    '''
    Downloads |artifact| using maven, to its standard location. It can be
    modified by using custom settings, see SetCustomSettings()
    '''
    parts = artifact.split(':')
    assert len(parts) == 4, ('Artifacts expected as'
                             '"group_id:artifact_id:version:file_type".')
    group_id, artifact_id, version, file_type = parts

    cmd = ['mvn', 'org.apache.maven.plugins:maven-dependency-plugin:2.1:get',
           '-DrepoUrl=https://maven.google.com', '-Dartifact=' + artifact]
    if self.settings_path:
      cmd.extend(['--global-settings', self.settings_path])
      logging.debug('Downloaded %s', artifact)

    try:
      ret_code = cmd_helper.RunCmd(cmd)
      assert ret_code == 0
    except OSError as e:
      if e.errno == os.errno.ENOENT:
        raise AssertionError('mvn command not found. Please install Maven.')
      else:
        raise

    return os.path.join(os.path.join(*group_id.split('.')),
                        artifact_id,
                        version,
                        '{}-{}.{}'.format(artifact_id, version, file_type))

  def ImportArtifact(self, target_repo, artifact_path):
    dst_dir = os.path.join(target_repo, os.path.dirname(artifact_path))
    _MakeDirsIfAbsent(dst_dir)
    shutil.copy(os.path.join(self.download_cache, artifact_path), dst_dir)
    return dst_dir


if __name__ == '__main__':
  logging.basicConfig(level=logging.DEBUG)
  logging_utils.ColorStreamHandler.MakeDefault()

  # TODO(dgn): This should be more like a library, and have a dedicated script
  # for updating the support library, e.g.
  # tools/android/roll/update_support_library.py
  support_lib_repo = os.path.join(host_paths.DIR_SOURCE_ROOT, 'third_party',
                                  'android_tools', 'sdk', 'extras', 'android',
                                  'm2repository')
  try:
    MavenDownloader().Install(support_lib_repo, [
        'android.arch.core:common:1.0.0:jar',
        'android.arch.lifecycle:common:1.0.0:jar',
        'android.arch.lifecycle:runtime:1.0.0:aar',
        'com.android.support:animated-vector-drawable:26.1.0:aar',
        'com.android.support:appcompat-v7:26.1.0:aar',
        'com.android.support:cardview-v7:26.1.0:aar',
        'com.android.support:design:26.1.0:aar',
        'com.android.support:gridlayout-v7:26.1.0:aar',
        'com.android.support:leanback-v17:26.1.0:aar',
        'com.android.support:mediarouter-v7:26.1.0:aar',
        'com.android.support:palette-v7:26.1.0:aar',
        'com.android.support:preference-leanback-v17:26.1.0:aar',
        'com.android.support:preference-v14:26.1.0:aar',
        'com.android.support:preference-v7:26.1.0:aar',
        'com.android.support:recyclerview-v7:26.1.0:aar',
        'com.android.support:support-annotations:26.1.0:jar',
        'com.android.support:support-compat:26.1.0:aar',
        'com.android.support:support-core-ui:26.1.0:aar',
        'com.android.support:support-core-utils:26.1.0:aar',
        'com.android.support:support-fragment:26.1.0:aar',
        'com.android.support:support-media-compat:26.1.0:aar',
        'com.android.support:support-v13:26.1.0:aar',
        'com.android.support:support-vector-drawable:26.1.0:aar',
        'com.android.support:transition:26.1.0:aar',
    ])
  except AssertionError as err:
    # Handle known errors and print something decent instead of a trace.
    logging.error(err.message)
    exit(-1)
