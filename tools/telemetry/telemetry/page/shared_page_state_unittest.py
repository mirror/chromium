# Copyright 2014 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import tempfile
import unittest

from telemetry.core import wpr_modes
from telemetry.page import shared_page_state
from telemetry.page import page_runner
from telemetry.page import page_set
from telemetry.page import page_test
from telemetry.unittest_util import options_for_unittests


def SetUpPageRunnerArguments(options):
  parser = options.CreateParser()
  page_runner.AddCommandLineArgs(parser)
  options.MergeDefaultValues(parser.get_default_values())
  page_runner.ProcessCommandLineArgs(parser, options)


class DummyTest(page_test.PageTest):
  def ValidatePage(self, *_):
    pass


class FakeNetworkController(object):
  def __init__(self):
    self.archive_path = None
    self.wpr_mode = None

  def SetReplayArgs(self, archive_path, wpr_mode, _netsim, _extra_wpr_args,
                    _make_javascript_deterministic=False):
    self.archive_path = archive_path
    self.wpr_mode = wpr_mode


class SharedPageStateTests(unittest.TestCase):

  def setUp(self):
    self.options = options_for_unittests.GetCopy()
    SetUpPageRunnerArguments(self.options)
    self.options.output_formats = ['none']
    self.options.suppress_gtest_report = True

  # pylint: disable=W0212
  def TestUseLiveSitesFlag(self, expected_wpr_mode):
    with tempfile.NamedTemporaryFile() as f:
      run_state = shared_page_state.SharedPageState(
          DummyTest(), self.options, page_set.PageSet())
      fake_network_controller = FakeNetworkController()
      run_state._PrepareWpr(fake_network_controller, f.name, None)
      self.assertEquals(fake_network_controller.wpr_mode, expected_wpr_mode)
      self.assertEquals(fake_network_controller.archive_path, f.name)

  def testUseLiveSitesFlagSet(self):
    self.options.use_live_sites = True
    self.TestUseLiveSitesFlag(expected_wpr_mode=wpr_modes.WPR_OFF)

  def testUseLiveSitesFlagUnset(self):
    self.TestUseLiveSitesFlag(expected_wpr_mode=wpr_modes.WPR_REPLAY)
