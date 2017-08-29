# Copyright 2017 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import os
from contrib.vr_benchmarks.vr_page_sets.vr_page import VrPage

SAMPLE_DIR = os.path.join(
    os.path.dirname(__file__), '..', '..', '..', '..', '..', 'chrome', 'test',
    'data', 'vr', 'webvr_info', 'samples')


class VrSamplePage(VrPage):

  def __init__(self, page, page_set, get_parameters=None):
    url = '%s.html' % page
    if get_parameters is not None:
      url += '?' + '&'.join(get_parameters)
    name = url.replace('.html', '')
    url = 'file://' + os.path.join(SAMPLE_DIR, url)
    super(VrSamplePage, self).__init__(url=url, page_set=page_set, name=name)
