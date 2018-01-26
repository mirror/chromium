# Copyright 2017 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.
from core import perf_benchmark

from measurements import rendering
import page_sets
from telemetry import benchmark
from telemetry import story as story_module


class _Rendering(perf_benchmark.PerfBenchmark):

  test = rendering.Rendering

  @classmethod
  def Name(cls):
    return 'rendering'


@benchmark.Owner(emails=['vmiura@chromium.org'])
class RenderingDesktop(_Rendering):

  page_set = page_sets.RenderingDesktopPageSet
  SUPPORTED_PLATFORMS = [story_module.expectations.ALL_DESKTOP]

  @classmethod
  def Name(cls):
    return 'rendering.desktop'


@benchmark.Owner(emails=['vmiura@chromium.org'])
class RenderingMobile(_Rendering):

  page_set = page_sets.KeySilkCasesPageSet
  SUPPORTED_PLATFORMS = [story_module.expectations.ALL_MOBILE]

  @classmethod
  def Name(cls):
    return 'rendering.mobile'

  def CreateStorySet(self, options):
    stories = super(RenderingMobile, self).CreateStorySet(options)
    # Page26 (befamous) is too noisy to be useful; crbug.com/461127
    to_remove = [story for story in stories
                 if isinstance(story, page_sets.key_silk_cases.Page26)]
    for story in to_remove:
      stories.RemoveStory(story)
    return stories
