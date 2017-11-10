// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/sync/user_events/user_event_service_impl.h"

#include <utility>

#include "base/bind.h"
#include "base/feature_list.h"

#include "base/rand_util.h"
#include "base/stl_util.h"
#include "base/time/time.h"
#include "components/sync/driver/sync_driver_switches.h"
#include "components/sync/driver/sync_service.h"
#include "components/sync/model/model_type_sync_bridge.h"
#include "components/sync/user_events/user_event_sync_bridge.h"
#include "components/variations/active_field_trials.h"

using sync_pb::UserEventSpecifics;

namespace syncer {

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

void UserEventServiceImpl::Shutdown() {}

void UserEventServiceImpl::RecordUserEvent(
    std::unique_ptr<UserEventSpecifics> specifics) {
  if (ShouldRecordEvent(*specifics)) {
    DCHECK(!specifics->has_session_id());
    specifics->set_session_id(session_id_);

    if (recorded_events_.insert(specifics->event_case()).second) {
      for (const std::pair<sync_pb::UserEventSpecifics::EventCase, std::string>&
               pair : event_trial_deps_) {
        if (pair.first == specifics->event_case()) {
          TryRecordFieldTrial(specifics->event_case(), pair.second);
        }
      }
    }

    bridge_->RecordUserEvent(std::move(specifics));
  }
}

void UserEventServiceImpl::RecordUserEvent(
    const UserEventSpecifics& specifics) {
  RecordUserEvent(std::make_unique<UserEventSpecifics>(specifics));
}

base::WeakPtr<ModelTypeSyncBridge> UserEventServiceImpl::GetSyncBridge() {
  return bridge_->AsWeakPtr();
}

// static
bool UserEventServiceImpl::MightRecordEvents(bool off_the_record,
                                             SyncService* sync_service) {
  return !off_the_record && sync_service &&
         base::FeatureList::IsEnabled(switches::kSyncUserEvents);
}

bool UserEventServiceImpl::ShouldRecordEvent(
    const UserEventSpecifics& specifics) {
  // TODO(skym): These checks do not make sense for events that are not tied to
  // navigations. In the past, everything has been tied to a navigation, but
  // we're about to leave that world. Should this method verify that event case
  // and presence of navigation id match our expectations?

  // We only record events if the user is syncing history (as indicated by
  // GetPreferredDataTypes()) and has not enabled a custom passphrase (as
  // indicated by IsUsingSecondaryPassphrase()).
  return sync_service_->IsEngineInitialized() &&
         !sync_service_->IsUsingSecondaryPassphrase() &&
         sync_service_->GetPreferredDataTypes().Has(HISTORY_DELETE_DIRECTIVES);
}

// TODO(skym): Switch the order of these arguments.
void UserEventServiceImpl::RegisterDependentFieldTrial(
    const std::string& trial_name,
    UserEventSpecifics::EventCase event_case) {
  if (event_trial_deps_.insert(std::make_pair(event_case, trial_name)).second) {
    TryRecordFieldTrial(event_case, trial_name);
  }
}

void UserEventServiceImpl::OnFieldTrialGroupFinalized(
    const std::string& trial_name,
    const std::string& group_name) {
  for (const std::pair<sync_pb::UserEventSpecifics::EventCase, std::string>&
           pair : event_trial_deps_) {
    if (pair.second == trial_name) {
      TryRecordFieldTrial(pair.first, trial_name);
    }
  }
}

void UserEventServiceImpl::TryRecordFieldTrial(
    UserEventSpecifics::EventCase event_case,
    const std::string& trial_name) {
  if (!base::FeatureList::IsEnabled(switches::kSyncUserFieldTrialEvents)) {
    return;
  }

  // There are four conditions that must be met for us to proceed:
  // 1) |event_case| dependency on |trial_name| has been registered.
  // 2) |event_case| has actually been recorded.
  // 3) |trial_name| has not been recorded.
  // 4) |trial_name| is active.

  // We assume the dependency between |event_case| and |trial_name| was already
  // registered instead of checking it, because all of our callers already do
  // this implicitly.
  DCHECK(base::ContainsKey(event_trial_deps_,
                           std::make_pair(event_case, trial_name)));

  // The check if the trial is active last because it acquires a lock.
  if (!base::ContainsKey(emitted_trials_, trial_name) &&
      base::ContainsKey(recorded_events_, event_case) &&
      base::FieldTrialList::IsTrialActive(trial_name)) {
    auto specifics = std::make_unique<UserEventSpecifics>();
    specifics->set_event_time_usec(
        (base::Time::Now() - base::Time()).InMicroseconds());

    // TODO(skym): Limit the total number of trials that can be emitted.

    // TODO(skym): In the past we've spoken about potentially emitting all trail
    // data that's being recorded for this session on each FieldTrial event,
    // do we still want to do that?
    variations::ActiveGroupId field_ids = variations::MakeActiveGroupId(
        trial_name, base::FieldTrialList::FindFullName(trial_name));
    auto* pair =
        specifics->mutable_field_trial_event()->add_field_trial_pairs();
    pair->set_name_id(field_ids.name);
    pair->set_group_id(field_ids.group);

    RecordUserEvent(std::move(specifics));
    // TODO(skym): This is not technically safe. It's possible we recorded the
    // dependent event, then something changed that'll cause ShouldRecordEvent
    // to return false, and then a trial was finalized. The RecordUserEvent will
    // no-op and our insert to |emitted_trials_| will fail. However, we're not
    // going to address this right now because in the future ShouldRecordEvent
    // will likely stop blocking FieldTrial events, this situation is quite the
    // edge case, and the only impact is fairly minor, some events are missed.
    emitted_trials_.insert(trial_name);
  }
}

}  // namespace syncer
