// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/omnibox/favicon_cache.h"

#include <vector>

#include "base/containers/mru_cache.h"
#include "components/favicon/core/favicon_service.h"
#include "components/omnibox/browser/autocomplete_result.h"

namespace {

size_t GetFaviconCacheSize() {
  // Set cache size to twice the number of maximum results to avoid favicon
  // refetches as the user types. Favicon fetches are uncached and can hit disk.
  return 2 * AutocompleteResult::GetMaxMatches();
}

}  // namespace

struct FaviconCache::Request {
  base::CancelableTaskTracker::TaskId task_id;
  std::vector<FaviconFetchedCallback> waiting_callbacks;
};

FaviconCache::FaviconCache(favicon::FaviconService* favicon_service)
    : favicon_service_(favicon_service),
      mru_cache_(GetFaviconCacheSize()),
      weak_factory_(this) {}

FaviconCache::~FaviconCache() {}

gfx::Image FaviconCache::GetFaviconForPageUrl(
    const GURL& page_url,
    FaviconFetchedCallback on_favicon_fetched) {
  auto cache_iterator = mru_cache_.Get(page_url);
  if (cache_iterator != mru_cache_.end())
    return cache_iterator->second;

  // We don't have the favicon in the cache. We kick off the request and return
  // an empty gfx::Image.
  if (!favicon_service_)
    return gfx::Image();

  // We have an outstanding request for this page. Add one more waiting callback
  // and return an empty gfx::Image.
  auto pending_request_it = pending_requests_.find(page_url);
  if (pending_request_it != pending_requests_.end()) {
    pending_request_it->second.waiting_callbacks.push_back(
        std::move(on_favicon_fetched));
    return gfx::Image();
  }

  // TODO(tommycli): Investigate using the version of this method that specifies
  // the desired size.
  Request request;
  request.task_id = favicon_service_->GetFaviconImageForPageURL(
      page_url,
      base::BindRepeating(&FaviconCache::OnFaviconFetched,
                          weak_factory_.GetWeakPtr(), page_url),
      &task_tracker_);
  request.waiting_callbacks.push_back(std::move(on_favicon_fetched));
  pending_requests_[page_url] = std::move(request);

  return gfx::Image();
}

void FaviconCache::OnFaviconFetched(
    const GURL& page_url,
    const favicon_base::FaviconImageResult& result) {
  // TODO(tommycli): Also cache null image results with a reasonable expiry.
  if (result.image.IsEmpty())
    return;

  mru_cache_.Put(page_url, result.image);

  auto pending_request_it = pending_requests_.find(page_url);
  DCHECK(pending_request_it != pending_requests_.end());

  for (auto& callback : pending_request_it->second.waiting_callbacks) {
    std::move(callback).Run(result.image);
  }

  pending_requests_.erase(pending_request_it);
}
