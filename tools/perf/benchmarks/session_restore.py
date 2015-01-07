# Copyright 2013 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import os
import tempfile

from measurements import session_restore
import page_sets
from profile_creators import small_profile_creator
from telemetry import benchmark
from telemetry.page import profile_generator


class _SessionRestoreTest(benchmark.Benchmark):

  @classmethod
  def ProcessCommandLineArgs(cls, parser, args):
    super(_SessionRestoreTest, cls).ProcessCommandLineArgs(parser, args)
    profile_type = 'small_profile'
    if not args.browser_options.profile_dir:
      output_dir = os.path.join(tempfile.gettempdir(), profile_type)
      profile_dir = os.path.join(output_dir, profile_type)
      if not os.path.exists(output_dir):
        os.makedirs(output_dir)

      # Generate new profiles if profile_dir does not exist. It only exists if
      # all profiles had been correctly generated in a previous run.
      if not os.path.exists(profile_dir):
        new_args = args.Copy()
        new_args.pageset_repeat = 1
        new_args.output_dir = output_dir
        profile_generator.GenerateProfiles(
            small_profile_creator.SmallProfileCreator, profile_type, new_args)
      args.browser_options.profile_dir = profile_dir

  def CreatePageTest(self, options):
    is_cold = (self.tag == 'cold')
    return session_restore.SessionRestore(cold=is_cold)

# crbug.com/325479, crbug.com/381990
@benchmark.Disabled('android', 'linux', 'reference')
class SessionRestoreColdTypical25(_SessionRestoreTest):
  tag = 'cold'
  page_set = page_sets.Typical25PageSet
  options = {'pageset_repeat': 5}


# crbug.com/325479, crbug.com/381990
@benchmark.Disabled('android', 'linux', 'reference')
class SessionRestoreWarmTypical25(_SessionRestoreTest):
  tag = 'warm'
  page_set = page_sets.Typical25PageSet
  options = {'pageset_repeat': 20}
