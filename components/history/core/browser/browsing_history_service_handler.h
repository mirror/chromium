// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_HISTORY_CORE_BROWSER_BROWSING_HISTORY_SERVICE_HANDLER_H_
#define COMPONENTS_HISTORY_CORE_BROWSER_BROWSING_HISTORY_SERVICE_HANDLER_H_

#include <vector>

#include "components/history/core/browser/browsing_history_service.h"

// Interface for handling calls from the BrowsingHistoryService.
class BrowsingHistoryServiceHandler {
 public:
  // Callback for QueryHistory().
  virtual void OnQueryComplete(
      const std::vector<BrowsingHistoryService::HistoryEntry>& results,
      const BrowsingHistoryService::QueryResultsInfo& query_results_info) = 0;

  // Callback for RemoveVisits().
  virtual void OnRemoveVisitsComplete() = 0;

  // Callback for RemoveVisits() that fails.
  virtual void OnRemoveVisitsFailed() = 0;

  // Callback for RemoveVisits() with the list of expire arguments. This gives
  // the handler a chance to perform embedder specific removal logic.
  virtual void OnRemoveVisits(
      const std::vector<history::ExpireHistoryArgs>& expire_list) = 0;

  // Called when HistoryService or WebHistoryService deletes one or more
  // items.
  virtual void HistoryDeleted() = 0;

  // Whether other forms of browsing history were found on the history
  // service.
  virtual void HasOtherFormsOfBrowsingHistory(bool has_other_forms,
                                              bool has_synced_results) = 0;

  // If history deletions are currently allowed.
  virtual bool AllowHistoryDeletions() = 0;

  // If the given url from web history is allowed to be shown to the user.
  virtual bool ShouldHideWebHistoryUrl(const GURL& url) = 0;

  // Retrieve the WebHistory service, which may or may not currently exist or be
  // accessible.
  virtual history::WebHistoryService* GetWebHistoryService() = 0;

  // Whether the Clear Browsing Data UI should show a notice about the existence
  // of other forms of browsing history stored in user's account. The response
  // is returned in a |callback|.
  virtual void ShouldShowNoticeAboutOtherFormsOfBrowsingHistory(
      const syncer::SyncService* sync_service,
      history::WebHistoryService* history_service,
      base::Callback<void(bool)> callback) = 0;

 protected:
  BrowsingHistoryServiceHandler() {}
  virtual ~BrowsingHistoryServiceHandler() {}

 private:
  DISALLOW_COPY_AND_ASSIGN(BrowsingHistoryServiceHandler);
};

#endif  // COMPONENTS_HISTORY_CORE_BROWSER_BROWSING_HISTORY_SERVICE_HANDLER_H_
