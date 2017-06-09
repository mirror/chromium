// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/ntp_snippets/remote/remote_suggestions_fetcher.h"

#include "components/ntp_snippets/features.h"
#include "components/ntp_snippets/ntp_snippets_constants.h"
#include "components/strings/grit/components_strings.h"
#include "components/variations/variations_associated_data.h"
#include "ui/base/l10n/l10n_util.h"

namespace ntp_snippets {

namespace {

// Variation parameter for chrome-content-suggestions backend.
const char kContentSuggestionsBackend[] = "content_suggestions_backend";

}  // namespace

GURL GetFetchEndpoint(version_info::Channel channel) {
  std::string endpoint = variations::GetVariationParamValueByFeature(
      ntp_snippets::kArticleSuggestionsFeature, kContentSuggestionsBackend);
  if (!endpoint.empty()) {
    return GURL{endpoint};
  }

  switch (channel) {
    case version_info::Channel::STABLE:
    case version_info::Channel::BETA:
      return GURL{kContentSuggestionsServer};

    case version_info::Channel::DEV:
    case version_info::Channel::CANARY:
    case version_info::Channel::UNKNOWN:
      return GURL{kContentSuggestionsStagingServer};
  }
  NOTREACHED();
  return GURL{kContentSuggestionsStagingServer};
}

CategoryInfo BuildArticleCategoryInfo(
    const base::Optional<base::string16>& title) {
  return CategoryInfo(
      title.has_value() ? title.value()
                        : l10n_util::GetStringUTF16(
                              IDS_NTP_ARTICLE_SUGGESTIONS_SECTION_HEADER),
      ContentSuggestionsCardLayout::FULL_CARD,
      ContentSuggestionsAdditionalAction::FETCH,
      /*show_if_empty=*/true,
      l10n_util::GetStringUTF16(IDS_NTP_ARTICLE_SUGGESTIONS_SECTION_EMPTY));
}

CategoryInfo BuildRemoteCategoryInfo(const base::string16& title,
                                     bool allow_fetching_more_results) {
  ContentSuggestionsAdditionalAction action =
      ContentSuggestionsAdditionalAction::NONE;
  if (allow_fetching_more_results) {
    action = ContentSuggestionsAdditionalAction::FETCH;
  }
  return CategoryInfo(
      title, ContentSuggestionsCardLayout::FULL_CARD, action,
      /*show_if_empty=*/false,
      // TODO(tschumann): The message for no-articles is likely wrong
      // and needs to be added to the stubby protocol if we want to
      // support it.
      l10n_util::GetStringUTF16(IDS_NTP_ARTICLE_SUGGESTIONS_SECTION_EMPTY));
}

RemoteSuggestionsFetcher::FetchedCategory::FetchedCategory(Category category,
                                                           CategoryInfo&& info)
    : category(category), info(info) {}

RemoteSuggestionsFetcher::FetchedCategory::FetchedCategory(FetchedCategory&&) =
    default;

RemoteSuggestionsFetcher::FetchedCategory::~FetchedCategory() = default;

RemoteSuggestionsFetcher::FetchedCategory&
RemoteSuggestionsFetcher::FetchedCategory::operator=(FetchedCategory&&) =
    default;

RemoteSuggestionsFetcher::~RemoteSuggestionsFetcher() = default;

}  // namespace ntp_snippets
