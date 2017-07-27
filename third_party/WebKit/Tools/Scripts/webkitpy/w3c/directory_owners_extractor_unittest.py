# Copyright 2017 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import unittest

from webkitpy.common.system.filesystem_mock import MockFileSystem
from webkitpy.w3c.directory_owners_extractor import DirectoryOwnersExtractor


class DirectoryOwnersExtractorTest(unittest.TestCase):

    def setUp(self):
        self.filesystem = MockFileSystem()
        self.extractor = DirectoryOwnersExtractor(self.filesystem)

    def test_list_owners(self):
        self.filesystem.files = {
            '/mock-checkout/third_party/WebKit/LayoutTests/external/wpt/foo/x.html': '',
            '/mock-checkout/third_party/WebKit/LayoutTests/external/wpt/foo/OWNERS':
            'a@chromium.org\nc@chromium.org\n',
            '/mock-checkout/third_party/WebKit/LayoutTests/external/wpt/bar/x/y.html': '',
            '/mock-checkout/third_party/WebKit/LayoutTests/external/wpt/bar/OWNERS':
            'a@chromium.org\nc@chromium.org\n',
            '/mock-checkout/third_party/WebKit/LayoutTests/external/wpt/baz/x/y.html': '',
            '/mock-checkout/third_party/WebKit/LayoutTests/external/wpt/baz/x/OWNERS':
            'b@chromium.org\n',
            '/mock-checkout/third_party/WebKit/LayoutTests/external/wpt/quux/x/y.html': '',
        }
        changed_files = [
            # Same owners:
            'third_party/WebKit/LayoutTests/external/wpt/foo/x.html',
            'third_party/WebKit/LayoutTests/external/wpt/bar/x/y.html',
            # Same owned directories:
            'third_party/WebKit/LayoutTests/external/wpt/baz/x/y.html',
            'third_party/WebKit/LayoutTests/external/wpt/baz/x/z.html',
            # Owners not found:
            'third_party/WebKit/LayoutTests/external/wpt/quux/x/y.html',
        ]
        self.assertEqual(
            self.extractor.list_owners(changed_files),
            {('a@chromium.org', 'c@chromium.org'): ['external/wpt/bar', 'external/wpt/foo'],
             ('b@chromium.org',): ['external/wpt/baz/x']}
        )

    def test_find_and_extract_owners_current_dir(self):
        self.filesystem.files = {'/mock-checkout/third_party/WebKit/LayoutTests/external/wpt/foo/OWNERS': 'a@chromium.org'}
        self.assertEqual(self.extractor.find_and_extract_owners('third_party/WebKit/LayoutTests/external/wpt/foo'),
                         ('/mock-checkout/third_party/WebKit/LayoutTests/external/wpt/foo/OWNERS', ['a@chromium.org']))

    def test_find_and_extract_owners_ancestor(self):
        self.filesystem.files = {
            '/mock-checkout/third_party/WebKit/LayoutTests/external/wpt/x/OWNERS': 'a@chromium.org',
            '/mock-checkout/third_party/WebKit/LayoutTests/external/wpt/x/y/z.html': '',
        }
        self.assertEqual(self.extractor.find_and_extract_owners('third_party/WebKit/LayoutTests/external/wpt/x/y'),
                         ('/mock-checkout/third_party/WebKit/LayoutTests/external/wpt/x/OWNERS', ['a@chromium.org']))

    def test_find_and_extract_owners_not_found(self):
        self.filesystem.files = {
            '/mock-checkout/third_party/WebKit/LayoutTests/external/wpt/foo/OWNERS': 'a@chromium.org',
            '/mock-checkout/third_party/WebKit/LayoutTests/external/OWNERS': 'a@chromium.org',
            '/mock-checkout/third_party/WebKit/LayoutTests/external/wpt/x/y/z.html': '',
        }
        self.assertEqual(self.extractor.find_and_extract_owners('third_party/WebKit/LayoutTests/external/wpt/x/y'),
                         (None, None))

    def test_find_and_extract_owners_skip_empty(self):
        self.filesystem.files = {
            '/mock-checkout/third_party/WebKit/LayoutTests/external/wpt/x/OWNERS': 'a@chromium.org',
            '/mock-checkout/third_party/WebKit/LayoutTests/external/wpt/x/y/OWNERS': '# b@chromium.org',
            '/mock-checkout/third_party/WebKit/LayoutTests/external/wpt/x/y/z.html': '',
        }
        self.assertEqual(self.extractor.find_and_extract_owners('third_party/WebKit/LayoutTests/external/wpt/x/y'),
                         ('/mock-checkout/third_party/WebKit/LayoutTests/external/wpt/x/OWNERS', ['a@chromium.org']))

    def test_find_and_extract_owners_absolute_path(self):
        with self.assertRaises(AssertionError):
            self.extractor.find_and_extract_owners('/absolute/path')

    def test_extract_owners(self):
        self.filesystem.files = {
            '/mock-checkout/third_party/WebKit/LayoutTests/external/wpt/foo/OWNERS':
            '#This is a comment\n'
            '*\n'
            'foo@chromium.org\n'
            'bar@chromium.org\n'
            'foobar\n'
            '#foobar@chromium.org\n'
            '# TEAM: some-team@chromium.org\n'
            '# COMPONENT: Blink>Layout\n'
        }
        self.assertEqual(self.extractor.extract_owners('/mock-checkout/third_party/WebKit/LayoutTests/external/wpt/foo/OWNERS'),
                         ['foo@chromium.org', 'bar@chromium.org'])
