// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/offline_pages/core/prefetch/prefetch_service_stub_taco.h"

#include "base/memory/ptr_util.h"

#include "components/offline_pages/core/prefetch/offline_metrics_collector.h"
#include "components/offline_pages/core/prefetch/offline_metrics_collector_stub.h"
#include "components/offline_pages/core/prefetch/prefetch_dispatcher.h"
#include "components/offline_pages/core/prefetch/prefetch_dispatcher_impl.h"
#include "components/offline_pages/core/prefetch/prefetch_gcm_handler.h"
#include "components/offline_pages/core/prefetch/prefetch_gcm_handler_stub.h"
#include "components/offline_pages/core/prefetch/prefetch_in_memory_store.h"
#include "components/offline_pages/core/prefetch/prefetch_service_impl.h"
#include "components/offline_pages/core/prefetch/suggested_articles_observer.h"

namespace offline_pages {

PrefetchServiceStubTaco::PrefetchServiceStubTaco() {
  metrics_collector_ = base::MakeUnique<OfflineMetricsCollectorStub>();
  dispatcher_ = base::MakeUnique<PrefetchDispatcherImpl>();
  gcm_handler_ = base::MakeUnique<PrefetchGCMHandlerStub>();
  store_ = base::MakeUnique<PrefetchInMemoryStore>();
}

PrefetchServiceStubTaco::~PrefetchServiceStubTaco() = default;

void PrefetchServiceStubTaco::SetOfflineMetricsCollector(
    std::unique_ptr<OfflineMetricsCollector> metrics_collector) {
  CHECK(!prefetch_service_);
  metrics_collector_ = std::move(metrics_collector);
}

void PrefetchServiceStubTaco::SetPrefetchDispatcher(
    std::unique_ptr<PrefetchDispatcher> dispatcher) {
  CHECK(!prefetch_service_);
  dispatcher_ = std::move(dispatcher);
}

void PrefetchServiceStubTaco::SetPrefetchGCMHandler(
    std::unique_ptr<PrefetchGCMHandler> gcm_handler) {
  CHECK(!prefetch_service_);
  gcm_handler_ = std::move(gcm_handler);
}

void PrefetchServiceStubTaco::SetPrefetchStore(
    std::unique_ptr<PrefetchStore> store) {
  CHECK(!prefetch_service_);
  store_ = std::move(store);
}

void PrefetchServiceStubTaco::SetSuggestedArticlesObserver(
    std::unique_ptr<SuggestedArticlesObserver> suggested_articles_observer) {
  CHECK(!prefetch_service_);
  suggested_articles_observer_ = std::move(suggested_articles_observer);
}

void PrefetchServiceStubTaco::CreatePrefetchService() {
  prefetch_service_ = base::MakeUnique<PrefetchServiceImpl>(
      std::move(metrics_collector_), std::move(dispatcher_),
      std::move(gcm_handler_), std::move(store_),
      std::move(suggested_articles_observer_));
}

}  // namespace offline_page
