// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_OFFLINE_PAGES_CORE_PREFETCH_PREFETCH_SERVICE_H_
#define COMPONENTS_OFFLINE_PAGES_CORE_PREFETCH_PREFETCH_SERVICE_H_

#include "components/keyed_service/core/keyed_service.h"

namespace base {
class FilePath;
}  // namespace base

namespace offline_pages {
class OfflineEventLogger;
class OfflineMetricsCollector;
class PrefetchDispatcher;
class PrefetchGCMHandler;
class SuggestedArticlesObserver;

// Main class and entry point for the Offline Pages Prefetching feature, that
// controls the lifetime of all major subcomponents of the prefetching system.
class PrefetchService : public KeyedService {
 public:
  ~PrefetchService() override = default;

  // The interface to handle the download events from DownloadService.
  class DownloadDelegate {
   public:
    virtual ~DownloadDelegate() = default;

    // Called when the download service is ready.
    virtual void OnDownloadServiceReady() = 0;
    // Called when the archive download succeeds.
    virtual void OnDownloadSucceeded(const std::string& download_id,
                                     const base::FilePath& path,
                                     uint64_t size) = 0;
    // Called when the archive download fails.
    virtual void OnDownloadFailed(const std::string& download_id) = 0;
  };

  // Subobjects that are created and owned by this service. Creation should be
  // lightweight, all heavy work must be done on-demand only.
  // The service manages lifetime, hookup and initialization of Prefetch
  // system that consists of multiple specialized objects, all vended by this
  // service.
  virtual OfflineEventLogger* GetLogger() = 0;
  virtual OfflineMetricsCollector* GetOfflineMetricsCollector() = 0;
  virtual PrefetchDispatcher* GetPrefetchDispatcher() = 0;
  virtual PrefetchGCMHandler* GetPrefetchGCMHandler() = 0;

  // May be |nullptr| in tests.  The PrefetchService does not depend on the
  // SuggestedArticlesObserver, it merely owns it for lifetime purposes.
  virtual SuggestedArticlesObserver* GetSuggestedArticlesObserver() = 0;

  // Returns the delegate to handle the download events.
  virtual DownloadDelegate* GetDownloadDelegate() = 0;
};

}  // namespace offline_pages

#endif  // COMPONENTS_OFFLINE_PAGES_CORE_PREFETCH_PREFETCH_SERVICE_H_
