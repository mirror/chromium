// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import <EarlGrey/EarlGrey.h>

#include "base/ios/ios_util.h"
#import "ios/chrome/browser/metrics/tab_usage_recorder.h"
#import "ios/chrome/browser/metrics/tab_usage_recorder_test_util.h"
#include "ios/chrome/browser/ui/ui_util.h"
#import "ios/chrome/test/app/histogram_test_util.h"
#import "ios/chrome/test/app/tab_test_util.h"
#import "ios/chrome/test/earl_grey/chrome_earl_grey.h"
#import "ios/chrome/test/earl_grey/chrome_test_case.h"
#include "ios/web/public/test/http_server/http_server_util.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

namespace {

const char kTestUrl[] =
    "http://ios/testing/data/http_server_files/memory_usage.html";

}  // namespace

// External URL test case class for TabUsageRecorder.
@interface ExternalURLTabUsageRecorderTestCase : ChromeTestCase
@end

@implementation ExternalURLTabUsageRecorderTestCase

// Verify correct recording of metrics when the reloading of an evicted tab
// fails.
- (void)testEvictedTabReloadFailure {
#if TARGET_IPHONE_SIMULATOR
  // TODO(crbug.com/748404): Reenable it.
  if (!IsIPadIdiom() && !base::ios::IsRunningOnIOS10OrLater()) {
    EARL_GREY_TEST_DISABLED(@"Disabled for iPhone 9 simulators");
  }
#endif
  web::test::SetUpFileBasedHttpServer();
  chrome_test_util::HistogramTester histogramTester;
  FailureBlock failureBlock = ^(NSString* error) {
    GREYFail(error);
  };

  // This URL is purposely invalid so it triggers a navigation error.
  GURL invalidURL(kTestUrl);

  chrome_test_util::OpenNewTab();
  [ChromeEarlGrey loadURL:invalidURL];
  [ChromeEarlGrey waitForErrorPage];
  tab_usage_recorder_test_util::OpenNewIncognitoTabUsingUIAndEvictMainTabs();

  tab_usage_recorder_test_util::SwitchToNormalMode();
  [ChromeEarlGrey waitForErrorPage];

  histogramTester.ExpectUniqueSample(kEvictedTabReloadSuccessRate,
                                     TabUsageRecorder::LOAD_FAILURE, 1,
                                     failureBlock);
  histogramTester.ExpectUniqueSample(kDidUserWaitForEvictedTabReload,
                                     TabUsageRecorder::USER_WAITED, 1,
                                     failureBlock);
  histogramTester.ExpectTotalCount(kEvictedTabReloadTime, 0, failureBlock);
}

@end
