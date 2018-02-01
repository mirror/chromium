# Copyright 2014 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.
from telemetry.page import page as page_module
from telemetry.page import shared_page_state
from telemetry import story

from page_sets import key_mobile_sites_pages


def _IssueMarkerAndScroll(action_runner):
  with action_runner.CreateGestureInteraction('ScrollAction'):
    action_runner.ScrollPage()


def _CreatePageClassWithSmoothInteractions(page_cls):

  class DerivedSmoothPage(page_cls):  # pylint: disable=no-init

    def RunPageInteractions(self, action_runner):
      _IssueMarkerAndScroll(action_runner)

  return DerivedSmoothPage


class KeyMobileSitesSmoothPage(page_module.Page):

  def __init__(self,
               url,
               page_set,
               name='',
               tags=None,
               action_on_load_complete=False,
               extra_browser_args=None,
               shared_page_state_class=shared_page_state.SharedMobilePageState):
    if name == '':
      name = url
    super(KeyMobileSitesSmoothPage, self).__init__(
        url=url,
        page_set=page_set,
        name=name,
        tags=tags,
        extra_browser_args=extra_browser_args,
        shared_page_state_class=shared_page_state_class)
    self.action_on_load_complete = action_on_load_complete

  def RunPageInteractions(self, action_runner):
    if self.action_on_load_complete:
      action_runner.WaitForJavaScriptCondition(
          'document.readyState == "complete"', timeout=30)
    _IssueMarkerAndScroll(action_runner)


class LinkedInSmoothPage(key_mobile_sites_pages.LinkedInPage):
  # Linkedin has expensive shader compilation so it can benefit from shader
  # cache from reload.
  def RunNavigateSteps(self, action_runner):
    super(LinkedInSmoothPage, self).RunNavigateSteps(action_runner)
    action_runner.ScrollPage()
    super(LinkedInSmoothPage, self).RunNavigateSteps(action_runner)


class WowwikiSmoothPage(KeyMobileSitesSmoothPage):
  """Why: Mobile wiki."""

  def __init__(self,
               page_set,
               name='',
               extra_browser_args=None,
               shared_page_state_class=shared_page_state.SharedMobilePageState):
    super(WowwikiSmoothPage, self).__init__(
        url='http://www.wowwiki.com/World_of_Warcraft:_Mists_of_Pandaria',
        page_set=page_set,
        name=name,
        extra_browser_args=extra_browser_args,
        shared_page_state_class=shared_page_state_class)

  # Wowwiki has expensive shader compilation so it can benefit from shader
  # cache from reload.
  def RunNavigateSteps(self, action_runner):
    super(WowwikiSmoothPage, self).RunNavigateSteps(action_runner)
    action_runner.ScrollPage()
    super(WowwikiSmoothPage, self).RunNavigateSteps(action_runner)


class GoogleNewsMobile2SmoothPage(key_mobile_sites_pages.GoogleNewsMobile2Page):

  def RunPageInteractions(self, action_runner):
    with action_runner.CreateGestureInteraction('ScrollAction'):
      action_runner.ScrollElement(
          element_function='document.getElementById(":5")',
          distance_expr="""
              Math.max(0, 2500 +
                  document.getElementById(':h').getBoundingClientRect().top)""",
          use_touch=True)


class AmazonNicolasCageSmoothPage(key_mobile_sites_pages.AmazonNicolasCagePage):

  def RunPageInteractions(self, action_runner):
    with action_runner.CreateGestureInteraction('ScrollAction'):
      action_runner.ScrollElement(
          selector='#search',
          distance_expr='document.body.scrollHeight - window.innerHeight')


class CNNArticleSmoothPage(key_mobile_sites_pages.CnnArticlePage):

  def RunPageInteractions(self, action_runner):
    with action_runner.CreateGestureInteraction('ScrollAction'):
      # With default top_start_ratio=0.5 the corresponding element in this page
      # will not be in the root scroller.
      action_runner.ScrollPage(top_start_ratio=0.01)


def AddPagesToPageSet(
    page_set,
    shared_page_state_class=shared_page_state.SharedMobilePageState,
    name_prefix='',
    extra_browser_args=None):
  # Add pages with predefined classes that contain custom navigation logic.
  predefined_page_classes = [
      (key_mobile_sites_pages.CapitolVolkswagenPage, 'capitolvolkswagen'),
      (key_mobile_sites_pages.TheVergeArticlePage, 'theverge_article'),
      (key_mobile_sites_pages.FacebookPage, 'facebook'),
      (key_mobile_sites_pages.YoutubeMobilePage, 'youtube'),
      (key_mobile_sites_pages.YahooAnswersPage, 'yahoo_answers'),
      (key_mobile_sites_pages.GoogleNewsMobilePage, 'google_news_mobile'),
  ]
  for page_class, page_name in predefined_page_classes:
    page_set.AddStory(
        _CreatePageClassWithSmoothInteractions(page_class)(
            page_set,
            shared_page_state_class=shared_page_state_class,
            name=name_prefix + page_name,
            extra_browser_args=extra_browser_args))

  # Add pages with custom page interaction logic.
  smooth_interaction_page_classes = [
    (LinkedInSmoothPage, 'linkedin'),
    (WowwikiSmoothPage, 'wowwiki'),
    (GoogleNewsMobile2SmoothPage, 'google_news_mobile2'),
    (AmazonNicolasCageSmoothPage, 'amazon'),
    (CNNArticleSmoothPage, 'cnn_article'),
  ]

  for page_class, page_name in smooth_interaction_page_classes:
    page_set.AddStory(
        page_class(
            page_set,
            shared_page_state_class=shared_page_state_class,
            name=name_prefix + page_name,
            extra_browser_args=extra_browser_args))

  # Add pages with custom tags.
  fastpath_urls = [
      # Why: Top news site.
      ('http://nytimes.com/', 'nytimes'),
      # Why: Image-heavy site.
      ('http://cuteoverload.com', 'cuteoverload'),
      # Why: #5 Alexa news.
      ('http://www.reddit.com/r/programming/comments/1g96ve', 'reddit'),
      # Why: Problematic use of fixed position elements.
      ('http://www.boingboing.net', 'boingboing'),
      # Why: crbug.com/169827
      ('http://slashdot.org', 'slashdot'),
  ]

  for page_url, page_name in fastpath_urls:
    page_set.AddStory(
        KeyMobileSitesSmoothPage(
            page_url,
            page_set,
            shared_page_state_class=shared_page_state_class,
            name=name_prefix + page_name,
            tags=['fastpath'],
            extra_browser_args=extra_browser_args))

  # Add simple pages with no custom navigation logic or tags.
  urls_list = [
      # Why: #11 (Alexa global), google property; some blogger layouts
      # have infinite scroll but more interesting.
      ('http://googlewebmastercentral.blogspot.com/', 'blogger'),
      # Why: #18 (Alexa global), Picked an interesting post
      ('http://en.blog.wordpress.com/2012/09/04/freshly-pressed-editors-picks-for-august-2012/',
       'wordpress'),
      # Why: #6 (Alexa) most visited worldwide, picked an interesting page
      ('http://en.wikipedia.org/wiki/Wikipedia', 'wikipedia'),
      # Why: #8 (Alexa global), picked an interesting page
      ('http://twitter.com/katyperry', 'twitter'),
      # Why: #37 (Alexa global).
      ('http://pinterest.com', 'pinterest'),
      # Why: #1 sports.
      ('http://espn.go.com', 'espn'),
      # Why: crbug.com/231413
      ('http://forecast.io', 'forecast.io'),
      # Why: Social; top Google property; Public profile; infinite scrolls.
      ('https://plus.google.com/app/basic/110031535020051778989/posts?source=apppromo',
       'google_plus_infinite_scroll'),
      # Why: crbug.com/242544
      # pylint: disable=line-too-long
      ('http://www.androidpolice.com/2012/10/03/rumor-evidence-mounts-that-an-lg-optimus-g-nexus-is-coming-along-with-a-nexus-phone-certification-program/',
       'androidpolice'),
      # Why: crbug.com/149958
      ('http://gsp.ro', 'gsp.ro'),
      # Why: Top tech blog
      ('http://theverge.com', 'theverge'),
      # Why: Top tech site
      ('http://digg.com', 'digg'),
      # Why: Top Google property; a Google tab is often open
      ('https://www.google.co.uk/search?hl=en&q=barack+obama&cad=h',
       'google_web_search'),
      # Why: #1 news worldwide (Alexa global)
      ('http://news.yahoo.com', 'yahoo_news'),
      # Why: #2 news worldwide
      ('http://www.cnn.com', 'cnn'),
      # Why: #1 commerce website by time spent by users in US
      ('http://shop.mobileweb.ebay.com/searchresults?kw=viking+helmet', 'ebay'),
      # Why: #1 Alexa recreation
      # pylint: disable=line-too-long
      ('http://www.booking.com/searchresults.html?src=searchresults&latitude=65.0500&longitude=25.4667',
       'booking.com'),
      # Why: Top tech blog
      ('http://techcrunch.com', 'techcrunch'),
      # Why: #6 Alexa sports
      ('http://mlb.com/', 'mlb'),
      # Why: #14 Alexa California
      ('http://www.sfgate.com/', 'sfgate'),
      # Why: Non-latin character set
      ('http://worldjournal.com/', 'worldjournal'),
      # Why: #15 Alexa news
      ('http://online.wsj.com/home-page', 'wsj'),
      # Why: Image-heavy mobile site
      ('http://www.deviantart.com/', 'deviantart'),
      # Why: Top search engine
      # pylint: disable=line-too-long
      ('http://www.baidu.com/s?wd=barack+obama&rsv_bp=0&rsv_spt=3&rsv_sug3=9&rsv_sug=0&rsv_sug4=3824&rsv_sug1=3&inputT=4920',
       'baidu'),
      # Why: Top search engine
      ('http://www.bing.com/search?q=sloths', 'bing'),
      # Why: Good example of poor initial scrolling
      ('http://ftw.usatoday.com/2014/05/spelling-bee-rules-shenanigans',
       'usatoday'),
  ]

  for page_url, page_name in urls_list:
    page_set.AddStory(
        KeyMobileSitesSmoothPage(
            page_url,
            page_set,
            shared_page_state_class=shared_page_state_class,
            name=name_prefix + page_name,
            extra_browser_args=extra_browser_args))

  # Why: Wikipedia page with a delayed scroll start
  page_set.AddStory(
      KeyMobileSitesSmoothPage(
          'http://en.wikipedia.org/wiki/Wikipedia',
          page_set,
          shared_page_state_class=shared_page_state_class,
          name=name_prefix + 'wikipedia_delayed_scroll_start',
          action_on_load_complete=True,
          extra_browser_args=extra_browser_args))


class KeyMobileSitesSmoothPageSet(story.StorySet):
  """ Key mobile sites with smooth interactions. """

  def __init__(self):
    super(KeyMobileSitesSmoothPageSet, self).__init__(
        archive_data_file='data/key_mobile_sites_smooth.json',
        cloud_storage_bucket=story.PARTNER_BUCKET)

    AddPagesToPageSet(self)
