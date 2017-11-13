#!/usr/bin/env python
# Copyright (c) 2012 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

'''Unit tests for grit.format.data_pack'''


import os
import sys
if __name__ == '__main__':
  sys.path.append(os.path.join(os.path.dirname(__file__), '../..'))

import unittest

from grit.format import data_pack


class FormatDataPackUnittest(unittest.TestCase):
  def testReadDataPackV4(self):
    expected_data = (
        '\x04\x00\x00\x00'                  # header(version
        '\x04\x00\x00\x00'                  #        no. entries,
        '\x01'                              #        encoding)
        '\x01\x00\x27\x00\x00\x00'          # index entry 1
        '\x04\x00\x27\x00\x00\x00'          # index entry 4
        '\x06\x00\x33\x00\x00\x00'          # index entry 6
        '\x0a\x00\x3f\x00\x00\x00'          # index entry 10
        '\x00\x00\x3f\x00\x00\x00'          # extra entry for the size of last
        'this is id 4this is id 6')         # data
    expected_resources = {
        1: '',
        4: 'this is id 4',
        6: 'this is id 6',
        10: '',
    }
    expected_data_pack = data_pack.DataPackContents(
        expected_resources, data_pack.UTF8)
    loaded = data_pack.ReadDataPackFromString(expected_data)
    self.assertEquals(loaded, expected_data_pack)


  def testReadWriteDataPackV6(self):
    expected_data = (
      '\x06\x00\x00\x00'                # version
      '\x01\x00\x00\x00'                # encoding and padding
      '\x06\x00'                        # resource_count
      '\x02\x00'                        # alias_count
      '\x04\x00'                        # id_entries
      '\x06\x00'                        # num_resources_64k_1
      '\x00\x00'                        # num_resources_64k_2
      '\x00\x00'                        # num_resources_64k_3
      '\x01\x00\x00\x00'                # id entry 1
      '\x04\x00\x02\x00'                # id entry 4
      '\x0a\x00\x04\x00'                # id entry 10
      '\x00\x00\x06\x00'                # dumb entry
      '\x0a\x00\x02\x00'                # alias table
      '\x0b\x00\x00\x00'                # alias table
      '\x00\x00'                        # offset for id 1
      '\x00\x00'                        # offset for id 2
      '\x0c\x00'                        # offset for id 4
      '\x18\x00'                        # offset for id 5
      '\x24\x00'                        # offset for id 10
      '\x24\x00'                        # offset for id 11
      'this is id 2this is id 4this is id 5')
    expected_resources = {
        1: '',
        2: 'this is id 2',
        4: 'this is id 4',
        5: 'this is id 5',
        10: 'this is id 4',
        11: '',
    }
    data = data_pack.WriteDataPackToString(expected_resources, data_pack.UTF8)
    self.assertEquals(data, expected_data)

    expected_data_pack = data_pack.DataPackContents(
        expected_resources, data_pack.UTF8)
    loaded = data_pack.ReadDataPackFromString(expected_data)
    self.assertEquals(loaded, expected_data_pack)

  def testReadWriteEmptyDataPack(self):
    expected_data = (
      '\x06\x00\x00\x00'                # version
      '\x01\x00\x00\x00'                # encoding and padding
      '\x00\x00'                        # resource_count
      '\x00\x00'                        # alias_count
      '\x01\x00'                        # id_entries_count
      '\x00\x00'                        # num_resources_64k_1
      '\x00\x00'                        # num_resources_64k_2
      '\x00\x00'                        # num_resources_64k_3
      '\x00\x00\x00\x00'                # dumb id entry
    )
    expected_resources = {}
    data = data_pack.WriteDataPackToString(expected_resources, data_pack.UTF8)
    self.assertEquals(data, expected_data)

    expected_data_pack = data_pack.DataPackContents(
        expected_resources, data_pack.UTF8)
    loaded = data_pack.ReadDataPackFromString(expected_data)
    self.assertEquals(loaded, expected_data_pack)

  def testReadWrittenDataPack(self):
    k32K = '1' * 32 * 1024
    expected_resources = {
        1: '',
        4: 'this is id 4',
        5: k32K,
        10: 'this is id 4',
        11: k32K + k32K,
        100: k32K * 4,
        101: k32K * 5,
        102: k32K * 2,
        103: k32K * 2,
    }
    data = data_pack.WriteDataPackToString(expected_resources, data_pack.UTF8)

    expected_data_pack = data_pack.DataPackContents(
        expected_resources, data_pack.UTF8)
    loaded = data_pack.ReadDataPackFromString(data)
    self.assertEquals(loaded, expected_data_pack)

  def testRePackUnittest(self):
    expected_with_whitelist = {
        1: 'Never gonna', 10: 'give you up', 20: 'Never gonna let',
        30: 'you down', 40: 'Never', 50: 'gonna run around and',
        60: 'desert you'}
    expected_without_whitelist = {
        1: 'Never gonna', 10: 'give you up', 20: 'Never gonna let', 65: 'Close',
        30: 'you down', 40: 'Never', 50: 'gonna run around and', 4: 'click',
        60: 'desert you', 6: 'chirr', 32: 'oops, try again', 70: 'Awww, snap!'}
    inputs = [{1: 'Never gonna', 4: 'click', 6: 'chirr', 10: 'give you up'},
              {20: 'Never gonna let', 30: 'you down', 32: 'oops, try again'},
              {40: 'Never', 50: 'gonna run around and', 60: 'desert you'},
              {65: 'Close', 70: 'Awww, snap!'}]
    whitelist = [1, 10, 20, 30, 40, 50, 60]
    inputs = [data_pack.DataPackContents(input, data_pack.UTF8) for input
              in inputs]

    # RePack using whitelist
    output, _ = data_pack.RePackFromDataPackStrings(
        inputs, whitelist, suppress_removed_key_output=True)
    self.assertDictEqual(expected_with_whitelist, output,
                         'Incorrect resource output')

    # RePack a None whitelist
    output, _ = data_pack.RePackFromDataPackStrings(
        inputs, None, suppress_removed_key_output=True)
    self.assertDictEqual(expected_without_whitelist, output,
                         'Incorrect resource output')


if __name__ == '__main__':
  unittest.main()
