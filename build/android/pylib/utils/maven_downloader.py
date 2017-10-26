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

from devil.utils import cmd_helper


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
  _MVN_SETTINGS_TEMPLATE = '''\
  <settings
      xmlns="http://maven.apache.org/SETTINGS/1.0.0"
      xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance"
      xsi:schemaLocation="http://maven.apache.org/SETTINGS/1.0.0
                          https://maven.apache.org/xsd/settings-1.0.0.xsd" >
    <localRepository>{}</localRepository>
  </settings>
  '''

  # Default Maven repository.
  _DEFAULT_REPO_PATH = os.path.join(os.getenv("HOME"), '.m2', 'repository')
  _POM_FILE_TYPE = 'pom'

  def __init__(self):
    self.repo_path = MavenDownloader._DEFAULT_REPO_PATH
    self.settings_path = None

  def SetCustomSettings(self, directory):
    self.repo_path = directory
    self.settings_path = os.path.join(directory, 'settings.xml')
    _MakeDirsIfAbsent(directory)
    with open(self.settings_path, 'w') as settings:
      settings.write(MavenDownloader._MVN_SETTINGS_TEMPLATE.format(directory))

  def Install(self, target_repo, artifacts, include_poms=False):
    for artifact in artifacts:
      parts = artifact.split(':')
      assert len(parts) == 4, ('Artifacts expected as'
                               '"group_id:artifact_id:version:file_type".')
      group_id, artifact_id, version, file_type = parts

      self._InstallArtifact(
          target_repo, group_id, artifact_id, version, file_type)

      if include_poms and file_type != MavenDownloader._POM_FILE_TYPE:
        self._InstallArtifact(target_repo, group_id, artifact_id, version,
                              MavenDownloader._POM_FILE_TYPE)

  def _InstallArtifact(self, target_repo,
                       group_id, artifact_id, version, file_type):
    logging.debug('Processing %s:%s:%s:%s',
                  group_id, artifact_id, version, file_type)

    download_relpath = self._DownloadArtifact(
        group_id, artifact_id, version, file_type)
    logging.debug('Downloaded.')

    install_path = self._ImportArtifact(target_repo, download_relpath)
    logging.debug('Installed to %s', install_path)

  def _DownloadArtifact(self, group_id, artifact_id, version, file_type):
    '''
    Downloads the specified artifact using maven, to its standard location. It
    can be modified by using custom settings, see SetCustomSettings()
    '''
    cmd = ['mvn',
           'org.apache.maven.plugins:maven-dependency-plugin:RELEASE:get',
           '-DremoteRepositories=https://maven.google.com',
           '-Dartifact={}:{}:{}:{}'.format(group_id, artifact_id, version,
                                           file_type)]
    if self.settings_path:
      cmd.extend(['--global-settings', self.settings_path])

    try:
      # TODO(dgn): Parallelize. dependency:get supports parallel builds and is
      # quite slow to run. It would help speeding up the execution greatly.
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

  def _ImportArtifact(self, target_repo, artifact_path):
    dst_dir = os.path.join(target_repo, os.path.dirname(artifact_path))
    _MakeDirsIfAbsent(dst_dir)
    shutil.copy(os.path.join(self.repo_path, artifact_path), dst_dir)
    return dst_dir
