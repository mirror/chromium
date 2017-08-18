// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_UI_HISTORY_HISTORY_SERVICE_FACADE_DELEGATE_H_
#define IOS_CHROME_BROWSER_UI_HISTORY_HISTORY_SERVICE_FACADE_DELEGATE_H_

#include <vector>

#include "components/history/core/browser/browsing_history_service.h"
#include "ios/chrome/browser/ui/history/history_service_facade.h"

// Delegate for HistoryServiceFacade. Defines methods to manage history query
// results, deletion actions, and other dependecies.
@protocol HistoryServiceFacadeDelegate<NSObject>

@optional

// Notifies the delegate that the result of a history query has been retrieved.
// Entries in |result| are already sorted.
- (void)
historyServiceFacadeOnQueryComplete:(HistoryServiceFacade*)facade
                            results:(const std::vector<
                                        BrowsingHistoryService::HistoryEntry>&)
                                        results
                 query_results_info:
                     (const BrowsingHistoryService::QueryResultsInfo&)
                         query_results_info;

// Notifies the delegate that history entries have been deleted by a different
// client and that the UI should be updated.
- (void)historyServiceFacadeDidObserveHistoryDeletion:
    (HistoryServiceFacade*)facade;

// Indicates to the delegate whether to show notice about other forms of
// browsing history.
- (void)historyServiceFacade:(HistoryServiceFacade*)facade
    shouldShowNoticeAboutOtherFormsOfBrowsingHistory:(BOOL)shouldShowNotice;

@end

#endif  // IOS_CHROME_BROWSER_UI_HISTORY_HISTORY_SERVICE_FACADE_DELEGATE_H_
