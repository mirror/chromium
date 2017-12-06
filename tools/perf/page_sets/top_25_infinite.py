# Copyright 2017 Arm, Inc.
# Copyright 2017 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

from page_sets import top_25_smooth

class Top25InfinitePageSet(top_25_smooth.Top25SmoothPageSet):
  """Take the standard set of pages defined in top_25_smooth and modify
  them to scroll up and down forever."""
  def __init__(self):
    super(Top25InfinitePageSet, self).__init__()
    for story in self.stories:
      self.MakeInfinite(story)

  def MakeInfinite(self, story):
    """Given a page, modify the interactions so that it runs the original
    interactions (e.g., to ensure that login works), then scrolls up and
    down forever"""
    run_page_interactions_original = story.RunPageInteractions
    def RunPageInteractions_Infinite(self, action_runner):
      run_page_interactions_original(action_runner)
      while True:
        action_runner.ScrollPage(direction='up')
        action_runner.ScrollPage(direction='down')
    story.RunPageInteractions = RunPageInteractions_Infinite.__get__(story)
