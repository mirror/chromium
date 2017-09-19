# Copyright 2017 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

from page_sets.system_health import platforms
from page_sets.system_health import story_tags
from page_sets.system_health import system_health_story

# The preamble to the U.S. Declaration of Independence. Just an example
# of a large amount of text to enter on a web page.
LONG_TEXT = """
We hold these truths to be self-evident, that all men are created
equal, that they are endowed, by their Creator, with certain
unalienable Rights, that among these are Life, Liberty, and the
pursuit of Happiness.

That to secure these rights, Governments are instituted among Men,
deriving their just powers from the consent of the governed, That
whenever any Form of Government becomes destructive of these ends, it
is the Right of the People to alter or abolish it, and to institute
new Government, laying its foundation on such principles, and
organizing its powers in such form, as to them shall seem most likely
to effect their Safety and Happiness.

Prudence, indeed, will dictate that Governments long established
should not be changed for light and transient causes; and accordingly
all experience hath shewn, that mankind are more disposed to suffer,
while evils are sufferable, than to right themselves by abolishing the
forms to which they are accustomed. But when a long train of abuses
and usurpations, pursuing invariably the same Object, evinces a design
to reduce them under absolute Despotism, it is their right, it is
their duty, to throw off such Government, and to provide new Guards
for their future security.
"""

class _AccessibilityStory(system_health_story.SystemHealthStory):
  """Abstract base class for accessibility System Health user stories."""
  ABSTRACT_STORY = True
  EXTRA_BROWSER_ARGS = ['--force-renderer-accessibility']

  def __init__(self, story_set, take_memory_measurement, tabset_repeat=1):
    super(_AccessibilityStory, self).__init__(
        story_set, take_memory_measurement)

  @classmethod
  def GenerateStoryDescription(cls):
    return 'Load %s with accessibility enabled' % cls.URL

# Tests scrolling an element within a page.
class AccessibilityScrollingCodeSearchStory(_AccessibilityStory):
  NAME = 'accessibility:load:codesearch'
  URL = ('https://cs.chromium.org/chromium/src/' +
         'ui/accessibility/platform/ax_platform_node_mac.mm')

  def RunNavigateSteps(self, action_runner):
    super(AccessibilityScrollingCodeSearchStory, self).RunNavigateSteps(
        action_runner)
    action_runner.WaitForElement(text='// namespace ui')
    action_runner.ScrollElement(selector='#file_scroller', distance=1000)

# Wikipedia page on Accessibility. Long, but very simple, clean layout.
class AccessibilityWikipediaStory(_AccessibilityStory):
  NAME = 'accessibility:load:wikipedia'
  URL = 'https://en.wikipedia.org/wiki/Accessibility'

# Amazon results page. Good example of a site with a data table.
class AccessibilityAmazonStory(_AccessibilityStory):
  NAME = 'accessibility:load:amazon'
  URL = 'https://www.amazon.com/gp/offer-listing/B01IENFJ14'

# Page that demos the aria-live attribute.
class AccessibilityAriaLiveStory(_AccessibilityStory):
  NAME = 'accessibility:load:aria-live'
  URL = ('http://www.freedomscientific.com/' +
         'Training/Surfs-Up/AriaLiveRegions.htm')

# Page that demos the aria-owns attribute.
class AccessibilityAriaOwnsStory(_AccessibilityStory):
  NAME = 'accessibility:load:aria-owns'
  URL = 'http://minorninth.github.io/aria-owns-demos/'

# Tests typing a lot of text into a contenteditable.
class AccessibilityContenteditableTypingStory(_AccessibilityStory):
  NAME = 'accessibility:typing:contenteditable'
  URL = 'https://nerget.com/contentEditableDemo.html'

  def RunNavigateSteps(self, action_runner):
    super(AccessibilityContenteditableTypingStory, self).RunNavigateSteps(
        action_runner)
    action_runner.WaitForElement(selector='div[contenteditable][style]')
    action_runner.ExecuteJavaScript(
        'var e = document.querySelector("div[contenteditable][style]"); ' +
        'e.style.width = "800px"; ' +
        'e.style.height = "600px";')
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
