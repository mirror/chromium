// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/sync/user_events/trial_recorder.h"

#include <memory>
#include <utility>

#include "base/bind.h"
#include "base/bind_helpers.h"
#include "base/feature_list.h"
#include "base/metrics/field_trial.h"
#include "base/metrics/field_trial_params.h"
#include "base/stl_util.h"
#include "base/time/time.h"
#include "components/sync/driver/sync_driver_switches.h"
#include "components/variations/active_field_trials.h"
#include "components/variations/variations_associated_data.h"

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

}  // namespace

TrialRecorder::TrialRecorder(UserEventService* user_event_service)
    : user_event_service_(user_event_service) {
  DCHECK(user_event_service_);
}

TrialRecorder::~TrialRecorder() {}

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

bool TrialRecorder::MaybeUpdateTrialsToRecord(
    UserEventSpecifics::EventCase event_case,
    const std::string& trial_name) {
  // Assume the dependency between |event_case| and |trial_name| was already
  // registered instead of checking it, because all of our callers already do
  // this implicitly.
  DCHECK(base::ContainsKey(event_trial_deps_,
                           std::make_pair(event_case, trial_name)));

  if (!FeatureList::IsEnabled(switches::kSyncUserFieldTrialEvents) ||
      !base::ContainsKey(recorded_events_, event_case)) {
    return false;
  }

  variations::VariationID variation_id = variations::GetGoogleVariationID(
      variations::CHROME_SYNC_SERVICE, trial_name,
      FieldTrialList::FindFullName(trial_name));

  if (variation_id == variations::EMPTY_ID) {
    return false;
  }

  return variation_ids_to_record_.insert(variation_id).second;
}

void TrialRecorder::RecordFieldTrials() {
  DCHECK(!variation_ids_to_record_.empty());
  auto specifics = std::make_unique<UserEventSpecifics>();
  specifics->set_event_time_usec((Time::Now() - Time()).InMicroseconds());

  for (variations::VariationID variation_id : variation_ids_to_record_) {
    DCHECK_NE(variations::EMPTY_ID, variation_id);
    specifics->mutable_field_trial_event()->add_variation_ids(variation_id);
  }

  user_event_service_->RecordUserEvent(std::move(specifics));
  field_trial_timer_.Start(
      FROM_HERE, GetFieldTrialDelay(),
      base::Bind(&TrialRecorder::RecordFieldTrials, base::Unretained(this)));
}

}  // namespace syncer
