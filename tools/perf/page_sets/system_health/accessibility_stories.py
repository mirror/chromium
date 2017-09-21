# Copyright 2017 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

from page_sets.system_health import platforms
from page_sets.system_health import story_tags
from page_sets.system_health import system_health_story

LONG_TEXT = """Lorem ipsum dolor sit amet, consectetur adipiscing elit. Nulla
suscipit enim ut nunc vestibulum, vitae porta dui eleifend. Donec
condimentum ante malesuada mi sodales maximus. Vivamus mauris libero,
placerat a lobortis quis, aliquam id lacus. Vestibulum at sodales
odio, sed mattis est. Ut at ligula tincidunt, consequat lectus sed,
sodales felis. Aliquam erat volutpat. Praesent id egestas ex. Donec
laoreet blandit euismod. Nullam nec ultricies libero, sed suscipit
ante. Cras nec libero sed sem mollis pellentesque."""

class _AccessibilityStory(system_health_story.SystemHealthStory):
  """Abstract base class for accessibility System Health user stories."""
  ABSTRACT_STORY = True

  def __init__(self, story_set, take_memory_measurement):
    super(_AccessibilityStory, self).__init__(
        story_set, take_memory_measurement)

  def SetExtraBrowserOptions(self, options):
    options.AppendExtraBrowserArgs(['--force-renderer-accessibility'])


class AccessibilityScrollingCodeSearchStory(_AccessibilityStory):
  """Tests scrolling an element within a page."""
  NAME = 'accessibility:load:codesearch'
  URL = 'https://cs.chromium.org/chromium/src/ui/accessibility/platform/ax_platform_node_mac.mm'

  def RunNavigateSteps(self, action_runner):
    super(AccessibilityScrollingCodeSearchStory, self).RunNavigateSteps(
        action_runner)
    action_runner.WaitForElement(text='// namespace ui')
    action_runner.ScrollElement(selector='#file_scroller', distance=1000)


class AccessibilityWikipediaStory(_AccessibilityStory):
  """Wikipedia page on Accessibility. Long, but very simple, clean layout."""
  NAME = 'accessibility:load:wikipedia'
  URL = 'https://en.wikipedia.org/wiki/Accessibility'


class AccessibilityAmazonStory(_AccessibilityStory):
  """Amazon results page. Good example of a site with a data table."""
  NAME = 'accessibility:load:amazon'
  URL = 'https://www.amazon.com/gp/offer-listing/B01IENFJ14'


class AccessibilityAriaLiveStory(_AccessibilityStory):
  """Page that demos the aria-live attribute."""
  NAME = 'accessibility:load:aria-live'
  URL = 'http://www.freedomscientific.com/Training/Surfs-Up/AriaLiveRegions.htm'


class AccessibilityAriaOwnsStory(_AccessibilityStory):
  """Page that demos the aria-owns attribute."""
  NAME = 'accessibility:load:aria-owns'
  URL = 'http://minorninth.github.io/aria-owns-demos/'


class AccessibilityContenteditableTypingStory(_AccessibilityStory):
  """Tests typing a lot of text into a contenteditable."""
  NAME = 'accessibility:typing:contenteditable'
  URL = 'https://nerget.com/contentEditableDemo.html'

  def RunNavigateSteps(self, action_runner):
    super(AccessibilityContenteditableTypingStory, self).RunNavigateSteps(
        action_runner)
    action_runner.WaitForElement(selector='div[contenteditable][style]')

    # This is in place of the user using the grabber to resize the editable
    # text area to make it bigger.
    action_runner.ExecuteJavaScript("""
        var e = document.querySelector("div[contenteditable][style]");
        e.style.width = "800px";
        e.style.height = "600px";
    """)
    action_runner.TapElement(selector='div[contenteditable][style]')

    # EnterText doesn't handle newlines for some reason.
    long_text = LONG_TEXT.replace('\n', ' ')

    # Enter some text
    action_runner.EnterText(long_text, character_delay_ms=1)

    # Move up a couple of lines and then enter it again, this causes
    # a huge amount of wrapping and re-layout
    action_runner.PressKey('Home')
    action_runner.PressKey('ArrowUp')
    action_runner.PressKey('ArrowUp')
    action_runner.EnterText(long_text, character_delay_ms=1)
