// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_FEATURE_ENGAGEMENT_TRACKER_INTERNAL_STATS_H_
#define COMPONENTS_FEATURE_ENGAGEMENT_TRACKER_INTERNAL_STATS_H_

#include <string>
#include <vector>

#include "components/feature_engagement_tracker/internal/condition_validator.h"
#include "components/feature_engagement_tracker/internal/configuration.h"
#include "components/feature_engagement_tracker/internal/proto/event.pb.h"

namespace feature_engagement_tracker {
namespace stats {

// Enum used in the metrics to record the result when in-product help UI is
// going to be triggered.
// Most of the fields maps to |ConditionValidator::Result|.
// The failure reasons are not mutually exclusive.
// Out-dated entries shouldn't be deleted but marked as obselete.
enum class TriggerHelpUIResult {
  // The help UI is triggered.
  SUCCESS = 0,

  // The help UI is not triggered.
  FAILURE = 1,

  // Data layer is not ready.
  FAILURE_MODEL_NOT_READY = 2,

  // Some other help UI is currently showing.
  FAILURE_CURRENTLY_SHOWING = 3,

  // The feature is disabled.
  FAILURE_FEATURE_DISABLED = 4,

  // Configuration can not be parsed.
  FAILURE_CONFIG_INVALID = 5,

  // Used event precondition is not satisfied.
  FAILURE_USED_PRECONDITION_UNMEET = 6,

  // Trigger event precondition is not satisfied.
  FAILURE_TRIGGER_PRECONDITION_UNMEET = 7,

  // Other event precondition is not satisfied.
  FAILURE_OTHER_PRECONDITION_UNMEET = 8,

  // Session rate does not meet the requirement.
  FAILURE_SESSION_RATE = 9,

  // Availability mode is not ready.
  FAILURE_AVAILABILITY_MODEL_NOT_READY = 10,

  // Availability precondition is not satisfied.
  FAILURE_AVAILABILITY_PRECONDITION_UNMEET = 11,

  // Last entry for the enum.
  COUNT = 12,
};

// Used in the metrics to track the configuration parsing event.
// The failure reasons are not mutually exclusive.
// Out-dated entries shouldn't be deleted but marked as obsolete.
enum class ConfigParsingEvent {
  // The configuration is parsed correctly.
  SUCCESS = 0,

  // The configuration is invalid after parsing.
  FAILURE = 1,

  // Fails to parse the feature config because no field trial is found.
  FAILURE_NO_FIELD_TRIAL = 2,

  // Fails to parse the used event.
  FAILURE_USED_EVENT_PARSE = 3,

  // Used event is missing.
  FAILURE_USED_EVENT_MISSING = 4,

  // Fails to parse the trigger event.
  FAILURE_TRIGGER_EVENT_PARSE = 5,

  // Trigger event is missing.
  FAILURE_TRIGGER_EVENT_MISSING = 6,

  // Fails to parse other events.
  FAILURE_OTHER_EVENT_PARSE = 7,

  // Fails to parse the session rate comparator.
  FAILURE_SESSION_RATE_PARSE = 8,

  // Fails to parse the availability comparator.
  FAILURE_AVAILABILITY_PARSE = 9,

  // UnKnown key in configuration parameters.
  FAILURE_UNKNOWN_KEY = 10,

  // Last entry for the enum.
  COUNT = 11,
};

// Records the feature engagement events. Used event will be tracked
// separately.
void RecordNotifyEvent(const std::string& event, const Configuration* config);

// Records user action and the result histogram when in-product help will be
// shown to the user.
void RecordShouldTriggerHelpUI(const base::Feature& feature,
                               const ConditionValidator::Result& result);

// Records when the user dismisses the in-product help UI.
void RecordUserDismiss();

// Records the result of level db updates.
void RecordDbUpdate(bool success);

// Record database init.
void RecordDbInitEvent(bool success);

// Records database load event.
void RecordDbLoadEvent(bool success, const std::vector<Event>& events);

// Records configuration parsing event.
void RecordConfigParsingEvent(ConfigParsingEvent event);

}  // namespace stats
}  // namespace feature_engagement_tracker

#endif  // COMPONENTS_FEATURE_ENGAGEMENT_TRACKER_INTERNAL_STATS_H_
