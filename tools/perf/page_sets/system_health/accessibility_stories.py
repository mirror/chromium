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
        story_set, take_memory_measurement,
        extra_browser_args=['--force-renderer-accessibility'])


class AccessibilityScrollingCodeSearchStory(_AccessibilityStory):
  """Tests scrolling an element within a page."""
  NAME = 'browse_accessibility:tech:codesearch'
  URL = 'https://cs.chromium.org/chromium/src/ui/accessibility/platform/ax_platform_node_mac.mm'
  TAGS = [story_tags.ACCESSIBILITY, story.tags.SCROLL]

  def RunNavigateSteps(self, action_runner):
    super(AccessibilityScrollingCodeSearchStory, self).RunNavigateSteps(
        action_runner)
    action_runner.WaitForElement(text='// namespace ui')
    action_runner.ScrollElement(selector='#file_scroller', distance=1000)


class AccessibilityWikipediaStory(_AccessibilityStory):
  """Wikipedia page on Accessibility. Long, but very simple, clean layout."""
  NAME = 'load_accessibility:media:wikipedia'
  URL = 'https://en.wikipedia.org/wiki/Accessibility'
  TAGS = [story_tags.ACCESSIBILITY]


class AccessibilityAmazonStory(_AccessibilityStory):
  """Amazon results page. Good example of a site with a data table."""
  NAME = 'load_accessibility:shopping:amazon'
  URL = 'https://www.amazon.com/gp/offer-listing/B01IENFJ14'
  TAGS = [story_tags.ACCESSIBILITY]


class AccessibilityAriaLiveStory(_AccessibilityStory):
  """Page that demos the aria-live attribute."""
  NAME = 'load_accessibility:tech:aria-live'
  URL = 'http://www.freedomscientific.com/Training/Surfs-Up/AriaLiveRegions.htm'
  TAGS = [story_tags.ACCESSIBILITY]


class AccessibilityAriaOwnsStory(_AccessibilityStory):
  """Page that demos the aria-owns attribute."""
  NAME = 'load_accessibility:tech:aria-owns'
  URL = 'http://minorninth.github.io/aria-owns-demos/'
  TAGS = [story_tags.ACCESSIBILITY]


class AccessibilityContenteditableTypingStory(_AccessibilityStory):
  """Tests typing a lot of text into a contenteditable."""
  NAME = 'browse_accessibility:tech:contenteditable'
  URL = 'https://nerget.com/contentEditableDemo.html'
  TAGS = [story_tags.ACCESSIBILITY, story_tags.KEYBOARD_INPUT]

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
