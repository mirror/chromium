// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_HISTORY_CORE_BROWSER_TOP_SITES_PROVIDER_H_
#define COMPONENTS_HISTORY_CORE_BROWSER_TOP_SITES_PROVIDER_H_

#include "components/history/core/browser/history_service.h"

class CancelableTaskTracker;

namespace history {

class TopSitesProvider {
 public:
  virtual ~TopSitesProvider() {}

  // Queries for the list of URLs to include in TopSites, running |callback|
  // when complete.
  virtual void ProvideTopSites(
      history::HistoryService* history_service,
      int result_count,
      const HistoryService::QueryMostVisitedURLsCallback& callback,
      base::CancelableTaskTracker* tracker) = 0;
};

}  // namespace history

#endif  // COMPONENTS_HISTORY_CORE_BROWSER_TOP_SITES_PROVIDER_H_
