// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/history/core/browser/top_sites_most_visited_provider.h"

namespace history {

HistoryFrecencyMostVisitedProvider::HistoryFrecencyMostVisitedProvider() {}
HistoryFrecencyMostVisitedProvider::~HistoryFrecencyMostVisitedProvider() {}

void HistoryFrecencyMostVisitedProvider::QueryMostVisitedURLs(
    HistoryService* history_service,
    int result_count,
    int days_back,
    const HistoryService::QueryMostVisitedURLsCallback& callback,
    base::CancelableTaskTracker* tracker) {
  history_service->QueryMostVisitedURLs(result_count, days_back, callback,
                                        tracker);
}

}  // namespace history
