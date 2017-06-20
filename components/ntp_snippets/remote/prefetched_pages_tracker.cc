// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/ntp_snippets/remote/prefetched_pages_tracker.h"

#include "base/bind.h"
#include "components/offline_pages/core/client_namespace_constants.h"

using offline_pages::OfflinePageItem;
using offline_pages::OfflinePageModel;
using offline_pages::OfflinePageModelQuery;
using offline_pages::OfflinePageModelQueryBuilder;

namespace ntp_snippets {

namespace {

std::unique_ptr<OfflinePageModelQuery> BuildPrefetchedPagesQuery(
    OfflinePageModel* model) {
  OfflinePageModelQueryBuilder builder;
  builder.RequireNamespace(offline_pages::kSuggestedArticlesNamespace);
  return builder.Build(model->GetPolicyController());
}

bool IsOfflineItemPrefetchedPage(const OfflinePageItem& offline_page_item) {
  return offline_page_item.client_id.name_space ==
         offline_pages::kSuggestedArticlesNamespace;
}

const GURL& GetOfflinePageUrl(const OfflinePageItem& offline_page_item) {
  return offline_page_item.original_url != GURL()
             ? offline_page_item.original_url
             : offline_page_item.url;
}

}  // namespace

PrefetchedPagesTracker::PrefetchedPagesTracker(
    OfflinePageModel* offline_page_model)
    : weak_ptr_factory_(this) {
  DCHECK(offline_page_model);
  // If Offline Page model is not loaded yet, it will process our query
  // once it has finished loading.
  offline_page_model->GetPagesMatchingQuery(
      BuildPrefetchedPagesQuery(offline_page_model),
      base::Bind(&PrefetchedPagesTracker::Initialize,
                 weak_ptr_factory_.GetWeakPtr(), offline_page_model));
}

PrefetchedPagesTracker::~PrefetchedPagesTracker() = default;

bool PrefetchedPagesTracker::OfflinePageExists(const GURL url) const {
  return prefetched_urls_.count(url);
}

void PrefetchedPagesTracker::OfflinePageModelLoaded(OfflinePageModel* model) {
  // Ignored. Offline Page model delayes our requests until it is loaded.
}

void PrefetchedPagesTracker::OfflinePageAdded(
    OfflinePageModel* model,
    const OfflinePageItem& added_page) {
  if (IsOfflineItemPrefetchedPage(added_page)) {
    AddOfflinePage(added_page);
  }
}

void PrefetchedPagesTracker::OfflinePageDeleted(
    int64_t offline_id,
    const offline_pages::ClientId& client_id) {
  std::map<int64_t, GURL>::iterator it =
      offline_id_to_url_mapping_.find(offline_id);
  if (it != offline_id_to_url_mapping_.end()) {
    DCHECK(prefetched_urls_.count(it->second));
    prefetched_urls_.erase(it->second);
    offline_id_to_url_mapping_.erase(it);
  }
}

void PrefetchedPagesTracker::Initialize(
    OfflinePageModel* offline_page_model,
    const std::vector<OfflinePageItem>& all_prefetched_offline_pages) {
  for (const OfflinePageItem& item : all_prefetched_offline_pages) {
    DCHECK(IsOfflineItemPrefetchedPage(item));
    AddOfflinePage(item);
  }

  offline_page_model->AddObserver(this);
}

void PrefetchedPagesTracker::AddOfflinePage(
    const OfflinePageItem& offline_page_item) {
  DCHECK(!prefetched_urls_.count(GetOfflinePageUrl(offline_page_item)));
  prefetched_urls_.insert(GetOfflinePageUrl(offline_page_item));
  offline_id_to_url_mapping_[offline_page_item.offline_id] =
      GetOfflinePageUrl(offline_page_item);
}

}  // namespace ntp_snippets
