// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/feature_engagement_tracker/internal/stats.h"

#include <string>

#include "base/metrics/histogram_functions.h"
#include "base/metrics/histogram_macros.h"
#include "base/metrics/user_metrics.h"
#include "components/feature_engagement_tracker/public/feature_list.h"

namespace feature_engagement_tracker {
namespace stats {
namespace {

// Helper function to log a TriggerHelpUIResult.
void LogTriggerHelpUIResult(const std::string& name,
                            TriggerHelpUIResult result) {
  // Must not use histograms macros here because we pass in the histogram name.
  base::UmaHistogramEnumeration(name, result, TriggerHelpUIResult::COUNT);
}

}  // namespace

void RecordNotifyEvent(const std::string& event_name,
                       const Configuration* config) {
  DCHECK(!event_name.empty());
  DCHECK(config);

  // Find which feature this event belongs to.
  const Configuration::ConfigMap& features = config->GetRegisteredFeatures();
  std::string feature_name;
  for (const auto& element : features) {
    const base::Feature* feature = element.first;
    const FeatureConfig& feature_config = element.second;

    // Track used event separately.
    if (feature_config.used.name == event_name) {
      feature_name = feature->name;
      DCHECK(!feature_name.empty());
      std::string used_event_action = "FeatureEngagement.NotifyUsedEvent.";
      used_event_action.append(feature_name);
      base::RecordComputedAction(used_event_action);
      break;
    }

    // Find if the |event_name| matches any configuration.
    for (auto& event : feature_config.event_configs) {
      if (event.name == event_name) {
        feature_name = feature->name;
        break;
      }
    }
    if (feature_config.trigger.name == event_name) {
      feature_name = feature->name;
      break;
    }
  }

  // Do nothing if no events in the configuration matches the |event_name|.
  if (feature_name.empty())
    return;

  std::string event_action = "FeatureEngagement.NotifyEvent.";
  event_action.append(feature_name);
  base::RecordComputedAction(event_action);
}

void RecordShouldTriggerHelpUI(const base::Feature& feature,
                               const ConditionValidator::Result& result) {
  // Records the user action.
  std::string name = "FeatureEngagement.ShouldTriggerHelpUI.";
  name.append(feature.name);
  base::RecordComputedAction(name);

  // Total count histogram, used to compute the percentage of each failure type.
  if (result.NoErrors()) {
    LogTriggerHelpUIResult(name, TriggerHelpUIResult::SUCCESS);
  } else {
    LogTriggerHelpUIResult(name, TriggerHelpUIResult::FAILURE);
  }

  // Histogram about the failure reasons.
  if (!result.model_ready_ok)
    LogTriggerHelpUIResult(name, TriggerHelpUIResult::FAILURE_MODEL_NOT_READY);
  if (!result.currently_showing_ok) {
    LogTriggerHelpUIResult(name,
                           TriggerHelpUIResult::FAILURE_CURRENTLY_SHOWING);
  }
  if (!result.feature_enabled_ok) {
    LogTriggerHelpUIResult(name, TriggerHelpUIResult::FAILURE_FEATURE_DISABLED);
  }
  if (!result.config_ok) {
    LogTriggerHelpUIResult(name, TriggerHelpUIResult::FAILURE_CONFIG_INVALID);
  }
  if (!result.used_ok) {
    LogTriggerHelpUIResult(
        name, TriggerHelpUIResult::FAILURE_USED_PRECONDITION_UNMEET);
  }
  if (!result.trigger_ok) {
    LogTriggerHelpUIResult(
        name, TriggerHelpUIResult::FAILURE_TRIGGER_PRECONDITION_UNMEET);
  }
  if (!result.preconditions_ok) {
    LogTriggerHelpUIResult(
        name, TriggerHelpUIResult::FAILURE_OTHER_PRECONDITION_UNMEET);
  }
  if (!result.session_rate_ok) {
    LogTriggerHelpUIResult(name, TriggerHelpUIResult::FAILURE_SESSION_RATE);
  }
}

void RecordUserDismiss() {
  base::RecordAction(base::UserMetricsAction("FeatureEngagement.Dismissed"));
}

void RecordDbUpdate(bool success) {
  UMA_HISTOGRAM_BOOLEAN("FeatureEngagement.Db.Update", success);
}

void RecordDbInitEvent(bool success) {
  UMA_HISTOGRAM_BOOLEAN("FeatureEngagement.Db.Init", success);
}

void RecordDbLoadEvent(bool success, const std::vector<Event>& events) {
  UMA_HISTOGRAM_BOOLEAN("FeatureEngagement.Db.Load", success);
  if (!success)
    return;

  // Tracks total number of events records when the database is successfully
  // loaded.
  int event_count = 0;
  for (auto& event : events)
    event_count += event.events_size();
  UMA_HISTOGRAM_COUNTS_1000("FeatureEngagement.Db.TotalEvents", event_count);
}

void RecordConfigParsingEvent(ConfigParsingEvent event) {
  UMA_HISTOGRAM_ENUMERATION("FeatureEngagement.Config.ParsingEvent", event,
                            ConfigParsingEvent::COUNT);
}

}  // namespace stats
}  // namespace feature_engagement_tracker
