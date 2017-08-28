// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.


/**
 * @fileoverview Tests the local NTP.
 */


/**
 * Sets up the page for each individual test.
 */
function setUp() {
  setUpPage('local-ntp-template');
}


// ******************************* SIMPLE TESTS *******************************
// These are run by runSimpleTests above.
// Functions from test_utils.js are automatically imported.


/**
 * Tests that Google NTPs show a fakebox and logo.
 */
function testShowsFakeboxAndLogoIfGoogle() {
  initLocalNTP(/*isGooglePage=*/true);
  assert(elementIsVisible($('fakebox')));
  assert(elementIsVisible($('logo')));
}


/**
 * Tests that non-Google NTPs do not show a fakebox.
 */
function testDoesNotShowFakeboxIfNotGoogle() {
  initLocalNTP(/*isGooglePage=*/false);
  assert(!elementIsVisible($('fakebox')));
  assert(!elementIsVisible($('logo')));
}


/**
 * Tests that the embeddedSearch.newTabPage.mostVisited API is hooked up, and
 * provides the correct data for the tiles (i.e. only IDs, no URLs).
 */
function testMostVisitedContents() {
  // Check that the API is available and properly hooked up, so that it returns
  // some data (see history::PrepopulatedPageList for the default contents).
  assert(window.chrome.embeddedSearch.newTabPage.mostVisited.length > 0);

  // Check that the items have the required fields: We expect a "restricted ID"
  // (rid), but there mustn't be url, title, etc. Those are only available
  // through getMostVisitedItemData(rid).
  for (var mvItem of window.chrome.embeddedSearch.newTabPage.mostVisited) {
    assert(isFinite(mvItem.rid));
    assert(!mvItem.url);
    assert(!mvItem.title);
    assert(!mvItem.domain);
  }

  // Try to get an item's details via getMostVisitedItemData. This should fail,
  // because that API is only available to the MV iframe.
  assert(!window.chrome.embeddedSearch.newTabPage.getMostVisitedItemData(
      window.chrome.embeddedSearch.newTabPage.mostVisited[0].rid));
}



// ****************************** ADVANCED TESTS ******************************
// Advanced tests are controlled from the native side. The helpers here are
// called from native code to set up the page and to check results.

function handlePostMessage(event) {
  if (event.data.cmd == 'loaded') {
    domAutomationController.send('loaded');
  }
}


function setupAdvancedTest(opt_waitForIframeLoaded) {
  if (opt_waitForIframeLoaded) {
    window.addEventListener('message', handlePostMessage);
  }

  setUpPage('local-ntp-template');
  initLocalNTP(/*isGooglePage=*/true);

  assert(elementIsVisible($('fakebox')));

  return true;
}


function getFakeboxPositionX() {
  assert(elementIsVisible($('fakebox')));
  var rect = $('fakebox').getBoundingClientRect();
  return rect.left;
}


function getFakeboxPositionY() {
  assert(elementIsVisible($('fakebox')));
  var rect = $('fakebox').getBoundingClientRect();
  return rect.top;
}


function fakeboxIsVisible() {
  return elementIsVisible($('fakebox'));
}


function fakeboxIsFocused() {
  return fakeboxIsVisible() &&
      document.body.classList.contains('fakebox-focused');
}
