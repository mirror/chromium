# Copyright 2016 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import json
import unittest

from webkitpy.common.host_mock import MockHost
from webkitpy.w3c.common import (
    read_credentials,
    is_testharness_baseline,
    is_basename_skipped,
    is_path_skipped,
    is_file_synced,
    CHROMIUM_WPT_DIR
)


class CommonTest(unittest.TestCase):

    def test_get_credentials_empty(self):
        host = MockHost()
        host.filesystem.write_text_file('/tmp/credentials.json', '{}')
        self.assertEqual(read_credentials(host, '/tmp/credentials.json'), {})

    def test_get_credentials_none(self):
        self.assertEqual(read_credentials(MockHost(), None), {})

    def test_get_credentials_gets_values_from_file(self):
        host = MockHost()
        host.filesystem.write_text_file(
            '/tmp/credentials.json',
            json.dumps({
                'GH_USER': 'user-github',
                'GH_TOKEN': 'pass-github',
                'GERRIT_USER': 'user-gerrit',
                'GERRIT_TOKEN': 'pass-gerrit',
            }))
        self.assertEqual(
            read_credentials(host, '/tmp/credentials.json'),
            {
                'GH_USER': 'user-github',
                'GH_TOKEN': 'pass-github',
                'GERRIT_USER': 'user-gerrit',
                'GERRIT_TOKEN': 'pass-gerrit',
            })

    def test_is_testharness_baseline(self):
        self.assertTrue(is_testharness_baseline('fake-test-expected.txt'))
        self.assertFalse(is_testharness_baseline('fake-test-expected.html'))
        self.assertTrue(is_testharness_baseline('external/wpt/fake-test-expected.txt'))
        self.assertTrue(is_testharness_baseline('/tmp/wpt/fake-test-expected.txt'))

    def test_is_basename_skipped(self):
        self.assertTrue(is_basename_skipped('OWNERS'))
        self.assertTrue(is_basename_skipped('reftest.list'))

    def test_is_basename_skipped_asserts_basename(self):
        with self.assertRaises(AssertionError):
            is_basename_skipped('third_party/fake/OWNERS')

    def test_is_path_skipped(self):
        self.assertTrue(is_path_skipped(CHROMIUM_WPT_DIR + 'MANIFEST.json'))
        self.assertTrue(is_path_skipped(CHROMIUM_WPT_DIR + 'resources/testharnessreport.js'))
        self.assertFalse(is_path_skipped(CHROMIUM_WPT_DIR + 'fake/MANIFEST.json'))

    def test_is_file_synced(self):
        self.assertTrue(is_file_synced(CHROMIUM_WPT_DIR + 'html/fake-test.html'))
        self.assertFalse(is_file_synced(CHROMIUM_WPT_DIR + 'MANIFEST.json'))
        self.assertFalse(is_file_synced(CHROMIUM_WPT_DIR + 'dom/OWNERS'))

    def test_is_file_synced_asserts_path(self):
        # Rejects basenames.
        with self.assertRaises(AssertionError):
            is_file_synced('MANIFEST.json')
        # Rejects files not in Chromium WPT.
        with self.assertRaises(AssertionError):
            is_file_synced('third_party/fake/OWNERS')
