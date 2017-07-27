// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_NTP_SNIPPETS_CONTEXTUAL_SUGGESTIONS_SOURCE_H_
#define COMPONENTS_NTP_SNIPPETS_CONTEXTUAL_SUGGESTIONS_SOURCE_H_

#include <memory>
#include <string>
#include <vector>

#include "base/callback.h"
#include "base/optional.h"
#include "base/scoped_observer.h"
#include "components/image_fetcher/core/image_fetcher.h"
#include "components/ntp_snippets/callbacks.h"
#include "components/ntp_snippets/remote/contextual_suggestions_fetcher.h"

namespace ntp_snippets {

class CachedImageFetcher;
class RemoteSuggestionsDatabase;

// Retrieves contextual suggestions for a given URL.
class ContextualSuggestionsSource {
 public:
  ContextualSuggestionsSource(std::unique_ptr<ContextualSuggestionsFetcher>
                                  contextual_suggestions_fetcher,
                              std::unique_ptr<CachedImageFetcher> image_fetcher,
                              std::unique_ptr<RemoteSuggestionsDatabase>
                                  contextual_suggestions_database);
  ~ContextualSuggestionsSource();

  using FetchContextualSuggestionsCallback =
      base::OnceCallback<void(Status status_code,
                              const GURL& url,
                              std::vector<ContentSuggestion> suggestions)>;

  // Fetches contextual suggestions for the given URL.
  void FetchContextualSuggestions(const GURL& url,
                                  FetchContextualSuggestionsCallback callback);

  // Asynchronously fetches an image for a given contextual suggestion ID.
  void FetchContextualSuggestionImage(
      const ContentSuggestion::ID& suggestion_id,
      const ImageFetchedCallback& callback);

 private:
  void OnFetchContextualSuggestions(
      const GURL& url,
      FetchContextualSuggestionsCallback callback,
      Status status,
      ContextualSuggestionsFetcher::OptionalSuggestions fetched_suggestions);

  void OnFetchContextualSuggestionImage(const ImageFetchedCallback& callback,
                                        const gfx::Image& image);

  // Cache for images of contextual suggestions.
  std::unique_ptr<RemoteSuggestionsDatabase> contextual_suggestions_database_;

  // Performs actual Json request to fetch contextual suggestions.
  std::unique_ptr<ContextualSuggestionsFetcher> contextual_suggestions_fetcher_;

  // Fetches image by looking into cache.
  std::unique_ptr<CachedImageFetcher> image_fetcher_;

  // Look up by ContentSuggestion::ID to get image URL.
  std::map<std::string, GURL> image_url_by_id_;

  DISALLOW_COPY_AND_ASSIGN(ContextualSuggestionsSource);
};

}  // namespace ntp_snippets

#endif  // COMPONENTS_NTP_SNIPPETS_CONTEXTUAL_SUGGESTIONS_SOURCE_H_
