// Copyright (c) 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_HISTORY_CORE_BROWSER_TOP_SITES_MOST_VISITED_PROVIDER_H_
#define COMPONENTS_HISTORY_CORE_BROWSER_TOP_SITES_MOST_VISITED_PROVIDER_H_

#include "base/macros.h"
#include "base/memory/weak_ptr.h"
#include "base/task/cancelable_task_tracker.h"
#include "components/history/core/browser/history_service.h"
#include "components/history/core/browser/history_types.h"

namespace history {

class MostEngagedProvider {
 public:
  // Fills |urls| with up to |result_count| most engaged URLs. Page titles and
  // redirect information for the URLs should not be included.
  virtual void QueryMostEngagedURLs(int result_count,
                                    history::MostVisitedURLList* urls) = 0;

  // Should return true if the url list filled by QueryMostVisitedURLs()
  // contains page metadata - title and redirects.
  virtual bool ProvidesMetadata() const = 0;
};

class TopSitesMostVisitedProvider {
 public:
  TopSitesMostVisitedProvider(MostEngagedProvider* most_engaged_provider);
  ~TopSitesMostVisitedProvider();

  MostEngagedProvider* most_engaged_provider() const {
    return most_engaged_provider_;
  }

  // Queries a provider for the most visited URLs, running |callback| when the
  // task is completed.
  void QueryMostVisitedURLs(
      history::HistoryService* history_service,
      int result_count,
      int days_back,
      const HistoryService::QueryMostVisitedURLsCallback& callback,
      base::CancelableTaskTracker* tracker);

 private:
  void FillMostVisitedURLDetails(history::HistoryService* history_service,
                                 history::MostVisitedURLList* urls,
                                 const base::Closure& callback,
                                 base::CancelableTaskTracker* tracker);

  void OnURLQueried(history::MostVisitedURLList* urls,
                    size_t index,
                    const base::Closure& callback,
                    bool success,
                    const URLRow& row,
                    const VisitVector& visits);

  void OnRedirectsQueried(history::MostVisitedURLList* urls,
                          size_t index,
                          const base::Closure& callback,
                          const RedirectList* redirects);

  // If non-null, this will be used to source the most visited URLs. Otherwise,
  // the history service will be used.
  MostEngagedProvider* most_engaged_provider_;

  int outstanding_callbacks_;

  base::WeakPtrFactory<TopSitesMostVisitedProvider> weak_ptr_factory_;

  DISALLOW_COPY_AND_ASSIGN(TopSitesMostVisitedProvider);
};

}  // namespace history

#endif  // COMPONENTS_HISTORY_CORE_BROWSER_TOP_SITES_MOST_VISITED_PROVIDER_H_
