// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/ntp_snippets/contextual_suggestions_service.h"

#include <iterator>
#include <memory>
#include <set>
#include <utility>

#include "base/bind.h"
#include "base/memory/ptr_util.h"
#include "base/time/default_clock.h"
#include "base/time/time.h"
#include "base/values.h"
#include "components/image_fetcher/core/image_fetcher_impl.h"
#include "components/ntp_snippets/pref_names.h"
#include "components/ntp_snippets/remote/cached_image_fetcher.h"
#include "components/ntp_snippets/remote/remote_suggestions_database.h"
#include "components/ntp_snippets/remote/remote_suggestions_provider_impl.h"
#include "ui/gfx/image/image.h"

namespace ntp_snippets {

ContextualSuggestionsService::ContextualSuggestionsService(
    std::unique_ptr<ContextualSuggestionsFetcher>
        contextual_suggestions_fetcher,
    std::unique_ptr<CachedImageFetcher> image_fetcher,
    std::unique_ptr<RemoteSuggestionsDatabase> contextual_suggestions_database)
    : contextual_suggestions_database_(
          std::move(contextual_suggestions_database)),
      contextual_suggestions_fetcher_(
          std::move(contextual_suggestions_fetcher)),
      image_fetcher_(std::move(image_fetcher)) {}

ContextualSuggestionsService::~ContextualSuggestionsService() = default;

void ContextualSuggestionsService::ClearCachedContextualSuggestions() {
  contextual_suggestions_database_->GarbageCollectImages(
      base::MakeUnique<std::set<std::string>>());
}

void ContextualSuggestionsService::FetchContextualSuggestions(
    const GURL& url,
    FetchContextualSuggestionsCallback callback) {
  contextual_suggestions_fetcher_->FetchContextualSuggestions(
      url, base::BindOnce(
               &ContextualSuggestionsService::OnFetchContextualSuggestions,
               base::Unretained(this), url, std::move(callback)));
}

void ContextualSuggestionsService::FetchContextualSuggestionImage(
    const ContentSuggestion::ID& suggestion_id,
    const ImageFetchedCallback& callback) {
  std::string id_within_category = suggestion_id.id_within_category();

  if (image_url_by_id_.count(id_within_category) > 0) {
    GURL image_url = image_url_by_id_.at(id_within_category);
    ImageFetchedCallback image_fetched_callback = base::Bind(
        &ContextualSuggestionsService ::OnFetchContextualSuggestionImage,
        base::Unretained(this), callback);
    image_fetcher_->FetchSuggestionImage(suggestion_id, image_url,
                                         image_fetched_callback);
  } else {
    DVLOG(1) << "FetchContextualSuggestionImage unknown image"
             << " id_within_category: " << id_within_category;
  }
}

void ContextualSuggestionsService::OnFetchContextualSuggestions(
    const GURL& url,
    FetchContextualSuggestionsCallback callback,
    Status status,
    ContextualSuggestionsFetcher::OptionalSuggestions fetched_suggestions) {
  std::vector<ContentSuggestion> suggestions;
  if (fetched_suggestions.has_value()) {
    for (const std::unique_ptr<RemoteSuggestion>& suggestion :
         fetched_suggestions.value()) {
      suggestions.emplace_back(suggestion->ToContentSuggestion(
          Category::FromKnownCategory(KnownCategories::CONTEXTUAL)));
      ContentSuggestion::ID id = suggestions.back().id();
      GURL image_url = suggestion->salient_image_url();
      image_url_by_id_[id.id_within_category()] = image_url;
    }
  }
  std::move(callback).Run(status, url, std::move(suggestions));
}

void ContextualSuggestionsService::OnFetchContextualSuggestionImage(
    const ImageFetchedCallback& callback,
    const gfx::Image& image) {
  callback.Run(image);
}

}  // namespace ntp_snippets
