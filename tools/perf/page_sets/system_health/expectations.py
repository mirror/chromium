# Copyright 2017 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.
from telemetry.story import expectations

class SystemHealthDesktopExpectations(expectations.StoryExpectations):
  def SetExpectations(self):
    self.DisableStory('browse:news:hackernews',
                      [expectations.ALL_WIN, expectations.ALL_MAC],
                      'crbug.com/676336')
    self.DisableStory('browse:search:google', [expectations.ALL_WIN],
                      'crbug.com/673775')
    self.DisableStory('browse:tools:maps', [expectations.ALL],
                      'crbug.com/712694')
    self.DisableStory('browse:tools:earth', [expectations.ALL],
                      'crbug.com/708590')
    self.DisableStory('load:games:miniclip', [expectations.ALL_MAC],
                      'crbug.com/664661')
    self.DisableStory('play:media:google_play_music', [expectations.ALL],
                      'crbug.com/649392')
    self.DisableStory('play:media:soundcloud', [expectations.ALL_WIN],
                      'crbug.com/649392')
    self.DisableStory('play:media:pandora', [expectations.ALL],
                      'crbug.com/64939')


class SystemHealthMobileExpectations(expectations.StoryExpectations):
  def SetExpectations(self):
    self.DisableStory('background:tools:gmail', [expectations.ALL_ANDROID],
                      'crbug.com/723783')
    self.DisableStory('browse:shopping:flipkart', [expectations.ALL_ANDROID],
                      'crbug.com/708300')
    self.DisableStory('browse:news:globo', [expectations.ALL_ANDROID],
                      'crbug.com/714650')
    self.DisableStory('load:tools:gmail', [expectations.ALL_ANDROID],
                      'crbug.com/657433')
    self.DisableStory('long_running:tools:gmail-background',
                      [expectations.ALL_ANDROID], 'crbug.com/657433')
