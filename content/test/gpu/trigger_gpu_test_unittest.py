#!/usr/bin/python
# Copyright 2018 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Tests for trigger_gpu_test.py."""

import unittest

import trigger_gpu_test

class Args(object):
  pass

class FakeTriggerer(trigger_gpu_test.GpuTestTriggerer):
  def __init__(self, gpu_configs, bot_statuses, first_random_number):
    super(FakeTriggerer, self).__init__()
    self._gpu_configs = gpu_configs
    self._bot_statuses = bot_statuses
    self._swarming_runs = []
    self._files = {}
    self._temp_file_id = 0
    self._last_random_number = first_random_number

  def set_files(self, files):
    self._files = files

  def choose_random_int(self, max_num):
    if self._last_random_number > max_num:
      self._last_random_number = 1
    result = self._last_random_number
    self._last_random_number += 1
    return result

  def make_temp_file(self, prefix=None, suffix=None):
    result = prefix + str(self._temp_file_id) + suffix
    self._temp_file_id += 1
    return result

  def delete_temp_file(self, temp_file):
    pass

  def read_json_from_temp_file(self, temp_file):
    return self._files[temp_file]

  def write_json_to_file(self, merged_json, output_file):
    self._files[output_file] = merged_json

  def parse_gpu_configs(self, args):
    pass

  def query_swarming_for_gpu_configs(self):
    # Sum up the total count of all bots.
    self._total_bots = sum(x['total'] for x in self._bot_statuses)

  def run_swarming(self, args):
    self._swarming_runs.append(args)


WIN_NVIDIA_QUADRO_P400_STABLE_DRIVER = '10de:1cb3-23.21.13.8792'
WIN7 = 'Windows-2008ServerR2-SP1'
WIN10 = 'Windows-10'

WIN7_NVIDIA = {
  'gpu': WIN_NVIDIA_QUADRO_P400_STABLE_DRIVER,
  'os': WIN7,
  'pool': 'Chrome-GPU',
}

WIN10_NVIDIA = {
  'gpu': WIN_NVIDIA_QUADRO_P400_STABLE_DRIVER,
  'os': WIN10,
  'pool': 'Chrome-GPU',
}

class UnitTest(unittest.TestCase):
  def basic_win7_win10_setup(self, bot_statuses, first_random_number=1):
    triggerer = FakeTriggerer(
      [
        WIN7_NVIDIA,
        WIN10_NVIDIA
      ],
      bot_statuses,
      first_random_number
    )
    triggerer.set_files({
      'trigger_gpu_test0.json': {
        'tasks': {
          'webgl_conformance_tests on NVIDIA GPU on Windows': {
            'task_id': 'f001',
          },
        },
      },
      'trigger_gpu_test1.json': {
        'tasks': {
          'webgl_conformance_tests on NVIDIA GPU on Windows': {
            'task_id': 'f002',
          },
        },
      },
    })
    args = Args()
    args.shards = 2
    args.dump_json = 'output.json'
    triggerer.trigger_tasks(
      args,
      [
        'trigger',
        '--dimension',
        'gpu',
        WIN_NVIDIA_QUADRO_P400_STABLE_DRIVER,
        '--dimension',
        'os',
        WIN7,
        '--dimension',
        'pool',
        'Chrome-GPU',
        '--',
        'webgl_conformance',
      ])
    return triggerer

  def list_contains_sublist(self, main_list, sub_list):
    return any(sub_list == main_list[offset:offset + len(sub_list)]
               for offset in xrange(len(main_list) - len(sub_list)))

  def shard_runs_on_os(self, triggerer, shard_index, os):
    return self.list_contains_sublist(triggerer._swarming_runs[shard_index],
                                      ['--dimension', 'os', os])

  def test_split_with_available_machines(self):
    triggerer = self.basic_win7_win10_setup(
      [
        {
          'total': 1,
          'available': 1,
        },
        {
          'total': 1,
          'available': 1,
        },
      ],
    )
    # First shard should run on Win7.
    self.assertTrue(self.shard_runs_on_os(triggerer, 0, WIN7))
    # Second shard should run on Win10.
    self.assertTrue(self.shard_runs_on_os(triggerer, 1, WIN10))
    # And not vice versa.
    self.assertFalse(self.shard_runs_on_os(triggerer, 0, WIN10))
    self.assertFalse(self.shard_runs_on_os(triggerer, 1, WIN7))

  def test_split_with_only_one_config_available(self):
    triggerer = self.basic_win7_win10_setup(
      [
        {
          'total': 2,
          'available': 2,
        },
        {
          'total': 2,
          'available': 0,
        },
      ],
    )
    # Both shards should run on Win7.
    self.assertTrue(self.shard_runs_on_os(triggerer, 0, WIN7))
    self.assertTrue(self.shard_runs_on_os(triggerer, 1, WIN7))
    # Redo with only Win10 bots available.
    triggerer = self.basic_win7_win10_setup(
      [
        {
          'total': 2,
          'available': 0,
        },
        {
          'total': 2,
          'available': 2,
        },
      ],
    )
    self.assertTrue(self.shard_runs_on_os(triggerer, 0, WIN10))
    self.assertTrue(self.shard_runs_on_os(triggerer, 1, WIN10))

  def test_split_with_no_bots_available(self):
    triggerer = self.basic_win7_win10_setup(
      [
        {
          'total': 1,
          'available': 0,
        },
        {
          'total': 1,
          'available': 0,
        },
      ],
    )
    # Given the fake random number generator above, first shard should
    # run on Win7.
    self.assertTrue(self.shard_runs_on_os(triggerer, 0, WIN7))
    # Second shard should run on Win10.
    self.assertTrue(self.shard_runs_on_os(triggerer, 1, WIN10))
    # Try again with different bot distribution and random numbers.
    triggerer = self.basic_win7_win10_setup(
      [
        {
          'total': 2,
          'available': 0,
        },
        {
          'total': 2,
          'available': 0,
        },
      ],
      first_random_number=3,
    )
    self.assertTrue(self.shard_runs_on_os(triggerer, 0, WIN10))
    self.assertTrue(self.shard_runs_on_os(triggerer, 1, WIN10))

if __name__ == '__main__':
  unittest.main()
