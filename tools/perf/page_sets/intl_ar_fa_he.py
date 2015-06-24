# Copyright 2014 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.
from telemetry.page import page as page_module
from telemetry.page import page_set as page_set_module
from telemetry.page import shared_page_state


class IntlArFaHePage(page_module.Page):

  def __init__(self, url, page_set):
    super(IntlArFaHePage, self).__init__(
        url=url, page_set=page_set,
        shared_page_state_class=shared_page_state.SharedDesktopPageState)
    self.archive_data_file = 'data/intl_ar_fa_he.json'


class IntlArFaHePageSet(page_set_module.PageSet):

  """ Popular pages in right-to-left languages Arabic, Farsi and Hebrew. """

  def __init__(self):
    super(IntlArFaHePageSet, self).__init__(
      archive_data_file='data/intl_ar_fa_he.json',
      bucket=page_set_module.PARTNER_BUCKET)

    urls_list = [
      'http://msn.co.il/',
      'http://ynet.co.il/',
      'http://www.islamweb.net/',
      'http://farsnews.com/',
      'http://www.masrawy.com/',
      'http://www.startimes.com/f.aspx',
      'http://www.aljayyash.net/',
      'http://www.google.com.sa/'
    ]

    for url in urls_list:
      self.AddUserStory(IntlArFaHePage(url, self))
