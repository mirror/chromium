// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/profiles/profile_statistics_aggregator.h"

#include <stddef.h>

#include "base/bind.h"
#include "base/macros.h"
#include "base/time/time.h"
#include "chrome/browser/bookmarks/bookmark_model_factory.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/history/history_service_factory.h"
#include "chrome/browser/password_manager/password_store_factory.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/profiles/profile_manager.h"
#include "chrome/browser/profiles/profile_statistics.h"
#include "chrome/browser/profiles/profile_statistics_factory.h"
#include "chrome/browser/web_data_service_factory.h"
#include "components/browsing_data/core/counters/autofill_counter.h"
#include "components/browsing_data/core/counters/bookmark_counter.h"
#include "components/browsing_data/core/counters/history_counter.h"
#include "components/browsing_data/core/counters/passwords_counter.h"
#include "components/browsing_data/core/pref_names.h"
#include "content/public/browser/browser_thread.h"

namespace {

const int kProfileStatCategories = 4;

}  // namespace

ProfileStatisticsAggregator::ProfileStatisticsAggregator(
    Profile* profile,
    const profiles::ProfileStatisticsCallback& stats_callback,
    const base::Closure& done_callback)
    : profile_(profile),
      profile_path_(profile_->GetPath()),
      done_callback_(done_callback) {
  AddCallbackAndStartAggregator(stats_callback);
}

ProfileStatisticsAggregator::~ProfileStatisticsAggregator() {}

size_t ProfileStatisticsAggregator::GetCallbackCount() {
  return stats_callbacks_.size();
}

void ProfileStatisticsAggregator::AddCallbackAndStartAggregator(
    const profiles::ProfileStatisticsCallback& stats_callback) {
  if (!stats_callback.is_null())
    stats_callbacks_.push_back(stats_callback);
  StartAggregator();
}

void ProfileStatisticsAggregator::AddCounter(
    std::unique_ptr<browsing_data::BrowsingDataCounter> counter) {
  counter->InitWithoutPref(base::Bind(
      &ProfileStatisticsAggregator::CounterCallback, base::Unretained(this)));
  counter->Restart();
  counters_.push_back(std::move(counter));
}

void ProfileStatisticsAggregator::StartAggregator() {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  DCHECK(g_browser_process->profile_manager()->IsValidProfile(profile_));
  profile_category_stats_.clear();

  // Cancel tasks.
  counters_.clear();

  // Initiate bookmark counting.
  bookmarks::BookmarkModel* bookmark_model =
      BookmarkModelFactory::GetForBrowserContextIfExists(profile_);
  if (bookmark_model) {
    AddCounter(
        base::MakeUnique<browsing_data::BookmarkCounter>(bookmark_model));
  } else {
    StatisticsCallbackFailure(profiles::kProfileStatisticsBookmarks);
  }

  // Initiate history counting.
  history::HistoryService* history_service =
      HistoryServiceFactory::GetForProfileWithoutCreating(profile_);

  if (history_service) {
    AddCounter(base::MakeUnique<browsing_data::HistoryCounter>(
        history_service,
        browsing_data::HistoryCounter::GetUpdatedWebHistoryServiceCallback(),
        /*sync_service=*/nullptr));
  } else {
    StatisticsCallbackFailure(profiles::kProfileStatisticsBrowsingHistory);
  }

  // Initiate stored password counting.
  scoped_refptr<password_manager::PasswordStore> password_store =
      PasswordStoreFactory::GetForProfile(
          profile_, ServiceAccessType::EXPLICIT_ACCESS);
  if (password_store) {
    AddCounter(base::MakeUnique<browsing_data::PasswordsCounter>(
        password_store, /*sync_service=*/nullptr));
  } else {
    StatisticsCallbackFailure(profiles::kProfileStatisticsPasswords);
  }

  // Initiate autofill counting.
  scoped_refptr<autofill::AutofillWebDataService> autofill_service =
      WebDataServiceFactory::GetAutofillWebDataForProfile(
          profile_, ServiceAccessType::EXPLICIT_ACCESS);
  if (password_store) {
    AddCounter(base::MakeUnique<browsing_data::AutofillCounter>(
        autofill_service, /*sync_service=*/nullptr));
  } else {
    StatisticsCallbackFailure(profiles::kProfileStatisticsAutofill);
  }
}

void ProfileStatisticsAggregator::CounterCallback(
    std::unique_ptr<browsing_data::BrowsingDataCounter::Result> result) {
  if (!result->Finished())
    return;
  const char* pref_name = result->source()->GetPrefName();
  int count = static_cast<browsing_data::BrowsingDataCounter::FinishedResult*>(
                  result.get())
                  ->Value();
  if (pref_name == browsing_data::BookmarkCounter::kFakePrefName) {
    StatisticsCallbackSuccess(profiles::kProfileStatisticsBookmarks, count);
  } else if (pref_name == browsing_data::prefs::kDeleteBrowsingHistory) {
    StatisticsCallbackSuccess(profiles::kProfileStatisticsBrowsingHistory,
                              count);
  } else if (pref_name == browsing_data::prefs::kDeletePasswords) {
    StatisticsCallbackSuccess(profiles::kProfileStatisticsPasswords, count);
  } else if (pref_name == browsing_data::prefs::kDeleteFormData) {
    StatisticsCallbackSuccess(profiles::kProfileStatisticsAutofill, count);
  } else {
    NOTREACHED();
  }
}

void ProfileStatisticsAggregator::StatisticsCallback(
    const char* category, ProfileStatValue result) {
  profiles::ProfileCategoryStat datum;
  datum.category = category;
  datum.count = result.count;
  datum.success = result.success;
  profile_category_stats_.push_back(datum);
  for (const auto& stats_callback : stats_callbacks_) {
    DCHECK(stats_callback);
    stats_callback.Run(profile_category_stats_);
  }

  if (result.success) {
    ProfileStatistics::SetProfileStatisticsToAttributesStorage(
        profile_path_, datum.category, result.count);
  }
  if (profile_category_stats_.size() == kProfileStatCategories) {
    if (!done_callback_.is_null())
      done_callback_.Run();
  }
}

void ProfileStatisticsAggregator::StatisticsCallbackSuccess(
    const char* category, int count) {
  ProfileStatValue result;
  result.count = count;
  result.success = true;
  StatisticsCallback(category, result);
}

void ProfileStatisticsAggregator::StatisticsCallbackFailure(
    const char* category) {
  ProfileStatValue result;
  result.count = 0;
  result.success = false;
  StatisticsCallback(category, result);
}

