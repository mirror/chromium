# Copyright 2017 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

from telemetry import page
from contrib.vr_benchmarks.vr_page_sets import (shared_android_vr_page_state as
                                                vr_state)


class VrPage(page.Page):
  """Superclass for all pages used for VR testing."""

  def __init__(self, url, page_set, name):
    super(VrPage, self).__init__(
        url=url,
        page_set=page_set,
        name=name,
        shared_page_state_class=vr_state.SharedAndroidVrPageState)
    self._shared_page_state = None

  def Run(self, shared_state):
    self._shared_page_state = shared_state
    super(VrPage, self).Run(shared_state)

  @property
  def platform(self):
    return self._shared_page_state.platform
