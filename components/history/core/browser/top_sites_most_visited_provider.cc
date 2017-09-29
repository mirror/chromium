// Copyright (c) 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/history/core/browser/top_sites_most_visited_provider.h"
#include "base/bind.h"
#include "base/callback.h"

namespace history {

TopSitesMostVisitedProvider::TopSitesMostVisitedProvider(
    MostEngagedProvider* most_engaged_provider)
    : most_engaged_provider_(most_engaged_provider),
      outstanding_callbacks_(0),
      weak_ptr_factory_(this) {}

TopSitesMostVisitedProvider::~TopSitesMostVisitedProvider() = default;

void TopSitesMostVisitedProvider::QueryMostVisitedURLs(
    history::HistoryService* history_service,
    int result_count,
    int days_back,
    const HistoryService::QueryMostVisitedURLsCallback& callback,
    base::CancelableTaskTracker* tracker) {
  if (most_engaged_provider_) {
    MostVisitedURLList* result = new MostVisitedURLList();
    most_engaged_provider_->QueryMostEngagedURLs(result_count, result);

    if (!most_engaged_provider_->ProvidesMetadata()) {
      FillMostVisitedURLDetails(history_service, result,
                                base::Bind(callback, base::Owned(result)),
                                tracker);
    }
  } else {
    history_service->QueryMostVisitedURLs(result_count, days_back, callback,
                                          tracker);
  }
}

void TopSitesMostVisitedProvider::FillMostVisitedURLDetails(
    history::HistoryService* history_service,
    history::MostVisitedURLList* urls,
    const base::Closure& callback,
    base::CancelableTaskTracker* tracker) {
  // We make 2 queries to the history service per each URL.
  outstanding_callbacks_ = urls->size() * 2;

  int i = 0;
  for (auto& mv : *urls) {
    history_service->QueryURL(
        mv.url, false,
        base::Bind(&TopSitesMostVisitedProvider::OnURLQueried,
                   weak_ptr_factory_.GetWeakPtr(), base::Unretained(urls), i,
                   callback),
        tracker);
    history_service->QueryRedirectsFrom(
        mv.url,
        base::Bind(&TopSitesMostVisitedProvider::OnRedirectsQueried,
                   weak_ptr_factory_.GetWeakPtr(), base::Unretained(urls), i,
                   callback),
        tracker);
    ++i;
  }
}

void TopSitesMostVisitedProvider::OnURLQueried(
    history::MostVisitedURLList* urls,
    size_t index,
    const base::Closure& callback,
    bool success,
    const URLRow& row,
    const VisitVector& visits) {
  DCHECK_GT(outstanding_callbacks_, 0);
  --outstanding_callbacks_;

  if (!success)
    return;
  (*urls)[index].title = row.title();

  if (outstanding_callbacks_ == 0)
    callback.Run();
}

void TopSitesMostVisitedProvider::OnRedirectsQueried(
    history::MostVisitedURLList* urls,
    size_t index,
    const base::Closure& callback,
    const RedirectList* redirects) {
  DCHECK_GT(outstanding_callbacks_, 0);
  --outstanding_callbacks_;

  (*urls)[index].InitRedirects(*redirects);

  if (outstanding_callbacks_ == 0)
    callback.Run();
}

}  // namespace history
