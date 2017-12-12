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

  page_set = page_sets.Top25SmoothPageSet
  SUPPORTED_PLATFORMS = [story_module.expectations.ALL_DESKTOP]

  @classmethod
  def Name(cls):
    return 'rendering.desktop'

  def GetExpectations(self):
    class StoryExpectations(story_module.expectations.StoryExpectations):
      def SetExpectations(self):
        self.DisableStory('http://www.cnn.com', [story_module.expectations.ALL],
                          'crbug.com/528474')
        self.DisableStory('http://www.amazon.com',
                          [story_module.expectations.ALL],
                          'crbug.com/667432')
        self.DisableStory('https://mail.google.com/mail/',
                          [story_module.expectations.ALL_WIN],
                          'crbug.com/750147')
        self.DisableStory('Docs  (1 open document tab)',
                          [story_module.expectations.ALL_WIN],
                          'crbug.com/762165')
        self.DisableStory('http://www.youtube.com',
                          [story_module.expectations.ALL_WIN],
                          'crbug.com/762165')
        self.DisableStory('https://plus.google.com/110031535020051778989/posts',
                          [story_module.expectations.ALL_WIN],
                          'crbug.com/762165')
        self.DisableStory('https://www.google.com/calendar/',
                          [story_module.expectations.ALL_WIN],
                          'crbug.com/762165')
        self.DisableStory('https://www.google.com/search?q=cats&tbm=isch',
                          [story_module.expectations.ALL_WIN],
                          'crbug.com/762165')

    return StoryExpectations()


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

  def GetExpectations(self):
    class StoryExpectations(story_module.expectations.StoryExpectations):
      def SetExpectations(self):
        self.DisableStory(
            'inbox_app.html?slide_drawer', [story_module.expectations.ALL],
            'Story contains multiple interaction records. Not supported in '
            'smoothness benchmarks.')
        self.DisableStory(
            'https://polymer-topeka.appspot.com/',
            [story_module.expectations.ALL],
            'crbug.com/780525')
    return StoryExpectations()
