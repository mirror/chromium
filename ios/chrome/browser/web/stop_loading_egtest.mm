// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import <EarlGrey/EarlGrey.h>

#include "base/memory/ptr_util.h"
#include "base/strings/stringprintf.h"
#include "base/time/time.h"
#include "ios/chrome/browser/ui/ui_util.h"
#include "ios/chrome/test/app/navigation_test_util.h"
#import "ios/chrome/test/earl_grey/chrome_earl_grey.h"
#import "ios/chrome/test/earl_grey/chrome_earl_grey_ui.h"
#import "ios/chrome/test/earl_grey/chrome_matchers.h"
#import "ios/chrome/test/earl_grey/chrome_test_case.h"
#include "ios/web/public/test/http_server/html_response_provider.h"
#import "ios/web/public/test/http_server/http_server.h"
#include "ios/web/public/test/http_server/http_server_util.h"
#include "url/gurl.h"

#include "ios/web/public/test/http_server/infinite_pending_response_provider.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

using chrome_test_util::ButtonWithAccessibilityLabelId;

// Test case for Stop Loading button.
@interface StopLoadingTestCase : ChromeTestCase
@end

@implementation StopLoadingTestCase

- (void)tearDown {
  // |testStopLoading| Disables synchronization, so make sure that it is enabled
  // if that test has failed and did not enable it back.
  [[GREYConfiguration sharedInstance]
          setValue:@YES
      forConfigKey:kGREYConfigKeySynchronizationEnabled];
  [super tearDown];
}

// Tests that tapping "Stop" button stops the loading.
- (void)testStopLoading {
  // Load a page which never finishes loading.
  GURL infinitePendingURL = web::test::HttpServer::MakeUrl("http://infinite");
  web::test::SetUpHttpServer(
      base::MakeUnique<InfinitePendingResponseProvider>(infinitePendingURL));

  // The page being loaded never completes, so call the LoadUrl helper that
  // does not wait for the page to complete loading.
  chrome_test_util::LoadUrl(infinitePendingURL);

  if (IsIPadIdiom()) {
    // Disable EG synchronization so the framework does not wait until the tab
    // loading spinner becomes idle (which will not happen until the stop button
    // is tapped).
    [[GREYConfiguration sharedInstance]
            setValue:@NO
        forConfigKey:kGREYConfigKeySynchronizationEnabled];
  }

  // Wait until the page is half loaded.
  [ChromeEarlGrey waitForWebViewContainingText:InfinitePendingResponseProvider::
                                                   GetPageText()];

  // On iPhone Stop/Reload button is a part of tools menu, so open it.
  if (!IsIPadIdiom()) {
    [ChromeEarlGreyUI openToolsMenu];
  }

  // Verify that stop button is visible and reload button is hidden.
  [[EarlGrey selectElementWithMatcher:chrome_test_util::StopButton()]
      assertWithMatcher:grey_sufficientlyVisible()];
  [[EarlGrey selectElementWithMatcher:chrome_test_util::ReloadButton()]
      assertWithMatcher:grey_notVisible()];

  // Stop the page loading.
  [[EarlGrey selectElementWithMatcher:chrome_test_util::StopButton()]
      performAction:grey_tap()];

  // Enable synchronization back. The spinner should become idle and test should
  // wait for it.
  [[GREYConfiguration sharedInstance]
          setValue:@YES
      forConfigKey:kGREYConfigKeySynchronizationEnabled];

  // Verify that stop button is hidden and reload button is visible.
  if (!IsIPadIdiom()) {
    [ChromeEarlGreyUI openToolsMenu];
  }
  [[EarlGrey selectElementWithMatcher:chrome_test_util::StopButton()]
      assertWithMatcher:grey_notVisible()];
  [[EarlGrey selectElementWithMatcher:chrome_test_util::ReloadButton()]
      assertWithMatcher:grey_sufficientlyVisible()];
}

@end
