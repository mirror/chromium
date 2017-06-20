// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/offline_pages/core/prefetch/prefetch_service_test_taco.h"

#include <utility>

#include "base/memory/ptr_util.h"
#include "components/offline_pages/core/prefetch/offline_metrics_collector.h"
#include "components/offline_pages/core/prefetch/prefetch_dispatcher.h"
#include "components/offline_pages/core/prefetch/prefetch_gcm_handler.h"
#include "components/offline_pages/core/prefetch/prefetch_network_request_factory.h"
#include "components/offline_pages/core/prefetch/prefetch_service_impl.h"
#include "components/offline_pages/core/prefetch/suggested_articles_observer.h"
#include "components/offline_pages/core/prefetch/test_offline_metrics_collector.h"
#include "components/offline_pages/core/prefetch/test_prefetch_dispatcher.h"
#include "components/offline_pages/core/prefetch/test_prefetch_gcm_handler.h"

namespace offline_pages {

PrefetchServiceTestTaco::PrefetchServiceTestTaco() {
  metrics_collector_ = base::MakeUnique<TestOfflineMetricsCollector>();
  dispatcher_ = base::MakeUnique<TestPrefetchDispatcher>();
  gcm_handler_ = base::MakeUnique<TestPrefetchGCMHandler>();
  suggested_articles_observer_ =
      base::MakeUnique<SuggestedArticlesObserver>(dispatcher_.get());

  // This sets up the vector of testing articles, no need to do anything with
  // the result.
  suggested_articles_observer_->GetTestingArticles();
}

PrefetchServiceTestTaco::~PrefetchServiceTestTaco() = default;

void PrefetchServiceTestTaco::SetOfflineMetricsCollector(
    std::unique_ptr<OfflineMetricsCollector> metrics_collector) {
  CHECK(!prefetch_service_);
  metrics_collector_ = std::move(metrics_collector);
}

void PrefetchServiceTestTaco::SetPrefetchDispatcher(
    std::unique_ptr<PrefetchDispatcher> dispatcher) {
  CHECK(!prefetch_service_);
  dispatcher_ = std::move(dispatcher);
}

void PrefetchServiceTestTaco::SetPrefetchGCMHandler(
    std::unique_ptr<PrefetchGCMHandler> gcm_handler) {
  CHECK(!prefetch_service_);
  gcm_handler_ = std::move(gcm_handler);
}

void PrefetchServiceTestTaco::SetPrefetchNetworkRequestFactory(
    std::unique_ptr<PrefetchNetworkRequestFactory> network_request_factory) {
  CHECK(!prefetch_service_);
  network_request_factory_ = std::move(network_request_factory);
}

void PrefetchServiceTestTaco::SetSuggestedArticlesObserver(
    std::unique_ptr<SuggestedArticlesObserver> suggested_articles_observer) {
  CHECK(!prefetch_service_);
  suggested_articles_observer_ = std::move(suggested_articles_observer);
}

void PrefetchServiceTestTaco::CreatePrefetchService() {
  prefetch_service_ = base::MakeUnique<PrefetchServiceImpl>(
      std::move(metrics_collector_), std::move(dispatcher_),
      std::move(gcm_handler_), std::move(network_request_factory_),
      std::move(suggested_articles_observer_));
}

}  // namespace offline_pages
