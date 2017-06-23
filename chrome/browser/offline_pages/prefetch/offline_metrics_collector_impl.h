// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_OFFLINE_PAGES_PREFETCH_OFFLINE_METRICS_COLLECTOR_IMPL_H_
#define CHROME_BROWSER_OFFLINE_PAGES_PREFETCH_OFFLINE_METRICS_COLLECTOR_IMPL_H_

#include <memory>

#include "base/macros.h"
#include "base/time/clock.h"
#include "components/offline_pages/core/prefetch/offline_metrics_collector.h"
#include "components/prefs/pref_registry_simple.h"
#include "components/prefs/pref_service.h"

namespace offline_pages {

// Accumulates the type of usage of Chrome during a day (offline/online etc)
// and then reports it to UMA when network connection is likely on.
// The usage accumulation is done by day - by accumulating the usage type
// samples during the day. Incoming samples of usage are generated
// when Chrome loads pages and are accumulated according to rules below. The
// accumulated usage is stored on the disk (as a Pref).
// During accumulation, the usage type is 'upgraded'  when samples come in,
// following these rules:
// 1. New accumulated usage starts with UNKNOWN
// 2. Incoming STARTED only updates UNKNOWN -> STARTED, ignored otherwise.
// 3. OFFLINE or ONLINE upgrade UNKNOWN or STARTED -> OFFLINE or ONLINE,
//    respectively.
// 4. OFFLINE sample when accumulated usage is ONLINE or vice versa upgrades
//    accumulated usage to MIXED.
// 5. Once MIXED, usage type does not change anymore.
// 6. If incoming sample is the same as accumulated, no change.
// If incoming sample is for another day, that means the current accumulation
// needs to be converted into increment of one of the counters and reset, to
// start new accumulation.
// All changes in accumulated usage, if they happen, trigger an eventual attempt
// to write them to disk.
class OfflineMetricsCollectorImpl : public OfflineMetricsCollector {
 public:
  static void RegisterPrefs(PrefRegistrySimple* registry);

  // This class is used in UMA reporting. Keep it in sync with
  // OfflinePagesOfflineUsage enum in histograms. Public for testing.
  enum class UsageType {
    UNUSED = 0,   // Not used for a day.
    STARTED = 1,  // Started, but no successful navigations happened.
                  // Perhaps offline.
    OFFLINE = 2,  // Only successful offline navigations happened (however
                  // device may be connected).
    ONLINE = 3,   // Only online navigations happened.
    MIXED = 4,    // Both offline and online navigations happened.
    MAX_USAGE = 5,
  };

  explicit OfflineMetricsCollectorImpl(PrefService* prefs);
  ~OfflineMetricsCollectorImpl() override;

  // OfflineMetricsCollector implementation.
  void OnAppStartupOrResume() override;
  void OnSuccessfulNavigationOnline() override;
  void OnSuccessfulNavigationOffline() override;
  void ReportAccumulatedStats() override;

  void SetClockForTesting(base::Clock* clock);

 private:
  void EnsureLoaded();
  void SaveToPrefs();

  // Returns 'true' if the accumulated state changed and needs to be updated on
  // disk.
  bool ApplyUsageSample(UsageType sample);
  void UpdateWithSample(UsageType sample);
  // Returns 'true' if the past days counters changed and need to be updated on
  // disk.
  bool UpdatePastDaysIfNeeded();
  void ReportUsageUma(UsageType usage_type);

  base::Time Now() const;

  // Has the same lifetime as profile, so should outlive this subcomponent
  // of profile's PrefetchService.
  PrefService* prefs_ = nullptr;

  // In-memory copy of the accumulated usage. It is kept in sync with a copy
  // saved in Prefs.
  UsageType accumulated_usage_type_ = UsageType::UNUSED;
  base::Time timestamp_;

  // Accumulated counters. Kept in sync with a copy saved in Prefs.
  int unused_days_count_ = 0;
  int started_days_count_ = 0;
  int offline_days_count_ = 0;
  int online_days_count_ = 0;
  int mixed_days_count_ = 0;

  // Used in tests, managed by the test, overlives this object.
  base::Clock* testing_clock_ = nullptr;

  DISALLOW_COPY_AND_ASSIGN(OfflineMetricsCollectorImpl);
};

}  // namespace offline_pages

#endif  // CHROME_BROWSER_OFFLINE_PAGES_PREFETCH_OFFLINE_METRICS_COLLECTOR_IMPL_H_
