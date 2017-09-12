# Copyright 2017 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.
from telemetry.page import page as page_module
from telemetry.page import shared_page_state
from telemetry import story

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

class SimplePage(page_module.Page):
  def __init__(self, url, page_set, name):
    super(SimplePage, self).__init__(
        url, page_set=page_set, name=name,
        shared_page_state_class=shared_page_state.SharedDesktopPageState)

  def RunPageInteractions(self, action_runner):
    pass

class ContenteditableTyping(SimplePage):
  def __init__(self, page_set):
    super(ContenteditableTyping, self).__init__(
      url='https://nerget.com/contentEditableDemo.html',
      page_set=page_set,
      name='Contenteditable Typing')

  def RunNavigateSteps(self, action_runner):
    super(ContenteditableTyping, self).RunNavigateSteps(action_runner)
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

class ScrollingCodeSearch(SimplePage):
  def __init__(self, page_set):
    super(ScrollingCodeSearch, self).__init__(
      url='https://cs.chromium.org/chromium/src/' +
          'ui/accessibility/platform/ax_platform_node_win.cc',
      page_set=page_set,
      name='Scrolling Chromium Code Search Page')

  def RunNavigateSteps(self, action_runner):
    super(ScrollingCodeSearch, self).RunNavigateSteps(action_runner)
    action_runner.WaitForElement(text='// namespace ui')
    action_runner.ScrollElement(selector='#file_scroller', distance=1000)

class AccessibilityPageSet(story.StorySet):
  """Sites to exercise accessibility"""

  def __init__(self):
    super(AccessibilityPageSet, self).__init__(
      archive_data_file='data/accessibility.json',
      cloud_storage_bucket=story.PARTNER_BUCKET)

    # Wikipedia page on Accessibility. Long, but very simple, clean layout.
    self.AddStory(SimplePage(
        'https://en.wikipedia.org/wiki/Accessibility', self,
        name='Wikipedia'))

    # Amazon results page. Good example of a site with a data table.
    self.AddStory(SimplePage(
        'https://www.amazon.com/gp/offer-listing/B01IENFJ14', self,
        name='Amazon'))

    # Freedom Scientific page that demos the aria-live attribute.
    self.AddStory(SimplePage(
        'http://www.freedomscientific.com/' +
        'Training/Surfs-Up/AriaLiveRegions.htm', self,
        name='Freedom Scientific Live Regions Demo'))

    # Page that demos the aria-owns attribute.
    self.AddStory(SimplePage(
        'http://minorninth.github.io/aria-owns-demos/', self,
        name='Aria-owns Demo'))

    # Tests typing a lot of text into a contenteditable.
    self.AddStory(ContenteditableTyping(self))

    # Tests scrolling an element within a page.
    self.AddStory(ScrollingCodeSearch(self))
