// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_HISTORY_CORE_BROWSER_TOP_SITES_MOST_VISITED_PROVIDER_H_
#define COMPONENTS_HISTORY_CORE_BROWSER_TOP_SITES_MOST_VISITED_PROVIDER_H_

#include "base/macros.h"
#include "components/history/core/browser/history_service.h"

class CancelableTaskTracker;

namespace history {

class TopSitesMostVisitedProvider {
 public:
  virtual ~TopSitesMostVisitedProvider(){};

  // Queries for the most visited URLs, running |callback| when complete.
  virtual void QueryMostVisitedURLs(
      history::HistoryService* history_service,
      int result_count,
      int days_back,
      const HistoryService::QueryMostVisitedURLsCallback& callback,
      base::CancelableTaskTracker* tracker) = 0;
};

class HistoryFrecencyMostVisitedProvider : public TopSitesMostVisitedProvider {
 public:
  HistoryFrecencyMostVisitedProvider();
  ~HistoryFrecencyMostVisitedProvider() override;

  void QueryMostVisitedURLs(
      HistoryService* history_service,
      int result_count,
      int days_back,
      const HistoryService::QueryMostVisitedURLsCallback& callback,
      base::CancelableTaskTracker* tracker) override;

 private:
  DISALLOW_COPY_AND_ASSIGN(HistoryFrecencyMostVisitedProvider);
};

}  // namespace history

#endif  // COMPONENTS_HISTORY_CORE_BROWSER_TOP_SITES_MOST_VISITED_PROVIDER_H_
