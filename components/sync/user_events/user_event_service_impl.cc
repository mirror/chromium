// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/sync/user_events/user_event_service_impl.h"

#include <utility>

#include "base/bind.h"
#include "base/feature_list.h"

#include "base/bind_helpers.h"
#include "base/metrics/field_trial_params.h"
#include "base/rand_util.h"
#include "base/stl_util.h"
#include "base/time/time.h"
#include "components/sync/driver/sync_driver_switches.h"
#include "components/sync/driver/sync_service.h"
#include "components/sync/model/model_type_sync_bridge.h"
#include "components/sync/user_events/user_event_sync_bridge.h"
#include "components/variations/active_field_trials.h"

using base::TimeDelta;
using sync_pb::UserEventSpecifics;

namespace syncer {

namespace {

enum NavigationPresence {
  kMustHave,
  kCannotHave,
  kEitherOkay,
};

NavigationPresence GetNavigationPresence(
    UserEventSpecifics::EventCase event_case) {
  switch (event_case) {
    case UserEventSpecifics::kTestEvent:
      return kEitherOkay;
    case UserEventSpecifics::kFieldTrialEvent:
      return kCannotHave;
    case UserEventSpecifics::kLanguageDetectionEvent:
      return kMustHave;
    case UserEventSpecifics::kTranslationEvent:
      return kMustHave;
    case UserEventSpecifics::kGaiaPasswordReuseEvent:
      return kMustHave;
    case UserEventSpecifics::EVENT_NOT_SET:
      NOTREACHED();
      return kEitherOkay;
  }
}

bool NavigationPresenceValid(UserEventSpecifics::EventCase event_case,
                             bool has_navigation_id) {
  NavigationPresence presence = GetNavigationPresence(event_case);
  return presence == kEitherOkay ||
         (presence == kMustHave && has_navigation_id) ||
         (presence == kCannotHave && !has_navigation_id);
}

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

UserEventServiceImpl::UserEventServiceImpl(
    SyncService* sync_service,
    std::unique_ptr<UserEventSyncBridge> bridge)
    : sync_service_(sync_service),
      bridge_(std::move(bridge)),
      session_id_(base::RandUint64()) {
  DCHECK(bridge_);
  DCHECK(sync_service_);
  base::FieldTrialList::AddObserver(this);
}

UserEventServiceImpl::~UserEventServiceImpl() {}

void UserEventServiceImpl::Shutdown() {
  base::FieldTrialList::RemoveObserver(this);
}

void UserEventServiceImpl::RecordUserEvent(
    std::unique_ptr<UserEventSpecifics> specifics) {
  if (ShouldRecordEvent(*specifics)) {
    DCHECK(!specifics->has_session_id());
    specifics->set_session_id(session_id_);
    bool updated = false;

    if (recorded_events_.insert(specifics->event_case()).second) {
      for (const std::pair<sync_pb::UserEventSpecifics::EventCase, std::string>&
               pair : event_trial_deps_) {
        if (pair.first == specifics->event_case()) {
          updated |=
              MaybeUpdateTrialsToRecord(specifics->event_case(), pair.second);
        }
      }
    }

    bridge_->RecordUserEvent(std::move(specifics));
    if (updated) {
      RecordFieldTrials();
    }
  }
}

void UserEventServiceImpl::RecordUserEvent(
    const UserEventSpecifics& specifics) {
  RecordUserEvent(std::make_unique<UserEventSpecifics>(specifics));
}

base::WeakPtr<ModelTypeSyncBridge> UserEventServiceImpl::GetSyncBridge() {
  return bridge_->AsWeakPtr();
}

void UserEventServiceImpl::RegisterDependentFieldTrial(
    UserEventSpecifics::EventCase event_case,
    const std::string& trial_name) {
  if (event_trial_deps_.insert(std::make_pair(event_case, trial_name)).second &&
      MaybeUpdateTrialsToRecord(event_case, trial_name)) {
    RecordFieldTrials();
  }
}

void UserEventServiceImpl::OnFieldTrialGroupFinalized(
    const std::string& trial_name,
    const std::string& group_name) {
  for (const std::pair<sync_pb::UserEventSpecifics::EventCase, std::string>&
           pair : event_trial_deps_) {
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

// static
bool UserEventServiceImpl::MightRecordEvents(bool off_the_record,
                                             SyncService* sync_service) {
  return !off_the_record && sync_service &&
         base::FeatureList::IsEnabled(switches::kSyncUserEvents);
}

bool UserEventServiceImpl::CanRecordHistory() {
  // Before the engine is initialized, we cannot trust the other fields.
  return sync_service_->IsEngineInitialized() &&
         !sync_service_->IsUsingSecondaryPassphrase() &&
         sync_service_->GetPreferredDataTypes().Has(HISTORY_DELETE_DIRECTIVES);
}

bool UserEventServiceImpl::ShouldRecordEvent(
    const UserEventSpecifics& specifics) {
  if (specifics.event_case() == UserEventSpecifics::EVENT_NOT_SET) {
    return false;
  }

  if (!NavigationPresenceValid(specifics.event_case(),
                               specifics.has_navigation_id())) {
    return false;
  }

  if (specifics.has_navigation_id() && !CanRecordHistory()) {
    return false;
  }

  return true;
}

bool UserEventServiceImpl::MaybeUpdateTrialsToRecord(
    UserEventSpecifics::EventCase event_case,
    const std::string& trial_name) {
  // Assume the dependency between |event_case| and |trial_name| was already
  // registered instead of checking it, because all of our callers already do
  // this implicitly.
  DCHECK(base::ContainsKey(event_trial_deps_,
                           std::make_pair(event_case, trial_name)));

  if (!base::FeatureList::IsEnabled(switches::kSyncUserFieldTrialEvents) ||
      base::ContainsKey(trials_to_record_, trial_name) ||
      !base::ContainsKey(recorded_events_, event_case) ||
      !base::FieldTrialList::IsTrialActive(trial_name) ||
      trials_to_record_.size() >= GetFieldTrialMaxCount()) {
    return false;
  }

  trials_to_record_.insert(trial_name);
  return true;
}

void UserEventServiceImpl::RecordFieldTrials() {
  DCHECK(!trials_to_record_.empty());
  auto specifics = std::make_unique<UserEventSpecifics>();
  specifics->set_event_time_usec(
      (base::Time::Now() - base::Time()).InMicroseconds());

  for (std::string trial_name : trials_to_record_) {
    variations::ActiveGroupId field_ids = variations::MakeActiveGroupId(
        trial_name, base::FieldTrialList::FindFullName(trial_name));
    auto* pair =
        specifics->mutable_field_trial_event()->add_field_trial_pairs();
    pair->set_name_id(field_ids.name);
    pair->set_group_id(field_ids.group);
  }

  RecordUserEvent(std::move(specifics));
  field_trial_timer_.Start(FROM_HERE, GetFieldTrialDelay(),
                           base::Bind(&UserEventServiceImpl::RecordFieldTrials,
                                      base::Unretained(this)));
}

}  // namespace syncer
