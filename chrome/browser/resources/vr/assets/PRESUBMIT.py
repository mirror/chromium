# Copyright 2018 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.


def ParseVersion(lines):
  major_version = None
  minor_version = None
  for line in lines:
    key, val = line.rstrip('\r\n').split('=', 1)
    if key == 'MAJOR':
      if major_version is not None:
        return None
      major_version = val
    elif key == 'MINOR':
      if minor_version is not None:
        return None
      minor_version = val
  if major_version is None or minor_version is None:
    return None
  return (major_version, minor_version)


def IsNewer(old_version, new_version):
  if (old_version is not None and new_version is not None and
      (old_version[0] < new_version[0] or
       (old_version[0] == new_version[0] and old_version[1] < new_version[1]))):
    return True
  return False


def CheckVersion(input_api, output_api):
  """Checks that
  - the version was upraded if assets files were changed,
  - the version was not downgraded.
  """
  old_version = None
  new_version = None
  changed_assets = False
  changed_version = False
  for file in input_api.AffectedFiles():
    basename = input_api.os_path.basename(file.LocalPath())
    extension = input_api.os_path.splitext(basename)[1][1:].strip().lower()
    if (extension == 'sha1'):
      changed_assets = True
    if (basename == 'VERSION'):
      changed_version = True
      old_version = ParseVersion(file.OldContents())
      new_version = ParseVersion(file.NewContents())

  version_upgraded = IsNewer(old_version, new_version)
  local_version_filename = input_api.os_path.join(
      input_api.os_path.dirname(input_api.AffectedFiles()[0].LocalPath()),
      'VERSION')

  if changed_assets and not version_upgraded:
    return [
        output_api.PresubmitError(
            'Must increment version in \'%s\' when '
            'updating VR assets.' % local_version_filename)
    ]
  if changed_version and not version_upgraded:
    return [
        output_api.PresubmitError(
            'Must not downgrade version in \'%s\'.' % local_version_filename)
    ]

  return []


def CheckChangeOnUpload(input_api, output_api):
  return CheckVersion(input_api, output_api)


def CheckChangeOnCommit(input_api, output_api):
  return CheckVersion(input_api, output_api)
