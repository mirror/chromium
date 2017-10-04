#!/usr/bin/env python
# Copyright 2017 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import os
import threading
import unittest

import concurrent


def _ForkTestHelper(test_instance, parent_pid, arg1, arg2, _=None):
  test_instance.assertNotEquals(os.getpid(), parent_pid)
  return arg1 + arg2


class Unpicklable(object):
  """Ensures that pickle() is not called on paramters."""
  def __getstate__(self):
    raise AssertionError('Tried to pickle')


class ConcurrentTest(unittest.TestCase):

  def testEncodeDictOfLists_AllStrings(self):
    test_dict = {'foo': ['a', 'b', 'c'], 'foo2': ['a', 'b']}
    encoded = concurrent.EncodeDictOfLists(test_dict)
    decoded = concurrent.DecodeDictOfLists(encoded)
    self.assertEquals(test_dict, decoded)

  def testEncodeDictOfLists_KeyTransform(self):
    test_dict = {0: ['a', 'b', 'c'], 9: ['a', 'b']}
    encoded = concurrent.EncodeDictOfLists(test_dict, key_transform=str)
    decoded = concurrent.DecodeDictOfLists(encoded, key_transform=int)
    self.assertEquals(test_dict, decoded)

  def testEncodeDictOfLists_Join(self):
    test_dict1 = {'key1': ['a']}
    test_dict2 = {'key2': ['b']}
    expected = {'key1': ['a'], 'key2': ['b']}
    encoded1 = concurrent.EncodeDictOfLists(test_dict1)
    encoded2 = concurrent.EncodeDictOfLists(test_dict2)
    encoded = concurrent.JoinEncodedDictOfLists([encoded1, encoded2])
    decoded = concurrent.DecodeDictOfLists(encoded)
    self.assertEquals(expected, decoded)

  def testCallOnThread(self):
    main_thread = threading.current_thread()
    def callback(arg1, arg2):
      self.assertEquals(arg1, 1)
      self.assertEquals(arg2, 2)
      my_thread = threading.current_thread()
      self.assertNotEquals(my_thread, main_thread)
      return 3

    result = concurrent.CallOnThread(callback, 1, arg2=2)
    self.assertEquals(result.get(), 3)

  def testForkAndCall_normal(self):
    parent_pid = os.getpid()
    result = concurrent.ForkAndCall(
        _ForkTestHelper, (self, parent_pid, 1, 2, Unpicklable()))
    self.assertEquals(result.get(), 3)

  def testForkAndCall_exception(self):
    parent_pid = os.getpid()
    result = concurrent.ForkAndCall(_ForkTestHelper, (self, parent_pid, 1, 'a'))
    self.assertRaises(TypeError, result.get)

  def testBulkForkAndCall_normal(self):
    parent_pid = os.getpid()
    results = concurrent.BulkForkAndCall(_ForkTestHelper, [
        (self, parent_pid, 1, 2, Unpicklable()),
        (self, parent_pid, 3, 4)])
    self.assertEquals(set(results), {3, 7})

  def testBulkForkAndCall_exception(self):
    parent_pid = os.getpid()
    results = concurrent.BulkForkAndCall(_ForkTestHelper, [
        (self, parent_pid, 1, 'a')])
    self.assertRaises(TypeError, results.next)

if __name__ == '__main__':
  unittest.main()
