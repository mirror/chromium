// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_SYNC_USER_EVENTS_TRIAL_RECORDER_H_
#define COMPONENTS_SYNC_USER_EVENTS_TRIAL_RECORDER_H_

#include <set>
#include <string>
#include <utility>

#include "base/compiler_specific.h"
#include "base/macros.h"
#include "base/metrics/field_trial.h"
#include "base/timer/timer.h"
#include "components/sync/protocol/user_event_specifics.pb.h"
#include "components/sync/user_events/user_event_service.h"

namespace syncer {

class TrialRecorder : public base::FieldTrialList::Observer {
 public:
  explicit TrialRecorder(UserEventService* user_event_service);
  ~TrialRecorder() override;

  // base::FieldTrialList::Observer implementation.
  void OnFieldTrialGroupFinalized(const std::string& trial_name,
                                  const std::string& group_name) override;

  // Should be called every time a dependency is registered.
  void RegisterDependentFieldTrial(
      sync_pb::UserEventSpecifics::EventCase event_case,
      const std::string& trial_name);

  // Should be called every time an event is recorded.
  void OnEventCaseRecorded(sync_pb::UserEventSpecifics::EventCase event_case);

 private:
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

  // Non-owning pointer to interface of how events are actually recorded.
  UserEventService* user_event_service_;

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

  DISALLOW_COPY_AND_ASSIGN(TrialRecorder);
};

}  // namespace syncer

#endif  // COMPONENTS_SYNC_USER_EVENTS_TRIAL_RECORDER_H_
