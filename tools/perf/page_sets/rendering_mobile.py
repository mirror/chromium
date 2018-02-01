# Copyright 2018 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.
from telemetry.page import shared_page_state
from telemetry import story

from page_sets import key_mobile_sites_smooth


class _SharedPageState(shared_page_state.SharedMobilePageState):
  pass


class RenderingMobilePageSet(story.StorySet):
  """ Page set for measuring rendering performance on desktop. """

  def __init__(self):
    super(RenderingMobilePageSet, self).__init__(
        archive_data_file='data/rendering_mobile.json',
        cloud_storage_bucket=story.PARTNER_BUCKET)

    key_mobile_sites_smooth.AddPagesToPageSet(
        self,
        shared_page_state_class=_SharedPageState,
        name_prefix='key_mobile_sites_smooth/')
    key_mobile_sites_smooth.AddPagesToPageSet(
        self,
        shared_page_state_class=_SharedPageState,
        name_prefix='key_mobile_sites_smooth/sync_scroll/',
        extra_browser_args='--disable-threaded-scrolling')
