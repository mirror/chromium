// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/offline_pages/prefetch/offline_metrics_collector_impl.h"

#include "base/logging.h"
#include "base/metrics/histogram_macros.h"
#include "chrome/common/pref_names.h"

namespace offline_pages {

// static
void OfflineMetricsCollectorImpl::RegisterPrefs(PrefRegistrySimple* registry) {
  registry->RegisterIntegerPref(prefs::kOfflineAccumulatedUsage, 0);
  registry->RegisterInt64Pref(prefs::kOfflineAccumulatedTimestamp, 0L);
  registry->RegisterIntegerPref(prefs::kOfflineUsageUnusedCount, 0);
  registry->RegisterIntegerPref(prefs::kOfflineUsageStartedCount, 0);
  registry->RegisterIntegerPref(prefs::kOfflineUsageOfflineCount, 0);
  registry->RegisterIntegerPref(prefs::kOfflineUsageOnlineCount, 0);
  registry->RegisterIntegerPref(prefs::kOfflineUsageMixedCount, 0);
}

OfflineMetricsCollectorImpl::OfflineMetricsCollectorImpl(PrefService* prefs)
    : prefs_(prefs) {}

OfflineMetricsCollectorImpl::~OfflineMetricsCollectorImpl() {}

void OfflineMetricsCollectorImpl::OnAppStartupOrResume() {
  EnsureLoaded();
  UpdateWithSample(UsageType::STARTED);
}

void OfflineMetricsCollectorImpl::OnSuccessfulNavigationOffline() {
  EnsureLoaded();
  UpdateWithSample(UsageType::OFFLINE);
}

void OfflineMetricsCollectorImpl::OnSuccessfulNavigationOnline() {
  EnsureLoaded();
  UpdateWithSample(UsageType::ONLINE);
}

void OfflineMetricsCollectorImpl::ReportAccumulatedStats() {
  int total_day_count = unused_days_count_ + started_days_count_ +
                        offline_days_count_ + online_days_count_ +
                        mixed_days_count_;
  if (total_day_count == 0)
    return;

  for (int i = 0; i < unused_days_count_; ++i)
    ReportUsageUma(UsageType::UNUSED);
  for (int i = 0; i < started_days_count_; ++i)
    ReportUsageUma(UsageType::STARTED);
  for (int i = 0; i < offline_days_count_; ++i)
    ReportUsageUma(UsageType::OFFLINE);
  for (int i = 0; i < online_days_count_; ++i)
    ReportUsageUma(UsageType::ONLINE);
  for (int i = 0; i < mixed_days_count_; ++i)
    ReportUsageUma(UsageType::MIXED);

  unused_days_count_ = started_days_count_ = offline_days_count_ =
      online_days_count_ = mixed_days_count_ = 0;
  SaveToPrefs();
}

void OfflineMetricsCollectorImpl::EnsureLoaded() {
  if (!accumulation_midnight_.is_null())
    return;

  DCHECK(prefs_);
  accumulated_usage_type_ = static_cast<UsageType>(
      prefs_->GetInteger(prefs::kOfflineAccumulatedUsage));
  int64_t time_value = prefs_->GetInt64(prefs::kOfflineAccumulatedTimestamp);
  // For the very first run, initialize to current time.
  accumulation_midnight_ = time_value == 0L
                               ? Now().LocalMidnight()
                               : base::Time::FromInternalValue(time_value);
  unused_days_count_ = prefs_->GetInteger(prefs::kOfflineUsageUnusedCount);
  started_days_count_ = prefs_->GetInteger(prefs::kOfflineUsageStartedCount);
  offline_days_count_ = prefs_->GetInteger(prefs::kOfflineUsageOfflineCount);
  online_days_count_ = prefs_->GetInteger(prefs::kOfflineUsageOnlineCount);
  mixed_days_count_ = prefs_->GetInteger(prefs::kOfflineUsageMixedCount);
}

void OfflineMetricsCollectorImpl::SaveToPrefs() {
  prefs_->SetInteger(prefs::kOfflineAccumulatedUsage,
                     static_cast<int>(accumulated_usage_type_));
  prefs_->SetInt64(prefs::kOfflineAccumulatedTimestamp,
                   accumulation_midnight_.ToInternalValue());
  prefs_->SetInteger(prefs::kOfflineUsageUnusedCount, unused_days_count_);
  prefs_->SetInteger(prefs::kOfflineUsageStartedCount, started_days_count_);
  prefs_->SetInteger(prefs::kOfflineUsageOfflineCount, offline_days_count_);
  prefs_->SetInteger(prefs::kOfflineUsageOnlineCount, online_days_count_);
  prefs_->SetInteger(prefs::kOfflineUsageMixedCount, mixed_days_count_);
  prefs_->CommitPendingWrite();
}

bool OfflineMetricsCollectorImpl::ApplyUsageSample(UsageType sample) {
  DCHECK(sample == UsageType::STARTED || sample == UsageType::ONLINE ||
         sample == UsageType::OFFLINE);

  UsageType ac = accumulated_usage_type_;

  if (ac == sample || ac == UsageType::MIXED)
    return false;

  if (ac == UsageType::UNUSED || ac == UsageType::STARTED) {
    accumulated_usage_type_ = sample;
    return true;
  }

  if ((ac == UsageType::OFFLINE && sample == UsageType::ONLINE) ||
      (ac == UsageType::ONLINE && sample == UsageType::OFFLINE)) {
    accumulated_usage_type_ = UsageType::MIXED;
    return true;
  }
  return false;
}

void OfflineMetricsCollectorImpl::UpdateWithSample(UsageType sample) {
  // First, see if today is a new day and we need to update past days counters.
  bool changed = UpdatePastDaysIfNeeded();
  // Now, apply a new sample to accumlated value.
  if (ApplyUsageSample(sample) || changed)
    SaveToPrefs();
}

bool OfflineMetricsCollectorImpl::UpdatePastDaysIfNeeded() {
  base::Time current_midnight = Now().LocalMidnight();
  // It is still the same day, ro a day from the future (rarely may happen when
  // clock is reset), skip updating past days counters.
  if (accumulation_midnight_ >= current_midnight)
    return false;

  // It is a different day. Increment the counter for the last accumulated day.
  switch (accumulated_usage_type_) {
    case UsageType::UNUSED:
      unused_days_count_++;
      break;
    case UsageType::STARTED:
      started_days_count_++;
      break;
    case UsageType::OFFLINE:
      offline_days_count_++;
      break;
    case UsageType::ONLINE:
      online_days_count_++;
      break;
    case UsageType::MIXED:
      mixed_days_count_++;
      break;
    default:
      NOTREACHED();
      break;
  }

  // Assume the days between that day and the current one are 'unused', not
  // including the current day because its usage will be accumulated
  // by incoming samples.
  // Calculation of the 'days in between' is as following:
  // 1. If current_midnight == accumulation_midnight_, we returned earlier.
  // 2. If current_midnight is for the next day, days_in_between is 0,
  //    accumulation is started for the current day.
  // 3. If current_midnight is > 48hrs later, the days_in_between are added.
  int days_in_between =
      (current_midnight - accumulation_midnight_).InDays() - 1;
  DCHECK(days_in_between >= 0);
  unused_days_count_ += days_in_between;

  // Reset accumulation.
  accumulated_usage_type_ = UsageType::UNUSED;
  accumulation_midnight_ = current_midnight;

  return true;
}

void OfflineMetricsCollectorImpl::ReportUsageUma(UsageType usage_type) {
  UMA_HISTOGRAM_ENUMERATION("OfflinePages.OfflineUsage", usage_type,
                            UsageType::MAX_USAGE);
}

base::Time OfflineMetricsCollectorImpl::Now() const {
  if (testing_clock_)
    return testing_clock_->Now();
  return base::Time::Now();
}

void OfflineMetricsCollectorImpl::SetClockForTesting(base::Clock* clock) {
  testing_clock_ = clock;
}

}  // namespace offline_pages
