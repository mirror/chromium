// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/engagement/site_engagement_top_sites_provider.h"

#include <vector>

#include "base/bind.h"
#include "base/callback.h"
#include "chrome/browser/engagement/site_engagement_service.h"
#include "chrome/browser/profiles/profile.h"

SiteEngagementTopSitesProvider::SiteEngagementTopSitesProvider(Profile* profile)
    : engagement_service_(SiteEngagementService::Get(profile)),
      outstanding_callbacks_(0),
      weak_ptr_factory_(this) {}

SiteEngagementTopSitesProvider::~SiteEngagementTopSitesProvider() {}

void SiteEngagementTopSitesProvider::ProvideTopSites(
    history::HistoryService* history_service,
    int result_count,
    const history::HistoryService::QueryMostVisitedURLsCallback& callback,
    base::CancelableTaskTracker* tracker) {
  // Ownership is passed to |callback|.
  history::MostVisitedURLList* urls = new history::MostVisitedURLList();

  std::vector<mojom::SiteEngagementDetails> details =
      engagement_service_->GetAllDetails();
  std::sort(details.begin(), details.end(),
            [](const mojom::SiteEngagementDetails& lhs,
               const mojom::SiteEngagementDetails& rhs) {
              return lhs.total_score > rhs.total_score;
            });

  for (const auto& detail : details) {
    if (static_cast<int>(urls->size()) >= result_count)
      break;
    history::MostVisitedURL mv;
    mv.url = detail.origin;
    urls->push_back(mv);
  }

  FillMostVisitedURLDetails(history_service, urls,
                            base::Bind(callback, base::Owned(urls)), tracker);
}

void SiteEngagementTopSitesProvider::FillMostVisitedURLDetails(
    history::HistoryService* history_service,
    history::MostVisitedURLList* urls,
    const base::Closure& callback,
    base::CancelableTaskTracker* tracker) {
  // We make 2 queries to the history service per each URL.
  outstanding_callbacks_ += urls->size() * 2;

  int i = 0;
  for (const auto& mv : *urls) {
    history_service->QueryURL(
        mv.url, false,
        base::Bind(&SiteEngagementTopSitesProvider::OnURLQueried,
                   weak_ptr_factory_.GetWeakPtr(),
                   base::Unretained(&(*urls)[i]), callback),
        tracker);
    history_service->QueryRedirectsFrom(
        mv.url,
        base::Bind(&SiteEngagementTopSitesProvider::OnRedirectsQueried,
                   weak_ptr_factory_.GetWeakPtr(),
                   base::Unretained(&(*urls)[i]), callback),
        tracker);
    ++i;
  }
}

void SiteEngagementTopSitesProvider::OnURLQueried(
    history::MostVisitedURL* mv,
    const base::Closure& callback,
    bool success,
    const history::URLRow& row,
    const history::VisitVector& visits) {
  DCHECK_GT(outstanding_callbacks_, 0);
  --outstanding_callbacks_;

  if (!success)
    return;
  mv->title = row.title();

  if (outstanding_callbacks_ == 0)
    callback.Run();
}

void SiteEngagementTopSitesProvider::OnRedirectsQueried(
    history::MostVisitedURL* mv,
    const base::Closure& callback,
    const history::RedirectList* redirects) {
  DCHECK_GT(outstanding_callbacks_, 0);
  --outstanding_callbacks_;

  mv->InitRedirects(*redirects);

  if (outstanding_callbacks_ == 0)
    callback.Run();
}
