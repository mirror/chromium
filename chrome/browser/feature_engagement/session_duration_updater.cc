// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/feature_engagement/session_duration_updater.h"

#include "base/observer_list.h"
#include "base/time/time.h"
#include "base/timer/elapsed_timer.h"
#include "base/values.h"
#include "chrome/common/pref_names.h"
#include "components/prefs/pref_service.h"
#include "components/prefs/scoped_user_pref_update.h"

namespace feature_engagement {

SessionDurationUpdater::SessionDurationUpdater(
    PrefService* pref_service,
    const char* observed_session_time_pref_key)
    : duration_tracker_observer_(this),
      pref_service_(pref_service),
      observed_session_time_pref_key_(observed_session_time_pref_key) {
  AddDurationTrackerObserver();
}

SessionDurationUpdater::~SessionDurationUpdater() = default;

base::TimeDelta SessionDurationUpdater::GetActiveSessionElapsedTime() {
  const base::DictionaryValue* dict =
      pref_service_->GetDictionary(prefs::kObservedSessionTime);
  const base::Value* dict_value =
      dict->FindKey(observed_session_time_pref_key_);
  const double stored_value = dict_value ? dict_value->GetDouble() : 0L;
  base::TimeDelta elapsed_time = base::TimeDelta::FromSeconds(stored_value);
  return !elapsed_timer_ || !is_session_active_
             ? elapsed_time
             : elapsed_time + elapsed_timer_->Elapsed();
}

void SessionDurationUpdater::AddObserver(Observer* observer) {
  observer_list_.AddObserver(observer);

  // Re-adds SessionDurationUpdater as an observer of
  // DesktopSessionDurationTracker if another feature is added after
  // SessionDurationUpdater was removed.
  if (!duration_tracker_observer_.IsObserving(
          metrics::DesktopSessionDurationTracker::Get())) {
    if (!is_session_active_) {
      elapsed_timer_.reset(new base::ElapsedTimer());
      is_session_active_ = true;
    }
    AddDurationTrackerObserver();
  }
}

void SessionDurationUpdater::RemoveObserver(Observer* observer) {
  observer_list_.RemoveObserver(observer);
  // If all the observer Features have removed themselves due to their active
  // time limits have been reached, the SessionDurationUpdater removes itself
  // as an observer of DesktopSessionDurationTracker.
  if (!observer_list_.might_have_observers()) {
    elapsed_timer_.reset();
    is_session_active_ = false;
    RemoveDurationTrackerObserver();
  }
}

void SessionDurationUpdater::OnSessionStarted(base::TimeTicks session_start) {
  elapsed_timer_.reset(new base::ElapsedTimer());
  is_session_active_ = true;
}

void SessionDurationUpdater::OnSessionEnded(base::TimeDelta elapsed) {
  // This case is only used during testing as that is the only case that
  // DesktopSessionDurationTracker isn't calling this on its observer.
  if (!duration_tracker_observer_.IsObserving(
          metrics::DesktopSessionDurationTracker::Get())) {
    return;
  }

  base::TimeDelta elapsed_session_time;

  const base::DictionaryValue* dict =
      pref_service_->GetDictionary(prefs::kObservedSessionTime);
  const base::Value* dict_value =
      dict->FindKey(observed_session_time_pref_key_);
  const double stored_value = dict_value ? dict_value->GetDouble() : 0L;

  elapsed_session_time += base::TimeDelta::FromSeconds(stored_value) + elapsed;

  DictionaryPrefUpdate update(pref_service_, prefs::kObservedSessionTime);
  update->SetKey(
      observed_session_time_pref_key_,
      base::Value(static_cast<double>(elapsed_session_time.InSeconds())));

  elapsed_timer_.reset();
  is_session_active_ = false;

  for (Observer& observer : observer_list_)
    observer.OnSessionEnded(elapsed_session_time);
}

void SessionDurationUpdater::AddDurationTrackerObserver() {
  duration_tracker_observer_.Add(metrics::DesktopSessionDurationTracker::Get());
}

void SessionDurationUpdater::RemoveDurationTrackerObserver() {
  duration_tracker_observer_.Remove(
      metrics::DesktopSessionDurationTracker::Get());
}

PrefService* SessionDurationUpdater::GetPrefs() {
  return pref_service_;
}

}  // namespace feature_engagement
