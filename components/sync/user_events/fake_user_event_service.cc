// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/sync/user_events/fake_user_event_service.h"

#include <utility>

using sync_pb::UserEventSpecifics;

namespace syncer {

FakeUserEventService::FakeUserEventService() {}

FakeUserEventService::~FakeUserEventService() {}

void FakeUserEventService::RecordUserEvent(
    std::unique_ptr<UserEventSpecifics> specifics) {
  DCHECK(specifics);
  RecordUserEvent(*specifics);
}

void FakeUserEventService::RecordUserEvent(
    const UserEventSpecifics& specifics) {
  recorded_user_events_.push_back(specifics);
}

base::WeakPtr<ModelTypeSyncBridge> FakeUserEventService::GetSyncBridge() {
  return base::WeakPtr<ModelTypeSyncBridge>();
}

void FakeUserEventService::RegisterDependentFieldTrial(
    UserEventSpecifics::EventCase event_case,
    const std::string& trial_name) {
  registered_dependent_field_trials_.insert(
      std::make_pair(event_case, trial_name));
}

const std::vector<UserEventSpecifics>&
FakeUserEventService::GetRecordedUserEvents() const {
  return recorded_user_events_;
}

const std::multimap<sync_pb::UserEventSpecifics::EventCase, std::string>&
FakeUserEventService::GetRegisteredDependentFieldTrials() const {
  return registered_dependent_field_trials_;
}

}  // namespace syncer
