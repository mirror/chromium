// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/profiles/profile_statistics.h"

#include "base/bind.h"
#include "base/macros.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/profiles/profile_attributes_entry.h"
#include "chrome/browser/profiles/profile_attributes_storage.h"
#include "chrome/browser/profiles/profile_manager.h"
#include "chrome/browser/profiles/profile_statistics_aggregator.h"

ProfileStatistics::ProfileStatistics(Profile* profile)
    : profile_(profile), aggregator_(nullptr), weak_ptr_factory_(this) {
}

ProfileStatistics::~ProfileStatistics() {
}

void ProfileStatistics::GatherStatistics(
    const profiles::ProfileStatisticsCallback& callback) {
  LOG(ERROR) << "gather statistics";
  // IsValidProfile() can be false in unit tests.
  if (!g_browser_process->profile_manager()->IsValidProfile(profile_))
    return;
  DCHECK(!profile_->IsOffTheRecord() && !profile_->IsSystemProfile());

  if (HasAggregator()) {
    GetAggregator()->AddCallbackAndStartAggregator(callback);
  } else {
    // The statistics task may outlive ProfileStatistics in unit tests, so a
    // weak pointer is used for the callback.
    aggregator_ = new ProfileStatisticsAggregator(
        profile_, callback,
        base::Bind(&ProfileStatistics::DeregisterAggregator,
                   weak_ptr_factory_.GetWeakPtr()));
  }
}

bool ProfileStatistics::HasAggregator() const {
  return aggregator_ != nullptr;
}

ProfileStatisticsAggregator* ProfileStatistics::GetAggregator() const {
  return aggregator_.get();
}

void ProfileStatistics::DeregisterAggregator() {
  aggregator_ = nullptr;
}

