// Copyright (c) 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_BROWSING_DATA_HISTORY_COUNTER_H_
#define CHROME_BROWSER_BROWSING_DATA_HISTORY_COUNTER_H_

#include "base/task/cancelable_task_tracker.h"
#include "base/timer/timer.h"
#include "chrome/browser/browsing_data/browsing_data_counter.h"
#include "components/history/core/browser/history_service.h"
#include "components/history/core/browser/web_history_service.h"

class HistoryCounter: public BrowsingDataCounter {
 public:
  class HistoryResult : public FinishedResult {
   public:
    HistoryResult(const HistoryCounter* source,
                  ResultInt value,
                  bool has_synced_visits);
    ~HistoryResult() override;

    bool has_synced_visits() const { return has_synced_visits_; }

   private:
    bool has_synced_visits_;
  };

  HistoryCounter();
  ~HistoryCounter() override;

  const std::string& GetPrefName() const override;

  // Whether there are counting tasks in progress. Only used for testing.
  bool HasTrackedTasks();

  // Make the history counter use a custom WebHistoryService instance. Only
  // used for testing.
  void SetWebHistoryServiceForTesting(history::WebHistoryService* service);

 private:
  const std::string pref_name_;

  BrowsingDataCounter::ResultInt local_result_;
  bool has_synced_visits_;

  bool local_counting_finished_;
  bool web_counting_finished_;

  history::WebHistoryService* testing_web_history_service_;

  base::CancelableTaskTracker cancelable_task_tracker_;
  scoped_ptr<history::WebHistoryService::Request> web_history_request_;
  base::OneShotTimer web_history_timeout_;

  base::ThreadChecker thread_checker_;

  void Count() override;

  void OnGetLocalHistoryCount(history::HistoryCountResult result);
  void OnGetWebHistoryCount(history::WebHistoryService::Request* request,
                            const base::DictionaryValue* result);
  void OnWebHistoryTimeout();
  void MergeResults();
};

#endif  // CHROME_BROWSER_BROWSING_DATA_HISTORY_COUNTER_H_
