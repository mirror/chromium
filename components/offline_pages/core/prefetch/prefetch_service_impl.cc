// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/offline_pages/core/prefetch/prefetch_service_impl.h"

#include <utility>

#include "base/memory/ptr_util.h"
#include "components/offline_pages/core/prefetch/offline_metrics_collector.h"
#include "components/offline_pages/core/prefetch/prefetch_dispatcher.h"
#include "components/offline_pages/core/prefetch/prefetch_gcm_handler.h"
#include "components/offline_pages/core/prefetch/suggested_articles_observer.h"

namespace offline_pages {

PrefetchServiceImpl::PrefetchServiceImpl(
    std::unique_ptr<PrefetchGCMHandler> gcm_handler,
    std::unique_ptr<PrefetchDispatcher> prefetch_dispatcher,
    std::unique_ptr<OfflineMetricsCollector> offline_metrics_collector,
    std::unique_ptr<SuggestedArticlesObserver> suggested_articles_observer)
    : offline_metrics_collector_(std::move(offline_metrics_collector)),
      dispatcher_(std::move(prefetch_dispatcher)),
      gcm_handler_(std::move(gcm_handler)),
      suggested_articles_observer_(std::move(suggested_articles_observer)) {}

PrefetchServiceImpl::~PrefetchServiceImpl() = default;

OfflineMetricsCollector* PrefetchServiceImpl::GetOfflineMetricsCollector() {
  return offline_metrics_collector_.get();
}

PrefetchDispatcher* PrefetchServiceImpl::GetDispatcher() {
  return dispatcher_.get();
}

PrefetchGCMHandler* PrefetchServiceImpl::GetPrefetchGCMHandler() {
  return gcm_handler_.get();
}

SuggestedArticlesObserver* PrefetchServiceImpl::GetSuggestedArticlesObserver() {
  return suggested_articles_observer_.get();
}

void PrefetchServiceImpl::Shutdown() {}

}  // namespace offline_pages
