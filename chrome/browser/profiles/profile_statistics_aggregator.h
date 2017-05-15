// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_PROFILES_PROFILE_STATISTICS_AGGREGATOR_H_
#define CHROME_BROWSER_PROFILES_PROFILE_STATISTICS_AGGREGATOR_H_

#include <memory>
#include <vector>

#include "base/callback_forward.h"
#include "base/files/file_path.h"
#include "chrome/browser/profiles/profile_statistics_common.h"
#include "components/browsing_data/core/counters/browsing_data_counter.h"

class Profile;

class ProfileStatisticsAggregator {
  // This class is used internally by GetProfileStatistics
  //
  // The class collects statistical information about the profile and returns
  // the information via a callback function. Currently bookmarks, history,
  // logins and sites with autofill forms are counted.

 public:
  ProfileStatisticsAggregator(
      Profile* profile,
      const profiles::ProfileStatisticsCallback& stats_callback,
      const base::Closure& done_callback);

  ~ProfileStatisticsAggregator();

  void AddCallbackAndStartAggregator(
      const profiles::ProfileStatisticsCallback& stats_callback);

  // The method below is used for testing.
  size_t GetCallbackCount();

 private:
  struct ProfileStatValue {
    int count = 0;
    bool success = false;  // False means the statistics failed to load.
  };

  // Start gathering statistics. Also cancels existing statistics tasks.
  void StartAggregator();

  // Callback functions
  // Normal callback. Appends result to |profile_category_stats_|, and then call
  // the external callback. All other callbacks call this function.
  void StatisticsCallback(const char* category, ProfileStatValue result);
  // Callback for reporting success.
  void StatisticsCallbackSuccess(const char* category, int count);
  // Callback for reporting failure.
  void StatisticsCallbackFailure(const char* category);

  // Callback for counters.
  void CounterCallback(
      std::unique_ptr<browsing_data::BrowsingDataCounter::Result> result);

  // Registers, initializes and starts a BrowsingDataCounter.
  void AddCounter(std::unique_ptr<browsing_data::BrowsingDataCounter> counter);

  Profile* profile_;
  base::FilePath profile_path_;
  profiles::ProfileCategoryStats profile_category_stats_;

  // Callback function to be called when results arrive. Will be called
  // multiple times (once for each statistics).
  std::vector<profiles::ProfileStatisticsCallback> stats_callbacks_;

  // Callback function to be called when all statistics are calculated.
  base::Closure done_callback_;

  std::vector<std::unique_ptr<browsing_data::BrowsingDataCounter>> counters_;

  DISALLOW_COPY_AND_ASSIGN(ProfileStatisticsAggregator);
};

#endif  // CHROME_BROWSER_PROFILES_PROFILE_STATISTICS_AGGREGATOR_H_
