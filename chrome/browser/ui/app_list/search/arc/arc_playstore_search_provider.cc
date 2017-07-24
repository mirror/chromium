// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/app_list/search/arc/arc_playstore_search_provider.h"

#include <utility>

#include "base/metrics/histogram_macros.h"
#include "base/strings/utf_string_conversions.h"
#include "base/time/time.h"
#include "chrome/browser/ui/app_list/arc/arc_app_list_prefs.h"
#include "chrome/browser/ui/app_list/search/arc/arc_playstore_search_result.h"
#include "components/arc/arc_bridge_service.h"
#include "components/arc/arc_service_manager.h"

namespace app_list {

ArcPlayStoreSearchProvider::ArcPlayStoreSearchProvider(
    int max_results,
    Profile* profile,
    AppListControllerDelegate* list_controller)
    : max_results_(max_results),
      profile_(profile),
      list_controller_(list_controller),
      weak_ptr_factory_(this) {}

ArcPlayStoreSearchProvider::~ArcPlayStoreSearchProvider() = default;

void ArcPlayStoreSearchProvider::Start(bool is_voice_query,
                                       const base::string16& query) {
  // TODO(crbug.com/736027): should search for voice?
  arc::mojom::AppInstance* app_instance =
      arc::ArcServiceManager::Get()
          ? ARC_GET_INSTANCE_FOR_METHOD(
                arc::ArcServiceManager::Get()->arc_bridge_service()->app(),
                GetRecentAndSuggestedAppsFromPlayStore)
          : nullptr;

  if (app_instance == nullptr || query.empty())
    return;

  ClearResults();
  query_start_time_ = base::Time::Now();
  app_instance->GetRecentAndSuggestedAppsFromPlayStore(
      UTF16ToUTF8(query), max_results_,
      base::Bind(&ArcPlayStoreSearchProvider::OnResults,
                 weak_ptr_factory_.GetWeakPtr()));
}

void ArcPlayStoreSearchProvider::Stop() {}

void ArcPlayStoreSearchProvider::OnResults(
    bool canceled,
    std::vector<arc::mojom::AppDiscoveryResultPtr> results) {
  size_t instant_app_count = 0;
  for (auto& result : results) {
    if (result->is_instant_app)
      ++instant_app_count;
    Add(base::MakeUnique<ArcPlayStoreSearchResult>(std::move(result), profile_,
                                                   list_controller_));
  }
  // Record user metrics.
  if (!canceled) {
    UMA_HISTOGRAM_TIMES("Arc.PlayStoreSearch.QueryTime",
                        base::Time::Now() - query_start_time_);
    query_start_time_ = base::Time();
    // |max_results_| is initialized when this class is constructed and will
    // never change.
    const int kHistogramBuckets = max_results_ + 1;
    UMA_HISTOGRAM_CUSTOM_COUNTS("Arc.PlayStoreSearch.ReturnedAppsTotal",
                                results.size(), 0, max_results_,
                                kHistogramBuckets);
    UMA_HISTOGRAM_CUSTOM_COUNTS("Arc.PlayStoreSearch.ReturnedUninstalledApps",
                                results.size() - instant_app_count, 0,
                                max_results_, kHistogramBuckets);
    UMA_HISTOGRAM_CUSTOM_COUNTS("Arc.PlayStoreSearch.ReturnedInstantApps",
                                instant_app_count, 0, max_results_,
                                kHistogramBuckets);
  }
}

}  // namespace app_list
