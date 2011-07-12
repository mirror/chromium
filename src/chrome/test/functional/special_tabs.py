#!/usr/bin/python
# Copyright (c) 2011 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import logging
import os

import pyauto_functional  # Must be imported before pyauto
import pyauto
import test_utils

class SpecialTabsTest(pyauto.PyUITest):
  """TestCase for Special Tabs like about:version, chrome://history, etc."""

  special_accelerator_tabs = {
    pyauto.IDC_SHOW_HISTORY: 'History',
    pyauto.IDC_MANAGE_EXTENSIONS: 'Extensions',
    pyauto.IDC_SHOW_DOWNLOADS: 'Downloads',
  }

  special_url_redirects = {
    'about:': 'chrome://version',
    'about:about': 'chrome://about',
    'about:appcache-internals': 'chrome://appcache-internals',
    'about:credits': 'chrome://credits',
    'about:histograms': 'chrome://histograms',
    'about:plugins': 'chrome://plugins',
    'about:sync': 'chrome://sync-internals',
    'about:sync-internals': 'chrome://sync-internals',
    'about:version': 'chrome://version',
  }

  special_url_tabs = {
    'chrome://about': { 'title': 'Chrome URLs' },
    'chrome://appcache-internals': {
      'title': 'AppCache Internals',
      'CSP': False
    },
    'chrome://blob-internals': {
      'title': 'Blob Storage Internals',
      'CSP': False
    },
    'chrome://bugreport': {
      'title': 'Whoops. Let\'s fix that.',
      'CSP': False
    },
    'chrome://chrome-urls': { 'title': 'Chrome URLs' },
    'chrome://crashes': { 'title': 'Crashes' },
    'chrome://credits': { 'title': 'Credits', 'CSP': False },
    'chrome://downloads': { 'title': 'Downloads' },
    'chrome://extensions': { 'title': 'Extensions', 'CSP': False },
    'chrome://flags': {},
    'chrome://flash': {},
    'chrome://gpu-internals': { 'CSP': False },
    'chrome://histograms': { 'title': 'About Histograms' },
    'chrome://history': { 'title': 'History' },
    'chrome://history2': { 'title': 'History', 'CSP': False },
    'chrome://media-internals': { 'title': 'Media Internals', 'CSP': False },
    'chrome://memory-redirect': { 'title': 'About Memory' },
    'chrome://net-internals': { 'CSP': False },
    'chrome://newtab': { 'title': 'New Tab', 'CSP': False },
    'chrome://plugins': { 'title': 'Plug-ins' },
    'chrome://sessions': { 'title': 'Sessions', 'CSP': False },
    'chrome://settings': { 'title': 'Preferences - Basics' },
    'chrome://stats': { 'CSP': False },
    'chrome://sync': { 'title': 'Sync Internals', 'CSP': False },
    'chrome://sync-internals': { 'title': 'Sync Internals', 'CSP': False },
    'chrome://terms': { 'CSP': False },
    'chrome://textfields': { 'title': 'chrome://textfields', 'CSP': False },
    'chrome://version': { 'title': 'About Version' },
    'chrome://view-http-cache': {  'CSP': False },
  }

  broken_special_url_tabs = {
    # crashes under debug, bad include of locale.js (bug 88220).
    'chrome://taskmanager': { 'CSP': False },

    # crashed under debug when invoked from location bar (bug 88223).
    'chrome://devtools': { 'CSP': False },

    # intermittent crash on cromeos=1 on linux
    'chrome://tasks': { 'title': 'About Histograms' },

    # returns "not available" despite having an URL constant.
    'chrome://dialog': { 'CSP': False },

    # separate window on mac, PC untested, not implemented elsewhere.
    'chrome://ipc': { 'CSP': False },

    # race against redirects via meta-refresh.
    'chrome://memory': { 'CSP': False },
  }

  chromeos_special_url_tabs = {
    'chrome://active-downloads': { 'title': 'Downloads', 'CSP': False },
    'chrome://choose-mobile-network': { 'title': 'undefined', 'CSP': False },
    'chrome://imageburner': { 'title':'Create a Recovery Media', 'CSP': False },
    'chrome://keyboardoverlay': { 'title': 'Keyboard Overlay', 'CSP': False },
    'chrome://login': { 'CSP': False },
    'chrome://network': { 'title': 'About Network' },
    'chrome://oobe': { 'title': 'undefined', 'CSP': False },
    'chrome://os-credits': { 'title': 'Credits', 'CSP': False },
    'chrome://proxy-settings': { 'CSP': False },
    'chrome://register': { 'CSP': False },
    'chrome://sim-unlock': { 'title': 'Enter SIM Card PIN', 'CSP': False },
    'chrome://system': { 'title': 'About System', 'CSP': False },

    # OVERRIDE - usually a warning page without CSP (so far).
    'chrome://flags': { 'CSP': False },

    # OVERRIDE - title and page different on CrOS
    'chrome://settings': { 'title': 'Settings - Basics' },
  }

  broken_chromeos_special_url_tabs = {
    # returns "not available" page on chromeos=1 linux but has an URL constant.
    'chrome://activationmessage': { 'CSP': False },
    'chrome://cloudprintresources': { 'CSP': False },
    'chrome://cloudprintsetup': { 'CSP': False },
    'chrome://collected-cookies': { 'CSP': False },
    'chrome://constrained-test': { 'CSP': False },
    'chrome://enterprise-enrollment': { 'CSP': False },
    'chrome://http-auth': { 'CSP': False },
    'chrome://login-container': { 'CSP': False },
    'chrome://media-player': { 'CSP': False },
    'chrome://screenshots': { 'CSP': False },
    'chrome://slideshow': { 'CSP': False },
    'chrome://syncresources': { 'CSP': False },
    'chrome://theme': { 'CSP': False },

    # crashes on chromeos=1 on linux, possibly missing real CrOS features.
    'chrome://cryptohome': { 'CSP': False},
    'chrome://mobilesetup': { 'CSP': False },
    'chrome://print': { 'CSP': False },
  }

  linux_special_url_tabs = {
    'chrome://linux-proxy-config': { 'title': 'Proxy Configuration Help' },
    'chrome://tcmalloc': { 'title': 'About tcmalloc' },
    'chrome://sandbox': { 'title': 'Sandbox Status' },
  }

  broken_linux_special_url_tabs = {
  }

  mac_special_url_tabs = {
  }

  broken_mac_special_url_tabs = {
  }

  win_special_url_tabs = {
    'chrome://conflicts': { 'CSP': False },

    # OVERRIDE - different title for page.
    'chrome://settings': { 'title': 'Options - Basics' },
  }

  broken_win_special_url_tabs = {
    # Sync on windows badly broken at the moment.
    'chrome://sync': {},
  }

  google_chrome_special_url_tabs = {
    # OVERRIDE - different title for Google Chrome vs. Chromium.
    'chrome://terms': {
      'title': 'Google Chrome Terms of Service',
      'CSP': False
    },
  }

  broken_google_chrome_special_url_tabs = {
  }

  def _VerifyAppCacheInternals(self):
    """Confirm about:appcache-internals contains expected content for Caches.
       Also confirms that the about page populates Application Caches."""
    # Navigate to html page to activate DNS prefetching.
    self.NavigateToURL('http://static.webvm.net/appcache-test/simple.html')
    # Wait for page to load and display sucess or fail message.
    self.WaitUntil(
        lambda: self.GetDOMValue('document.getElementById("result").innerHTML'),
                                 expect_retval='SUCCESS')
    self.GetBrowserWindow(0).GetTab(0).GoBack()
    test_utils.StringContentCheck(self, self.GetTabContents(),
                                  ['Manifest', 'Remove this AppCache'],
                                  [])

  def _VerifyAboutDNS(self):
    """Confirm about:dns contains expected content related to DNS info.
       Also confirms that prefetching DNS records propogate."""
    # Navigate to a page to activate DNS prefetching.
    self.NavigateToURL('http://www.google.com')
    self.GetBrowserWindow(0).GetTab(0).GoBack()
    test_utils.StringContentCheck(self, self.GetTabContents(),
                                  ['Host name', 'How long ago', 'Motivation'],
                                  [])

  def _GetPlatformSpecialURLTabs(self):
    tabs = self.special_url_tabs.copy()
    broken_tabs = self.broken_special_url_tabs.copy()
    if self.IsChromeOS():
      tabs.update(self.chromeos_special_url_tabs)
      broken_tabs.update(self.broken_chromeos_special_url_tabs)
    elif self.IsLinux():
      tabs.update(self.linux_special_url_tabs)
      broken_tabs.update(self.broken_linux_special_url_tabs)
    elif self.IsMac():
      tabs.update(self.mac_special_url_tabs)
      broken_tabs.update(self.broken_mac_special_url_tabs)
    elif self.IsWin():
      tabs.update(self.win_special_url_tabs)
      broken_tabs.update(self.broken_win_special_url_tabs)
    if self.GetBrowserInfo()['properties']['branding'] == 'Google Chrome':
      tabs.update(self.google_chrome_special_url_tabs)
      broken_tabs.update(self.broken_google_chrome_special_url_tabs)
    for key, value in broken_tabs.iteritems():
      if key in tabs:
       del tabs[key]
    return tabs

  def testSpecialURLRedirects(self):
    """Test that older about: URLs are implemented by newer chrome:// URLs.
       The location bar may not get updated in all cases, so checking the
       tab URL is misleading, instead check for the same contents as the
       chrome:// page."""
    tabs = self._GetPlatformSpecialURLTabs()
    for url, redirect in self.special_url_redirects.iteritems():
      if redirect in tabs:
        logging.debug('Testing redirect from %s to %s.' % (url, redirect))
        self.NavigateToURL(url)
        self.assertEqual(self.special_url_tabs[redirect]['title'],
                         self.GetActiveTabTitle())

  def testSpecialURLTabs(self):
    """Test special tabs created by URLs like chrome://downloads,
       chrome://extensions, chrome://history etc.  Also ensures they
       specify content-security-policy and not inline scripts for those
       pages that are expected to do so."""
    tabs = self._GetPlatformSpecialURLTabs()
    for url, properties in tabs.iteritems():
      logging.debug('Testing URL %s.' % url)
      self.NavigateToURL(url)
      expected_title = 'title' in properties and properties['title'] or url
      actual_title = self.GetActiveTabTitle()
      logging.debug('  %s title was %s (%s)' %
                    (url, actual_title, expected_title == actual_title))
      self.assertEqual(expected_title, actual_title)
      include_list = []
      exclude_list = []
      if ('CSP' in properties and not properties['CSP']):
        exclude_list.extend(['X-WebKit-CSP'])
      else:
        include_list.extend(['X-WebKit-CSP'])
        exclude_list.extend(['<script>', 'onclick=', 'onload='])
      if ('includes' in properties):
        include_list.extend(properties['includes'])
      if ('excludes' in properties):
        exclude_list.extend(properties['exlcudes'])
      test_utils.StringContentCheck(self, self.GetTabContents(),
                                    include_list, exclude_list)

  def testAboutAppCacheTab(self):
    """Test App Cache tab to confirm about page populates caches."""
    self.NavigateToURL('about:appcache-internals')
    self._VerifyAppCacheInternals()
    self.assertEqual('AppCache Internals', self.GetActiveTabTitle())

  def testAboutDNSTab(self):
    """Test DNS tab to confirm DNS about page propogates records."""
    self.NavigateToURL('about:dns')
    self._VerifyAboutDNS()
    self.assertEqual('About DNS', self.GetActiveTabTitle())

  def testSpecialAcceratorTabs(self):
    """Test special tabs created by acclerators like IDC_SHOW_HISTORY,
       IDC_SHOW_DOWNLOADS."""
    for accel, title in self.special_accelerator_tabs.iteritems():
      self.RunCommand(accel)
      self.assertEqual(title, self.GetActiveTabTitle())


if __name__ == '__main__':
  pyauto_functional.Main()
