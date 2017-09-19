#!/usr/bin/env python
# Copyright 2017 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import unittest
import sys

import generate_system_health_csv
from core import path_util
sys.path.insert(1, path_util.GetTelemetryDir())  # To resolve telemetry imports

from page_sets.system_health import expectations


class GenerateSystemHealthCSVTest(unittest.TestCase):
  def testPopulateExpectations(self):
    expected_result = {
	    'browse:media:tumblr': 'Mac 10.11',
	    'browse:news:cnn': 'Mac Platforms',
	    'browse:news:hackernews': 'Win Platforms, Mac Platforms',
	    'browse:search:google': 'Win Platforms',
	    'browse:tools:earth': 'All Platforms',
	    'browse:tools:maps': 'All Platforms',
	    'play:media:google_play_music': 'All Platforms',
	    'play:media:pandora': 'All Platforms',
	    'play:media:soundcloud': 'Win Platforms'}
    all_expects = [
	    expectations.SystemHealthDesktopCommonExpectations(
		    ).AsDict()['stories']]

    self.assertEquals(
	    expected_result,
	    generate_system_health_csv.PopulateExpectations(all_expects))

if __name__ == '__main__':
    unittest.main()
