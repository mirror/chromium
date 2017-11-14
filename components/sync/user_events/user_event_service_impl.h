// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_SYNC_USER_EVENTS_USER_EVENT_SERVICE_IMPL_H_
#define COMPONENTS_SYNC_USER_EVENTS_USER_EVENT_SERVICE_IMPL_H_

#include <memory>
#include <set>
#include <string>
#include <utility>

#include "base/compiler_specific.h"
#include "base/macros.h"
#include "base/memory/weak_ptr.h"
#include "base/metrics/field_trial.h"
#include "base/timer/timer.h"
#include "components/keyed_service/core/keyed_service.h"
#include "components/sync/protocol/user_event_specifics.pb.h"
#include "components/sync/user_events/user_event_service.h"

namespace syncer {

class ModelTypeSyncBridge;
class SyncService;
class UserEventSyncBridge;

class UserEventServiceImpl : public UserEventService,
                             base::FieldTrialList::Observer {
 public:
  UserEventServiceImpl(SyncService* sync_service,
                       std::unique_ptr<UserEventSyncBridge> bridge);
  ~UserEventServiceImpl() override;

  // KeyedService implementation.
  void Shutdown() override;

  // UserEventService implementation.
  void RecordUserEvent(
      std::unique_ptr<sync_pb::UserEventSpecifics> specifics) override;
  void RecordUserEvent(const sync_pb::UserEventSpecifics& specifics) override;
  void RegisterDependentFieldTrial(
      sync_pb::UserEventSpecifics::EventCase event_case,
      const std::string& trial_name) override;
  base::WeakPtr<ModelTypeSyncBridge> GetSyncBridge() override;

  // base::FieldTrialList::Observer implementation.
  void OnFieldTrialGroupFinalized(const std::string& trial_name,
                                  const std::string& group_name) override;

  // Checks known (and immutable) conditions that should not change at runtime.
  static bool MightRecordEvents(bool off_the_record, SyncService* sync_service);

 private:
  // Whether allowed to record events that link to navigation data.
  bool CanRecordHistory();

  // Checks dynamic or event specific conditions.
  bool ShouldRecordEvent(const sync_pb::UserEventSpecifics& specifics);

  // Checks if conditions for the given |event_case| and |trial_name| pair have
  // changed such that |trial_name| should now be added to |recorded_events_|.
  // This method will modify |recorded_events_| and return whether
  // RecordFieldTrials() should be called. If the caller is performing multiple
  // invocations of MaybeUpdateTrialsToRecord() they should try to batch a
  // single RecordFieldTrials() call.
  bool MaybeUpdateTrialsToRecord(
      sync_pb::UserEventSpecifics::EventCase event_case,
      const std::string& trial_name) WARN_UNUSED_RESULT;

  // Construct and record a field trial event.
  void RecordFieldTrials();

  SyncService* sync_service_;

  std::unique_ptr<UserEventSyncBridge> bridge_;

  // Holds onto a random number for the duration of this execution of chrome. On
  // restart it will be regenerated. This can be attached to events to know
  // which events came from the same session.
  uint64_t session_id_;

  // Holds the relationships established through RegisterDependentFieldTrial().
  std::set<std::pair<sync_pb::UserEventSpecifics::EventCase, std::string>>
      event_trial_deps_;

  // The trial names that should be included when we record a FieldTrial event.
  std::set<std::string> trials_to_record_;

  // All of the event types that have been asked to be record that have been
  // sent through to actually be recorded. It's possible that we commit events
  // that were previously stored to disk, but we do not attempt to count them
  // here, they're from a different session and will have a different session
  // id.
  std::set<sync_pb::UserEventSpecifics::EventCase> recorded_events_;

  // Timer used to record a field trial event every given interval.
  base::OneShotTimer field_trial_timer_;

  DISALLOW_COPY_AND_ASSIGN(UserEventServiceImpl);
};

}  // namespace syncer

#endif  // COMPONENTS_SYNC_USER_EVENTS_USER_EVENT_SERVICE_IMPL_H_
