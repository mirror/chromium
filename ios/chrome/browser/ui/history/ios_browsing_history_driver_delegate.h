// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_UI_HISTORY_IOS_BROWSING_HISTORY_DRIVER_DELEGATE_H_
#define IOS_CHROME_BROWSER_UI_HISTORY_IOS_BROWSING_HISTORY_DRIVER_DELEGATE_H_

#include <vector>

#include "components/history/core/browser/browsing_history_service.h"
#include "ios/chrome/browser/ui/history/ios_browsing_history_driver.h"

// Delegate for IOSBrowsingHistoryDriver. Defines methods to manage history
// query results and deletion actions.
@protocol IOSBrowsingHistoryDriverDelegate<NSObject>

// Notifies the delegate that the result of a history query has been retrieved.
// Entries in |result| are already sorted.
- (void)
IOSBrowsingHistoryDriverOnQueryComplete:(IOSBrowsingHistoryDriver*)facade
                                results:
                                    (const std::vector<
                                        BrowsingHistoryService::HistoryEntry>&)
                                        results
                     query_results_info:
                         (const BrowsingHistoryService::QueryResultsInfo&)
                             query_results_info;

// Notifies the delegate that history entries have been deleted by a different
// client and that the UI should be updated.
- (void)IOSBrowsingHistoryDriverDidObserveHistoryDeletion:
    (IOSBrowsingHistoryDriver*)facade;

// Indicates to the delegate whether to show notice about other forms of
// browsing history.
- (void)IOSBrowsingHistoryDriver:(IOSBrowsingHistoryDriver*)facade
    shouldShowNoticeAboutOtherFormsOfBrowsingHistory:(BOOL)shouldShowNotice;

@end

#endif  // IOS_CHROME_BROWSER_UI_HISTORY_IOS_BROWSING_HISTORY_DRIVER_DELEGATE_H_
