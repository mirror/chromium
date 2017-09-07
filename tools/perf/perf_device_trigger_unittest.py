# Copyright 2017 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import json
import os
import tempfile
import unittest
import sys

import mock

import perf_device_trigger


class FakeArgs(object):
  def __init__(self, shards, bot_id, swarming_py_path, dump_json):
    self.shards = shards
    self.bot_id = bot_id
    self.swarming_py_path = swarming_py_path
    self.dump_json = dump_json

class PerfDeviceTriggerUnittest(unittest.TestCase):
  def testBasic(self):
    with mock.patch('perf_device_trigger.subprocess.call') as call_mock:
      call_mock.return_value = 0
      m = mock.mock_open(read_data=json.dumps({
          'tasks': {},
      }))
      with mock.patch('perf_device_trigger.open', m, create=True):
        _, json_temp = tempfile.mkstemp(prefix='perf_device_trigger_unittest')
        try:
          perf_device_trigger.trigger_tasks(
              FakeArgs(
                  1, ['build1'], '/path/to/swarming.py',
                  json_temp),
              ['some', 'test', '--', 'args'])

          call_mock.assert_called_once()

          called_args, keyword = call_mock.call_args
          self.assertEqual(keyword, {})
          self.assertEqual(called_args[0], sys.executable)
          python_args = called_args[1]
          json_ind = python_args.index('--dump_json')
          # Remove --dump_json and its arg
          python_args.pop(json_ind)
          temp_json_path = python_args.pop(json_ind)
          # We can't assert the exact name, since it's a temp file name. So just
          # make sure it has the proper prefix
          self.assertTrue(
              temp_json_path.startswith('/tmp/perf_device_trigger'))

          self.assertEqual(
            python_args, [
            '/path/to/swarming.py',
            '--shards', 1,
            'some', 'test',
            '--dimension', 'id', 'build1', '--',
            'args', '--id', 'build1'])
        finally:
          os.remove(json_temp)
