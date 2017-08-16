// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_UI_HISTORY_HISTORY_SERVICE_FACADE_H_
#define IOS_CHROME_BROWSER_UI_HISTORY_HISTORY_SERVICE_FACADE_H_

#include <vector>

#include "base/macros.h"
#include "base/memory/weak_ptr.h"
#include "components/history/core/browser/browsing_history_service_handler.h"
#include "url/gurl.h"

namespace history {
class HistoryService;
}

namespace ios {
class ChromeBrowserState;
}

@protocol HistoryServiceFacadeDelegate;

// A simple implementation of BrowsingHistoryServiceHandler that delegates to
// objective-c object HistoryServiceFacadeDelegate for most actions.
class HistoryServiceFacade : public BrowsingHistoryServiceHandler {
 public:
  HistoryServiceFacade(ios::ChromeBrowserState* browser_state,
                       id<HistoryServiceFacadeDelegate> delegate);
  ~HistoryServiceFacade() override;

 private:
  // BrowsingHistoryServiceHandler implementation.
  void OnQueryComplete(
      const std::vector<BrowsingHistoryService::HistoryEntry>& results,
      const BrowsingHistoryService::QueryResultsInfo& query_results_info)
      override;
  void OnRemoveVisitsComplete() override;
  void OnRemoveVisitsFailed() override;
  void OnRemoveVisits(
      const std::vector<history::ExpireHistoryArgs>& expire_list) override;
  void HistoryDeleted() override;
  void HasOtherFormsOfBrowsingHistory(bool has_other_forms,
                                      bool has_synced_results) override;
  bool AllowHistoryDeletions() override;
  bool ShouldHideWebHistoryUrl(const GURL& url) override;
  history::WebHistoryService* GetWebHistoryService() override;
  void ShouldShowNoticeAboutOtherFormsOfBrowsingHistory(
      const syncer::SyncService* sync_service,
      history::WebHistoryService* history_service,
      base::Callback<void(bool)> callback) override;

  // The current browser state.
  ios::ChromeBrowserState* browser_state_;  // weak

  // Delegate for HistoryServiceFacade. Serves as client for HistoryService.
  __weak id<HistoryServiceFacadeDelegate> delegate_;

  DISALLOW_COPY_AND_ASSIGN(HistoryServiceFacade);
};

#endif  // IOS_CHROME_BROWSER_UI_HISTORY_HISTORY_SERVICE_FACADE_H_
