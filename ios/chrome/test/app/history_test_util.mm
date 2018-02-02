// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/test/app/history_test_util.h"

#include "components/browsing_data/core/browsing_data_utils.h"
#include "ios/chrome/browser/browser_state/chrome_browser_state.h"
#import "ios/chrome/browser/browsing_data/browsing_data_removal_controller.h"
#include "ios/chrome/browser/browsing_data/ios_chrome_browsing_data_remover.h"
#import "ios/chrome/test/app/chrome_test_util.h"
#import "ios/testing/wait_util.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

namespace chrome_test_util {

bool ClearBrowsingHistory() {
  __block bool did_complete = false;
  ios::ChromeBrowserState* active_state = GetOriginalBrowserState();
  [[BrowsingDataRemovalController sharedInstance]
      removeBrowsingDataFromBrowserState:active_state
                                    mask:IOSChromeBrowsingDataRemover::
                                             REMOVE_HISTORY
                              timePeriod:browsing_data::TimePeriod::ALL_TIME
                       completionHandler:^{
                         did_complete = true;
                       }];

  // TODO(crbug.com/631795): This is a workaround that will be removed soon.
  // This code waits for success or timeout, then returns. This needs to be
  // fixed so that failure is correctly marked here, or the caller handles
  // waiting for the operation to complete. Wait for history to be cleared.
  return
      testing::WaitUntilConditionOrTimeout(testing::kWaitForUIElementTimeout, ^{
        return did_complete;
      });
}

}  // namespace chrome_test_util
