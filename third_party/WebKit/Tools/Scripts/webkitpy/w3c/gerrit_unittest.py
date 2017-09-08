# Copyright 2017 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import unittest

from webkitpy.common.host_mock import MockHost
from webkitpy.common.system.executive_mock import mock_git_commands
from webkitpy.w3c.gerrit import GerritCL
from webkitpy.w3c.gerrit_mock import MockGerritAPI


class GerritCLTest(unittest.TestCase):

    def test_fetch_commit(self):
        host = MockHost()
        host.executive = mock_git_commands({
            'fetch': '',
            'rev-parse': '4de71d0ce799af441c1f106c5432c7fa7256be45',
            'footers': 'refs/heads/master@{#458475}'
        }, strict=True)
        data = {
            'change_id': 'I001',
            'subject': 'subject',
            '_number': '1',
            'current_revision': '1',
            'has_review_started': True,
            'revisions': {'1': {
                'commit_with_footers': 'a commit with footers',
                'fetch': {'http': {
                    'url': 'https://chromium.googlesource.com/chromium/src',
                    'ref': 'refs/changes/1/1/1'
                }}
            }},
            'owner': {'email': 'test@chromium.org'},
        }
        gerrit_cl = GerritCL(data, MockGerritAPI())
        commit = gerrit_cl.fetch_commit(host)

        self.assertEqual(commit.sha, '4de71d0ce799af441c1f106c5432c7fa7256be45')
        self.assertEqual(host.executive.calls, [
            ['git', 'fetch', 'https://chromium.googlesource.com/chromium/src', 'refs/changes/1/1/1'],
            ['git', 'rev-parse', 'FETCH_HEAD'],
            ['git', 'footers', '--position', '4de71d0ce799af441c1f106c5432c7fa7256be45']
        ])
