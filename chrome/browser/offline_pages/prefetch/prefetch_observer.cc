// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/offline_pages/prefetch/prefetch_observer.h"

#include "chrome/browser/offline_pages/prefetch/prefetched_pages_notifier.h"
#include "components/offline_pages/core/client_policy_controller.h"
#include "components/offline_pages/core/offline_page_feature.h"
#include "components/offline_pages/core/offline_page_item.h"

namespace offline_pages {

namespace {
int kPrefetchObserverUserDataKey;
}

// static
void PrefetchObserver::AttachToOfflinePageModel(OfflinePageModel* model) {
  if (!IsOfflinePagesPrefetchingUIEnabled())
    return;
  auto observer = base::MakeUnique<PrefetchObserver>();
  model->AddObserver(observer.get());
  model->SetUserData(&kPrefetchObserverUserDataKey, std::move(observer));
}

PrefetchObserver::PrefetchObserver() = default;

PrefetchObserver::~PrefetchObserver() = default;

void PrefetchObserver::OfflinePageModelLoaded(OfflinePageModel* model) {}

void PrefetchObserver::OfflinePageAdded(OfflinePageModel* model,
                                        const OfflinePageItem& added_page) {
  if (model->GetPolicyController()->IsSupportedByDownload(
          added_page.client_id.name_space)) {
    OnPageAddedForPrefetchedContentNotification();
  }
}

void PrefetchObserver::OfflinePageDeleted(
    const OfflinePageModel::DeletedPageInfo& page_info) {}

}  // namespace offline_pages
