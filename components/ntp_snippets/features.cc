// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/ntp_snippets/features.h"

#include "base/feature_list.h"
#include "base/memory/ptr_util.h"
#include "base/time/clock.h"
#include "components/ntp_snippets/category_rankers/click_based_category_ranker.h"
#include "components/ntp_snippets/category_rankers/constant_category_ranker.h"

namespace ntp_snippets {

// Holds an experiment ID. So long as the feature is set through a server-side
// variations config, this feature should exist on the client. This ensures that
// the experiment ID is visible in chrome://snippets-internals.
const base::Feature kRemoteSuggestionsBackendFeature{
    "NTPRemoteSuggestionsBackend", base::FEATURE_DISABLED_BY_DEFAULT};

// Keep sorted, and keep nullptr at the end.
const base::Feature* const kAllFeatures[] = {
    &kArticleSuggestionsFeature,
    &kBookmarkSuggestionsFeature,
    &kBreakingNewsPushFeature,
    &kCategoryOrder,
    &kCategoryRanker,
    &kContentSuggestionsDebugLog,
    &kForeignSessionsSuggestionsFeature,
    &kIncreasedVisibility,
    &kKeepPrefetchedContentSuggestions,
    &kNotificationsFeature,
    &kPhysicalWebPageSuggestionsFeature,
    &kPublisherFaviconsFromNewServerFeature,
    &kRecentOfflineTabSuggestionsFeature,
    &kRemoteSuggestionsBackendFeature,
    nullptr};

const base::Feature kArticleSuggestionsFeature{
    "NTPArticleSuggestions", base::FEATURE_ENABLED_BY_DEFAULT};

const base::Feature kBookmarkSuggestionsFeature{
    "NTPBookmarkSuggestions", base::FEATURE_ENABLED_BY_DEFAULT};

const base::Feature kRecentOfflineTabSuggestionsFeature{
    "NTPOfflinePageSuggestions", base::FEATURE_DISABLED_BY_DEFAULT};

const base::Feature kIncreasedVisibility{"NTPSnippetsIncreasedVisibility",
                                         base::FEATURE_ENABLED_BY_DEFAULT};

const base::Feature kPhysicalWebPageSuggestionsFeature{
    "NTPPhysicalWebPageSuggestions", base::FEATURE_DISABLED_BY_DEFAULT};

const base::Feature kForeignSessionsSuggestionsFeature{
    "NTPForeignSessionsSuggestions", base::FEATURE_DISABLED_BY_DEFAULT};

const base::Feature kBreakingNewsPushFeature{"BreakingNewsPush",
                                             base::FEATURE_DISABLED_BY_DEFAULT};

const base::Feature kCategoryRanker{"ContentSuggestionsCategoryRanker",
                                    base::FEATURE_ENABLED_BY_DEFAULT};

const base::Feature kPublisherFaviconsFromNewServerFeature{
    "ContentSuggestionsFaviconsFromNewServer",
    base::FEATURE_DISABLED_BY_DEFAULT};

const base::Feature kRemoteSuggestionsEmulateM58FetchingSchedule{
    "RemoteSuggestionsEmulateM58FetchingSchedule",
    base::FEATURE_DISABLED_BY_DEFAULT};

const char kCategoryRankerConstantRanker[] = "constant";
const char kCategoryRankerClickBasedRanker[] = "click_based";
const char kCategoryRankerDefault[] = "default";
constexpr base::FeatureParam<CategoryRankerChoice>::Option
    kCategoryRankerParamOptions[] = {
        {CategoryRankerChoice::DEFAULT, kCategoryRankerDefault},
        {CategoryRankerChoice::CONSTANT, kCategoryRankerConstantRanker},
        {CategoryRankerChoice::CLICK_BASED, kCategoryRankerClickBasedRanker}};
constexpr base::FeatureParam<CategoryRankerChoice> kCategoryRankerParam{
    &kCategoryRanker, "category_ranker", CategoryRankerChoice::DEFAULT,
    &kCategoryRankerParamOptions};

std::unique_ptr<CategoryRanker> BuildSelectedCategoryRanker(
    PrefService* pref_service,
    std::unique_ptr<base::Clock> clock,
    bool is_chrome_home_enabled) {
  CategoryRankerChoice choice = kCategoryRankerParam.Get();
  if (choice == CategoryRankerChoice::DEFAULT) {
    if (is_chrome_home_enabled) {
      choice = CategoryRankerChoice::CONSTANT;
    } else {
      choice = CategoryRankerChoice::CLICK_BASED;
    }
  }

  switch (choice) {
    case CategoryRankerChoice::CONSTANT:
      return base::MakeUnique<ConstantCategoryRanker>();
    case CategoryRankerChoice::CLICK_BASED:
      return base::MakeUnique<ClickBasedCategoryRanker>(pref_service,
                                                        std::move(clock));
    case CategoryRankerChoice::DEFAULT:
      NOTREACHED();
  }
  return nullptr;
}

const base::Feature kCategoryOrder{"ContentSuggestionsCategoryOrder",
                                   base::FEATURE_DISABLED_BY_DEFAULT};

constexpr base::FeatureParam<CategoryOrderChoice>::Option
    kCategoryOrderParamOptions[] = {
        {CategoryOrderChoice::GENERAL, kCategoryOrderGeneral},
        {CategoryOrderChoice::EMERGING_MARKETS_ORIENTED,
         kCategoryOrderEmergingMarketsOriented}};
const base::FeatureParam<CategoryOrderChoice> kCategoryOrderParam{
    &kCategoryOrder, "category_order",
    CategoryOrderChoice::EMERGING_MARKETS_ORIENTED,
    &kCategoryOrderParamOptions};

const char kCategoryOrderGeneral[] = "general";
const char kCategoryOrderEmergingMarketsOriented[] =
    "emerging_markets_oriented";

CategoryOrderChoice GetSelectedCategoryOrder() {
  if (!base::FeatureList::IsEnabled(kCategoryOrder)) {
    return CategoryOrderChoice::GENERAL;
  }
  return kCategoryOrderParam.Get();
}

const base::Feature kNotificationsFeature = {"ContentSuggestionsNotifications",
                                             base::FEATURE_DISABLED_BY_DEFAULT};

const char kNotificationsPriorityParam[] = "priority";
const char kNotificationsTextParam[] = "text";
const char kNotificationsTextValuePublisher[] = "publisher";
const char kNotificationsTextValueSnippet[] = "snippet";
const char kNotificationsTextValueAndMore[] = "and_more";
const char kNotificationsKeepWhenFrontmostParam[] =
    "keep_notification_when_frontmost";
const char kNotificationsOpenToNTPParam[] = "open_to_ntp";
const char kNotificationsDailyLimit[] = "daily_limit";
const char kNotificationsIgnoredLimitParam[] = "ignored_limit";

const base::Feature kKeepPrefetchedContentSuggestions{
    "KeepPrefetchedContentSuggestions", base::FEATURE_DISABLED_BY_DEFAULT};

const base::Feature kContentSuggestionsDebugLog{
    "ContentSuggestionsDebugLog", base::FEATURE_DISABLED_BY_DEFAULT};

}  // namespace ntp_snippets
