# Copyright 2016 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""A mock LocalWPT class with canned responses."""


class MockLocalWPT(object):

    def __init__(self, test_patch=None, apply_patch=None, change_ids=None, commit_positions=None):
        """Initializes the mock with pre-populated responses.

        Args:
            test_patch: a list of (success, error), responses for test_patch.
            apply_patch: a list of error strings, responses for apply_patch.
            change_ids: a list of 'Change-Id's that can be found.
            commit_positions: a list of 'Cr-Commit-Position's that can be found.
        """
        self.test_patch_responses = test_patch or []
        self.apply_patch_responses = apply_patch or []
        self.test_patch_called = 0
        self.apply_patch_called = 0
        self.change_ids = change_ids or []
        self.commit_positions = commit_positions or []

    # TODO(robertma): Fill in other methods as needed.

    def test_patch(self, _):
        success, error = self.test_patch_responses[self.test_patch_called]
        self.test_patch_called += 1
        return success, error

    def apply_patch(self, _):
        error = self.apply_patch_responses[self.apply_patch_called]
        self.apply_patch_called += 1
        return error

    def seek_change_id(self, change_id):
        return change_id in self.change_ids

    def seek_commit_position(self, commit_position):
        return commit_position in self.commit_positions
