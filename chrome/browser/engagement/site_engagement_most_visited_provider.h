// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_ENGAGEMENT_SITE_ENGAGEMENT_MOST_VISITED_PROVIDER_H_
#define CHROME_BROWSER_ENGAGEMENT_SITE_ENGAGEMENT_MOST_VISITED_PROVIDER_H_

#include "base/macros.h"
#include "base/memory/weak_ptr.h"
#include "base/task/cancelable_task_tracker.h"
#include "components/history/core/browser/history_service.h"
#include "components/history/core/browser/history_types.h"
#include "components/history/core/browser/top_sites_most_visited_provider.h"

class Profile;
class SiteEngagementService;

class SiteEngagementMostVisitedProvider
    : public history::TopSitesMostVisitedProvider {
 public:
  explicit SiteEngagementMostVisitedProvider(Profile* profile);
  ~SiteEngagementMostVisitedProvider() override;

  void QueryMostVisitedURLs(
      history::HistoryService* history_service,
      int result_count,
      int days_back,
      const history::HistoryService::QueryMostVisitedURLsCallback& callback,
      base::CancelableTaskTracker* tracker) override;

 private:
  void FillMostVisitedURLDetails(history::HistoryService* history_service,
                                 history::MostVisitedURLList* urls,
                                 const base::Closure& callback,
                                 base::CancelableTaskTracker* tracker);

  void OnURLQueried(history::MostVisitedURLList* urls,
                    size_t index,
                    const base::Closure& callback,
                    bool success,
                    const history::URLRow& row,
                    const history::VisitVector& visits);

  void OnRedirectsQueried(history::MostVisitedURLList* urls,
                          size_t index,
                          const base::Closure& callback,
                          const history::RedirectList* redirects);

  SiteEngagementService* engagement_service_;
  int outstanding_callbacks_;
  base::WeakPtrFactory<SiteEngagementMostVisitedProvider> weak_ptr_factory_;

  DISALLOW_COPY_AND_ASSIGN(SiteEngagementMostVisitedProvider);
};

#endif  // CHROME_BROWSER_ENGAGEMENT_SITE_ENGAGEMENT_MOST_VISITED_PROVIDER_H_
