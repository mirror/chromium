# Copyright 2017 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.


"""Chromium presubmit script for src/tools/media_engagement_preload."""


def _MakeDafsaTestCommand(input_api, output_api, file_name):
  source_path = input_api.os_path.dirname(input_api.os_path.dirname(
      input_api.PresubmitLocalPath()))
  test_path = input_api.os_path.join(source_path, '%s.py' % file_name)
  cmd = [input_api.python_executable, test_path]
  return input_api.Command(
    name=file_name,
    cmd=cmd,
    kwargs={},
    message=output_api.PresubmitPromptWarning)


def _RunDafsaTests(input_api, output_api):
  """Runs unittest for make_dafsa if any related file has been modified."""
  files = ('tools/media_engagement_preload/make_dafsa.py',
           'tools/media_engagement_preload/make_dafsa_unittest.py',
           'tools/media_engagement_preload/media_engagement_preload_pb2.py')
  if not any(f in input_api.LocalPaths() for f in files):
    return []
  return input_api.RunTests([
    _MakeDafsaTestCommand(
        input_api, output_api, 'net/tools/dafsa/dafsa_utils_unittest'),
    _MakeDafsaTestCommand(
        input_api, output_api,
        'tools/media_engagement_preload/make_dafsa_unittest'),
  ])


def CheckChangeOnUpload(input_api, output_api):
  return _RunDafsaTests(input_api, output_api)


def CheckChangeOnCommit(input_api, output_api):
  return _RunDafsaTests(input_api, output_api)
