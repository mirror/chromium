# Copyright 2018 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.
from telemetry.page import shared_page_state
from telemetry import story

from page_sets import top_25_smooth
from page_sets import top_pages


class RenderingDesktopPageSet(story.StorySet):
  """ Page set for measuring rendering performance on desktop. """

  def __init__(self, scroll_forever=False):
    super(RenderingDesktopPageSet, self).__init__(
        archive_data_file='data/rendering_desktop.json',
        cloud_storage_bucket=story.PARTNER_BUCKET)

    self.scroll_forever = scroll_forever

    desktop_state_class = shared_page_state.SharedDesktopPageState

    smooth_page_classes = [
        (top_25_smooth.GmailSmoothPage, 'gmail'),
        (top_25_smooth.GoogleCalendarSmoothPage, 'google_calendar'),
        (top_25_smooth.GoogleDocSmoothPage, 'google_docs'),
        (top_25_smooth.ESPNSmoothPage, 'espn'),
    ]

    for smooth_page_class, name in smooth_page_classes:
      self.AddStory(
          smooth_page_class(
              self,
              shared_page_state_class=desktop_state_class,
              name='top_25_smooth/' + name))

    non_smooth_page_classes = [
        (top_pages.GoogleWebSearchPage, 'google_web_search'),
        (top_pages.GoogleImageSearchPage, 'google_image_search'),
        (top_pages.GooglePlusPage, 'google_plus'),
        (top_pages.YoutubePage, 'youtube'),
        (top_pages.BlogspotPage, 'blogspot'),
        (top_pages.WordpressPage, 'wordpress'),
        (top_pages.FacebookPage, 'facebook'),
        (top_pages.LinkedinPage, 'linkedin'),
        (top_pages.WikipediaPage, 'wikipedia'),
        (top_pages.TwitterPage, 'twitter'),
        (top_pages.PinterestPage, 'pinterest'),
        (top_pages.WeatherPage, 'weather'),
        (top_pages.YahooGamesPage, 'yahoo_games'),
    ]

    for non_smooth_page_class, name in non_smooth_page_classes:
      self.AddStory(
          top_25_smooth.CreatePageClassWithSmoothInteractions(
              non_smooth_page_class)(
                  self,
                  shared_page_state_class=desktop_state_class,
                  name='top_25_smooth/' + name))

    other_urls = [
        # Why: #1 news worldwide (Alexa global)
        ('http://news.yahoo.com', 'yahoo_news'),
        # Why: #2 news worldwide
        ('http://www.cnn.com', 'cnn'),
        # Why: #1 world commerce website by visits; #3 commerce in the US by
        # time spent
        ('http://www.amazon.com', 'amazon'),
        # Why: #1 commerce website by time spent by users in US
        ('http://www.ebay.com', 'ebay'),
        # Why: #1 Alexa recreation
        ('http://booking.com', 'booking'),
        # Why: #1 Alexa reference
        ('http://answers.yahoo.com', 'yahoo_answers'),
        # Why: #1 Alexa sports
        ('http://sports.yahoo.com/', 'yahoo_sports'),
        # Why: top tech blog
        ('http://techcrunch.com', 'techcrunch'),
    ]

    for url, name in other_urls:
      self.AddStory(
          top_25_smooth.TopSmoothPage(url, self, name='top_25_smooth/' + name))
