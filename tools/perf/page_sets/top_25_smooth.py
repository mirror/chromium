# Copyright 2014 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.
from telemetry.page import page as page_module
from telemetry.page import shared_page_state
from telemetry import story

from page_sets import top_pages

class TopSmoothScrollLogic(object):

  """Define extra arguments to pass to ScrollPage or ScrollElement"""
  _scroll_element_args = {}
  _scroll_page_args = {}

  def RunPageInteractions(self, action_runner):
    self.InitializePageInteractions(action_runner)
    self.ScrollPageInteractions(action_runner)

  def InitializePageInteractions(self, action_runner):
    """Override this to define e.g. login actions"""
    pass

  def ScrollPageInteractions(self, action_runner):
    action_runner.Wait(1)
    with action_runner.CreateGestureInteraction('ScrollAction'):
      self.Scroll(action_runner, 'down')
      if self.story_set.scroll_forever:
        while True:
          self.Scroll(action_runner, 'up')
          self.Scroll(action_runner, 'down')

  def Scroll(self, action_runner, direction):
    if self._scroll_element_args:
      action_runner.ScrollElement(direction=direction,
                                  **self._scroll_element_args)
    else:
      action_runner.ScrollPage(direction=direction, **self._scroll_page_args)


def _CreatePageClassWithSmoothInteractions(page_cls=None):

  class DerivedSmoothPage(TopSmoothScrollLogic,
                          page_cls):  # pylint: disable=no-init
    pass

  return DerivedSmoothPage


class TopSmoothPage(TopSmoothScrollLogic, page_module.Page):

  def __init__(self, url, page_set, name=''):
    if name == '':
      name = url
    super(TopSmoothPage, self).__init__(
        url=url, page_set=page_set, name=name,
        shared_page_state_class=shared_page_state.SharedDesktopPageState)


class GmailSmoothPage(TopSmoothScrollLogic, top_pages.GmailPage):

  """ Why: productivity, top google properties """

  _scroll_element_args = { 'element_function':
                           'window.__scrollableElementForTelemetry' }

  def __init__(self, page_set,
               shared_page_state_class=shared_page_state.SharedPageState):
    super(GmailSmoothPage, self).__init__(
        page_set=page_set,
        shared_page_state_class=shared_page_state_class)

  def InitializePageInteractions(self, action_runner):
    action_runner.ExecuteJavaScript('''
        gmonkey.load('2.0', function(api) {
          window.__scrollableElementForTelemetry = api.getScrollableElement();
        });''')
    action_runner.WaitForJavaScriptCondition(
        'window.__scrollableElementForTelemetry != null')


class GoogleCalendarSmoothPage(TopSmoothScrollLogic,
                               top_pages.GoogleCalendarPage):

  """ Why: productivity, top google properties """
  _scroll_element_args = { 'selector': '#scrolltimedeventswk' }


class GoogleDocSmoothPage(TopSmoothScrollLogic, top_pages.GoogleDocPage):

  """ Why: productivity, top google properties; Sample doc in the link """
  _scroll_element_args = { 'selector': '.kix-appview-editor' }


class ESPNSmoothPage(TopSmoothScrollLogic, top_pages.ESPNPage):

  """ Why: #1 sports """
  _scroll_page_args = { 'left_start_ratio': 0.1 }


class Top25SmoothPageSet(story.StorySet):

  """ Pages hand-picked for 2012 CrOS scrolling tuning efforts. """

  def __init__(self, techcrunch=True, scroll_forever=False):
    super(Top25SmoothPageSet, self).__init__(
        archive_data_file='data/top_25_smooth.json',
        cloud_storage_bucket=story.PARTNER_BUCKET)

    self.scroll_forever = scroll_forever
    desktop_state_class = shared_page_state.SharedDesktopPageState

    self.AddStory(_CreatePageClassWithSmoothInteractions(
        top_pages.GoogleWebSearchPage)(self, desktop_state_class))
    self.AddStory(GmailSmoothPage(self, desktop_state_class))
    self.AddStory(GoogleCalendarSmoothPage(self, desktop_state_class))
    self.AddStory(_CreatePageClassWithSmoothInteractions(
        top_pages.GoogleImageSearchPage)(self, desktop_state_class))
    self.AddStory(GoogleDocSmoothPage(self, desktop_state_class))
    self.AddStory(_CreatePageClassWithSmoothInteractions(
        top_pages.GooglePlusPage)(self, desktop_state_class))
    self.AddStory(_CreatePageClassWithSmoothInteractions(
        top_pages.YoutubePage)(self, desktop_state_class))
    self.AddStory(_CreatePageClassWithSmoothInteractions(
        top_pages.BlogspotPage)(self, desktop_state_class))
    self.AddStory(_CreatePageClassWithSmoothInteractions(
        top_pages.WordpressPage)(self, desktop_state_class))
    self.AddStory(_CreatePageClassWithSmoothInteractions(
        top_pages.FacebookPage)(self, desktop_state_class))
    self.AddStory(_CreatePageClassWithSmoothInteractions(
        top_pages.LinkedinPage)(self, desktop_state_class))
    self.AddStory(_CreatePageClassWithSmoothInteractions(
        top_pages.WikipediaPage)(self, desktop_state_class))
    self.AddStory(_CreatePageClassWithSmoothInteractions(
        top_pages.TwitterPage)(self, desktop_state_class))
    self.AddStory(_CreatePageClassWithSmoothInteractions(
        top_pages.PinterestPage)(self, desktop_state_class))
    self.AddStory(ESPNSmoothPage(self, desktop_state_class))
    self.AddStory(_CreatePageClassWithSmoothInteractions(
        top_pages.WeatherPage)(self, desktop_state_class))
    self.AddStory(_CreatePageClassWithSmoothInteractions(
        top_pages.YahooGamesPage)(self, desktop_state_class))

    other_urls = [
        # Why: #1 news worldwide (Alexa global)
        'http://news.yahoo.com',
        # Why: #2 news worldwide
        'http://www.cnn.com',
        # Why: #1 world commerce website by visits; #3 commerce in the US by
        # time spent
        'http://www.amazon.com',
        # Why: #1 commerce website by time spent by users in US
        'http://www.ebay.com',
        # Why: #1 Alexa recreation
        'http://booking.com',
        # Why: #1 Alexa reference
        'http://answers.yahoo.com',
        # Why: #1 Alexa sports
        'http://sports.yahoo.com/',
    ]

    if techcrunch:
      # Why: top tech blog
      other_urls.append('http://techcrunch.com')

    for url in other_urls:
      self.AddStory(TopSmoothPage(url, self))
