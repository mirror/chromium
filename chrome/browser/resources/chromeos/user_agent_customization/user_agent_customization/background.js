// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/** @fileoverview Customization of User-Agent with usage of webRequest API. */

/** Pattern for platform string in User-Agent. */
const REGEX_PLATFORM = new RegExp('\\(([^\\)]+)\\)', 'i');
/** Windows platform string. */
const PLATFORM_WIN = '(Windows NT 6.1)';
/** Url to be opened after extension installation. */
const START_URL = 'http://www.office.com';

/** Opens start page when extension was installed. */
chrome.runtime.onInstalled.addListener(function(details) {
  if (details.reason == 'install') {
    var args = {url: START_URL, active: true};
    chrome.tabs.create(args);
  }
});

/** Overrides User-Agent header for the specified domains. */
chrome.webRequest.onBeforeSendHeaders.addListener(function(details) {
  for (var i = 0; i < details.requestHeaders.length; ++i) {
    if (details.requestHeaders[i].name == 'User-Agent') {
      var userAgent = details.requestHeaders[i].value;
      userAgent = userAgent.replace(REGEX_PLATFORM, PLATFORM_WIN);
      details.requestHeaders[i].value = userAgent;
      break;
    }
  }
  return {requestHeaders: details.requestHeaders};
}, {urls: ['*://*.office.com/*']}, ['blocking', 'requestHeaders']);
