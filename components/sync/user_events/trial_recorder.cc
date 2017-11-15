// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/sync/user_events/trial_recorder.h"

#include <memory>
#include <utility>

#include "base/bind.h"
#include "base/bind_helpers.h"
#include "base/feature_list.h"
#include "base/metrics/field_trial_params.h"
#include "base/stl_util.h"
#include "base/time/time.h"
#include "components/sync/driver/sync_driver_switches.h"
#include "components/variations/active_field_trials.h"

using base::FeatureList;
using base::FieldTrialList;
using base::Time;
using base::TimeDelta;
using sync_pb::UserEventSpecifics;

namespace syncer {

namespace {

TimeDelta GetFieldTrialDelay() {
  return TimeDelta::FromSeconds(base::GetFieldTrialParamByFeatureAsInt(
      switches::kSyncUserFieldTrialEvents, "field_trial_delay_seconds",
      60 * 60 * 24));
}

size_t GetFieldTrialMaxCount() {
  return base::GetFieldTrialParamByFeatureAsInt(
      switches::kSyncUserFieldTrialEvents, "field_trial_max_count", 5);
}

}  // namespace

TrialRecorder::TrialRecorder(UserEventService* user_event_service)
    : user_event_service_(user_event_service) {
  DCHECK(user_event_service_);
  FieldTrialList::AddObserver(this);
}

TrialRecorder::~TrialRecorder() {
  FieldTrialList::RemoveObserver(this);
}

void TrialRecorder::OnEventCaseRecorded(
    UserEventSpecifics::EventCase event_case) {
  bool updated = false;
  if (recorded_events_.insert(event_case).second) {
    for (const std::pair<UserEventSpecifics::EventCase, std::string>& pair :
         event_trial_deps_) {
      if (pair.first == event_case) {
        updated |= MaybeUpdateTrialsToRecord(event_case, pair.second);
      }
    }
  }
  if (updated) {
    RecordFieldTrials();
  }
}

void TrialRecorder::RegisterDependentFieldTrial(
    UserEventSpecifics::EventCase event_case,
    const std::string& trial_name) {
  if (event_trial_deps_.insert(std::make_pair(event_case, trial_name)).second &&
      MaybeUpdateTrialsToRecord(event_case, trial_name)) {
    RecordFieldTrials();
  }
}

void TrialRecorder::OnFieldTrialGroupFinalized(const std::string& trial_name,
                                               const std::string& group_name) {
  for (const std::pair<UserEventSpecifics::EventCase, std::string>& pair :
       event_trial_deps_) {
    if (pair.second == trial_name &&
        MaybeUpdateTrialsToRecord(pair.first, trial_name)) {
      RecordFieldTrials();
      // The |trial_name| will only switch from not recorded to recorded once,
      // which means MaybeUpdateTrialsToRecord() is not going to return again
      // for the same |trial_name| that is held constant here. Safe to return.
      return;
    }
  }
}

bool TrialRecorder::MaybeUpdateTrialsToRecord(
    UserEventSpecifics::EventCase event_case,
    const std::string& trial_name) {
  // Assume the dependency between |event_case| and |trial_name| was already
  // registered instead of checking it, because all of our callers already do
  // this implicitly.
  DCHECK(base::ContainsKey(event_trial_deps_,
                           std::make_pair(event_case, trial_name)));

  if (!FeatureList::IsEnabled(switches::kSyncUserFieldTrialEvents) ||
      base::ContainsKey(trials_to_record_, trial_name) ||
      !base::ContainsKey(recorded_events_, event_case) ||
      !FieldTrialList::IsTrialActive(trial_name) ||
      trials_to_record_.size() >= GetFieldTrialMaxCount()) {
    return false;
  }

  trials_to_record_.insert(trial_name);
  return true;
}

void TrialRecorder::RecordFieldTrials() {
  DCHECK(!trials_to_record_.empty());
  auto specifics = std::make_unique<UserEventSpecifics>();
  specifics->set_event_time_usec((Time::Now() - Time()).InMicroseconds());

  for (std::string trial_name : trials_to_record_) {
    variations::ActiveGroupId field_ids = variations::MakeActiveGroupId(
        trial_name, FieldTrialList::FindFullName(trial_name));
    auto* pair =
        specifics->mutable_field_trial_event()->add_field_trial_pairs();
    pair->set_name_id(field_ids.name);
    pair->set_group_id(field_ids.group);
  }

  user_event_service_->RecordUserEvent(std::move(specifics));
  field_trial_timer_.Start(
      FROM_HERE, GetFieldTrialDelay(),
      base::Bind(&TrialRecorder::RecordFieldTrials, base::Unretained(this)));
}

}  // namespace syncer
