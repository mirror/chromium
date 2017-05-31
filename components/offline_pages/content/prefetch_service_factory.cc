// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/offline_pages/content/prefetch_service_factory.h"

#include "base/memory/ptr_util.h"
#include "base/memory/singleton.h"
#include "components/keyed_service/content/browser_context_dependency_manager.h"
#include "components/offline_pages/core/prefetch/offline_metrics_collector.h"
#include "components/offline_pages/core/prefetch/prefetch_service_impl.h"
#include "content/public/browser/browser_context.h"

namespace offline_pages {

PrefetchServiceFactory::PrefetchServiceFactory()
    : BrowserContextKeyedServiceFactory(
          "OfflinePagePrefetchService",
          BrowserContextDependencyManager::GetInstance()) {}

// static
PrefetchServiceFactory* PrefetchServiceFactory::GetInstance() {
  return base::Singleton<PrefetchServiceFactory>::get();
}

// static
PrefetchService* PrefetchServiceFactory::GetForBrowserContext(
    content::BrowserContext* context) {
  return static_cast<PrefetchService*>(
      GetInstance()->GetServiceForBrowserContext(context, true));
}

KeyedService* PrefetchServiceFactory::BuildServiceInstanceFor(
    content::BrowserContext* context) const {
  // TODO(dimich): Actually create one when another CL lands that moves factory.
  std::unique_ptr<OfflineMetricsCollector> offline_metrics_collector;
  return new PrefetchServiceImpl(std::move(offline_metrics_collector));
}

}  // namespace offline_pages
