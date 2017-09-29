#!/usr/bin/env python
# Copyright 2017 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

'''
TODO
Script to help uploading and downloading the Google Play services library to
and from a Google Cloud storage.
'''

import argparse
import logging
import os
import re
import shutil
import sys
import tempfile

import devil_chromium
from devil.utils import cmd_helper
from play_services import utils
from py_utils import tempfile_ext
from pylib.utils import logging_utils

# from pylib import constants
from pylib.constants import host_paths

# Template for a Maven settings.xml which instructs Maven to download to the
# given directory
MAVEN_SETTINGS_TEMPLATE = '''\
<settings xmlns="http://maven.apache.org/SETTINGS/1.0.0"
          xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance"
          xsi:schemaLocation="http://maven.apache.org/SETTINGS/1.0.0
                              https://maven.apache.org/xsd/settings-1.0.0.xsd" >
  <localRepository>{}</localRepository>
</settings>
'''

# def UpdateGmsCore(config_path, sdk_root):
#   config = utils.ConfigParser(config_path)
#   target_repo = os.path.join(sdk_root, EXTRAS_GOOGLE_M2REPOSITORY)
#   artifacts = ['com.google.android.gms:{}:{}'.format(client, config,version_number) for client in config.clients]
#   Update(target_repo, artifacts)


def UpdateSupportLib():
  DEFAULT_PATH = os.path.join(host_paths.DIR_SOURCE_ROOT, 'third_party',
      'android_tools', 'sdk', 'extras', 'android', 'm2repository')
  Update(DEFAULT_PATH, [
    ('com.android.support', 'design', '26.1.0', 'aar'),
    ('com.android.support', 'transition', '26.1.0', 'aar'),
    ('com.android.support', 'support-annotations', '26.1.0', 'jar'),
    ('com.android.support', 'support-compat', '26.1.0', 'aar'),
    ('com.android.support', 'support-core-ui', '26.1.0', 'aar'),
    ('com.android.support', 'support-core-utils', '26.1.0', 'aar'),
    ('com.android.support', 'support-fragment', '26.1.0', 'aar'),
    ('com.android.support', 'support-media-compat', '26.1.0', 'aar'),
    ('com.android.support', 'support-v13', '26.1.0', 'aar'),
    ('com.android.support', 'support-vector-drawable', '26.1.0', 'aar'),
    ('com.android.support', 'animated-vector-drawable', '26.1.0', 'aar'),
    ('com.android.support', 'appcompat-v7', '26.1.0', 'aar'),
    ('com.android.support', 'gridlayout-v7', '26.1.0', 'aar'),
    ('com.android.support', 'mediarouter-v7', '26.1.0', 'aar'),
    ('com.android.support', 'recyclerview-v7', '26.1.0', 'aar'),
    ('com.android.support', 'palette-v7', '26.1.0', 'aar'),
    ('com.android.support', 'preference-v7', '26.1.0', 'aar'),
    ('com.android.support', 'preference-v14', '26.1.0', 'aar'),
    ('com.android.support', 'leanback-v17', '26.1.0', 'aar'),
    ('com.android.support', 'preference-leanback-v17', '26.1.0', 'aar'),
    ('android.arch.lifecycle', 'runtime', '1.0.0', 'aar'),
    ('android.arch.lifecycle', 'common', '1.0.0', 'jar'),
    ('android.arch.core', 'common', '1.0.0', 'jar'),
  ])

def Update(target_repo, artifacts):
  # Remove the old SDK.
  # shutil.rmtree(target_repo)

  # with tempfile_ext.NamedTemporaryDirectory() as temp_dir:
  temp_dir = tempfile.mkdtemp()
  # Configure temp_dir as the Maven repository.
  settings_path = SetupRepository(temp_dir)

  for artifact in artifacts:
    artifact_path = DownloadArtifact(settings_path, *artifact)
    # ImportArtifact(temp_dir, target_repo, artifact_path)

  print(temp_dir)


def SetupRepository(temp_dir):
  # Configure temp_dir as the Maven repository.
  settings_path = os.path.join(temp_dir, 'settings.xml')
  with open(settings_path, 'w') as settings:
    settings.write(MAVEN_SETTINGS_TEMPLATE.format(temp_dir))
  return settings_path


# def DownloadArtifact(artifact_string):
#   parts = artifact_string.split(':')
#   DownloadArtifact(parts[:3].join(':'), parts[-3:1], parts[-2:1], parts[-1:])


def DownloadArtifact(settings_path, group_id, artifact_id, version, file_type):
  logging.info('DownloadArtifact args:', group_id, artifact_id, version, file_type)

  artifact = '{}:{}:{}:{}'.format(group_id, artifact_id, version, file_type)
  try:
    cmd_helper.Call([
        'mvn', '--global-settings', settings_path,
        'org.apache.maven.plugins:maven-dependency-plugin:2.1:get',
        '-DrepoUrl=https://maven.google.com', '-Dartifact=' + artifact])
  except OSError as e:
    if e.errno == os.errno.ENOENT:
      logging.error('mvn command not found. Please install Maven.')
      sys.exit(-1)
    else:
      raise

  output_path = os.path.join(os.path.join(*group_id.split('.')), artifact_id, version, '{}-{}.{}'.format(artifact_id, version, file_type))
  logging.info('DownloadArtifact output_path:', output_path)
  return output_path

def ImportArtifact(source_repo, target_repo, artifact_path):
    dst_dir = os.path.join(target_repo, os.path.dirname(artifact_path))
    _MakeDirIfAbsent(dst_dir)
    logging.info('Importing Artifact to', dst_dir)
    shutil.copy(os.path.join(source_repo, artifact_path), dst_dir)


def _MakeDirIfAbsent(path):
  try:
    os.makedirs(path)
  except OSError as e:
    if e.errno != os.errno.EEXIST:
      raise


if __name__ == '__main__':
  UpdateSupportLib()
