#!/usr/bin/python
# Copyright 2018 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Tests for trigger_multiple_dimensions.py."""

import unittest

import trigger_multiple_dimensions_unittest


WIN_NVIDIA_QUADRO_P400_STABLE_DRIVER = '10de:1cb3-23.21.13.8792'
WIN7 = 'Windows-2008ServerR2-SP1'
WIN10 = 'Windows-10'

PERF_BOT_1 = {
  'pool': 'Chrome-perf',
  'id': 'build1'
}

PERF_BOT_2 = {
  'pool': 'Chrome-perf',
  'id': 'build2'
}

class UnitTest(unittest.TestCase):
  def basic_win7_win10_setup(self, bot_statuses,
                             use_superclass_random_number_generator=False,
                             first_random_number=1):
    triggerer = trigger_multiple_dimensions_unittest.FakeTriggerer(
      [
        PERF_BOT_1,
        PERF_BOT_2
      ],
      bot_statuses,
      use_superclass_random_number_generator,
      first_random_number
    )
    # Note: the contents of these JSON files don't accurately reflect
    # that produced by "swarming.py trigger". The unit tests only
    # verify that shard 0's JSON is preserved.
    triggerer.set_files({
      'trigger_multiple_dimensions0.json': {
        'base_task_name': 'webgl_conformance_tests',
        'request': {
          'expiration_secs': 3600,
          'properties': {
            'execution_timeout_secs': 3600,
          },
        },
        'tasks': {
          'webgl_conformance_tests on NVIDIA GPU on Windows': {
            'task_id': 'f001',
          },
        },
      },
      'trigger_multiple_dimensions1.json': {
        'tasks': {
          'webgl_conformance_tests on NVIDIA GPU on Windows': {
            'task_id': 'f002',
          },
        },
      },
    })
    args = trigger_multiple_dimensions_unittest.Args()
    args.shards = 2
    args.dump_json = 'output.json'
    args.multiple_dimension_script_verbose = False
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

  def test_shard_env_vars(self):
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
    self.assertTrue(self.list_contains_sublist(
      triggerer._swarming_runs[0], ['--env', 'GTEST_SHARD_INDEX', '0']))
    self.assertTrue(self.list_contains_sublist(
      triggerer._swarming_runs[1], ['--env', 'GTEST_SHARD_INDEX', '1']))
    self.assertTrue(self.list_contains_sublist(
      triggerer._swarming_runs[0], ['--env', 'GTEST_TOTAL_SHARDS', '2']))
    self.assertTrue(self.list_contains_sublist(
      triggerer._swarming_runs[1], ['--env', 'GTEST_TOTAL_SHARDS', '2']))

  def test_json_merging(self):
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
    self.assertTrue('output.json' in triggerer._files)
    output_json = triggerer._files['output.json']
    self.assertTrue('base_task_name' in output_json)
    self.assertTrue('request' in output_json)
    self.assertEqual(output_json['request']['expiration_secs'], 3600)
    self.assertEqual(
      output_json['request']['properties']['execution_timeout_secs'], 3600)

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

  def test_superclass_random_number_generator_works(self):
    # Probe randomly a certain number of times.
    num_runs = 0
    for _ in xrange(100):
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
        use_superclass_random_number_generator=True
      )
      for _ in xrange(2):
        self.assertTrue(self.shard_runs_on_os(triggerer, 0, WIN7) or
                        self.shard_runs_on_os(triggerer, 0, WIN10))
        num_runs += 1
    self.assertEqual(num_runs, 200)

if __name__ == '__main__':
  unittest.main()
