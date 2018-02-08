// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/app_list/search/arc/arc_app_data_search_provider.h"

#include "base/strings/utf_string_conversions.h"
#include "base/time/time.h"
#include "components/arc/arc_bridge_service.h"
#include "components/arc/arc_service_manager.h"
#include "components/arc/connection_holder.h"

namespace app_list {

ArcAppDataSearchProvider::ArcAppDataSearchProvider(
    int max_results,
    Profile* profile,
    AppListControllerDelegate* list_controller)
    : max_results_(max_results),
      profile_(profile),
      list_controller_(list_controller),
      weak_ptr_factory_(this) {
  DCHECK(profile_);
  DCHECK(list_controller_);
}

ArcAppDataSearchProvider::~ArcAppDataSearchProvider() = default;

void ArcAppDataSearchProvider::Start(const base::string16& query) {
  arc::mojom::AppInstance* app_instance =
      arc::ArcServiceManager::Get()
          ? ARC_GET_INSTANCE_FOR_METHOD(
                arc::ArcServiceManager::Get()->arc_bridge_service()->app(),
                GetIcingGlobalQueryResults)
          : nullptr;

  if (app_instance == nullptr || query.empty()) {
    ClearResults();
    return;
  }

  app_instance->GetIcingGlobalQueryResults(
      base::UTF16ToUTF8(query), max_results_,
      base::Bind(&ArcAppDataSearchProvider::OnResults,
                 weak_ptr_factory_.GetWeakPtr(), base::TimeTicks::Now()));
}

void ArcAppDataSearchProvider::OnResults(
    base::TimeTicks query_start_time,
    std::vector<arc::mojom::AppDataResultPtr> results) {
  LOG(ERROR) << "on results***";
}

}  // namespace app_list
