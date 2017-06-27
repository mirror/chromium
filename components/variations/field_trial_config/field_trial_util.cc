// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/variations/field_trial_config/field_trial_util.h"

#include <stddef.h>

#include <string>
#include <vector>

#include "base/base_switches.h"
#include "base/command_line.h"
#include "base/feature_list.h"
#include "base/metrics/field_trial.h"
#include "base/strings/string_split.h"
#include "base/strings/stringprintf.h"
#include "cc/base/switches.h"
#include "components/variations/field_trial_config/fieldtrial_testing_config.h"
#include "components/variations/service/variations_service.h"
#include "components/variations/variations_associated_data.h"
#include "components/variations/variations_http_header_provider.h"
#include "components/variations/variations_switches.h"
#include "content/public/common/content_switches.h"
#include "net/base/escape.h"

namespace variations {
namespace {

std::string EscapeValue(const std::string& value) {
  return net::UnescapeURLComponent(
      value, net::UnescapeRule::PATH_SEPARATORS |
                 net::UnescapeRule::URL_SPECIAL_CHARS_EXCEPT_PATH_SEPARATORS);
}

void AssociateParamsFromExperiment(
    const std::string& study_name,
    const FieldTrialTestingExperiment& experiment,
    base::FeatureList* feature_list) {
  if (experiment.params_size != 0) {
    std::map<std::string, std::string> params;
    for (size_t i = 0; i < experiment.params_size; ++i) {
      const FieldTrialTestingExperimentParams& param = experiment.params[i];
      params[param.key] = param.value;
    }
    AssociateVariationParams(study_name, experiment.name, params);
  }
  base::FieldTrial* trial =
      base::FieldTrialList::CreateFieldTrial(study_name, experiment.name);

  if (!trial) {
    DLOG(WARNING) << "Field trial config study skipped: " << study_name
                  << "." << experiment.name
                  << " (it is overridden from chrome://flags)";
    return;
  }

  for (size_t i = 0; i < experiment.enable_features_size; ++i) {
    feature_list->RegisterFieldTrialOverride(
        experiment.enable_features[i],
        base::FeatureList::OVERRIDE_ENABLE_FEATURE, trial);
  }
  for (size_t i = 0; i < experiment.disable_features_size; ++i) {
    feature_list->RegisterFieldTrialOverride(
        experiment.disable_features[i],
        base::FeatureList::OVERRIDE_DISABLE_FEATURE, trial);
  }
}

// Chooses an experiment taking into account the command line. Defaults to
// picking the first experiment.
const FieldTrialTestingExperiment& ChooseExperiment(
    const FieldTrialTestingExperiment experiments[],
    size_t experiment_size) {
  DCHECK_GT(experiment_size, 0ul);
  const auto& command_line = *base::CommandLine::ForCurrentProcess();
  size_t chosen_index = 0;
  for (size_t i = 1; i < experiment_size && chosen_index == 0; ++i) {
    const auto& experiment = experiments[i];
    if (experiment.forcing_flag &&
        command_line.HasSwitch(experiment.forcing_flag)) {
      chosen_index = i;
      break;
    }
  }
  DCHECK_GT(experiment_size, chosen_index);
  return experiments[chosen_index];
}

} // namespace

bool AssociateParamsFromString(const std::string& varations_string) {
  // Format: Trial1.Group1:k1/v1/k2/v2,Trial2.Group2:k1/v1/k2/v2
  std::set<std::pair<std::string, std::string>> trial_groups;
  for (const base::StringPiece& experiment_group : base::SplitStringPiece(
           varations_string, ",",
           base::TRIM_WHITESPACE, base::SPLIT_WANT_ALL)) {
    std::vector<base::StringPiece> experiment = base::SplitStringPiece(
        experiment_group, ":", base::TRIM_WHITESPACE, base::SPLIT_WANT_ALL);
    if (experiment.size() != 2) {
      DLOG(ERROR) << "Experiment and params should be separated by ':'";
      return false;
    }

    std::vector<std::string> group_parts = base::SplitString(
        experiment[0], ".", base::TRIM_WHITESPACE, base::SPLIT_WANT_ALL);
    if (group_parts.size() != 2) {
      DLOG(ERROR) << "Trial and group name should be separated by '.'";
      return false;
    }

    std::vector<std::string> key_values = base::SplitString(
        experiment[1], "/", base::TRIM_WHITESPACE, base::SPLIT_WANT_ALL);
    if (key_values.size() % 2 != 0) {
      DLOG(ERROR) << "Param name and param value should be separated by '/'";
      return false;
    }
    std::string trial = EscapeValue(group_parts[0]);
    std::string group = EscapeValue(group_parts[1]);
    auto trial_group = std::make_pair(trial, group);
    if (trial_groups.find(trial_group) != trial_groups.end()) {
      DLOG(ERROR) << base::StringPrintf(
          "A (trial, group) pair listed more than once. (%s, %s)",
          trial.c_str(), group.c_str());
      return false;
    }
    trial_groups.insert(trial_group);
    std::map<std::string, std::string> params;
    for (size_t i = 0; i < key_values.size(); i += 2) {
      std::string key = EscapeValue(key_values[i]);
      std::string value = EscapeValue(key_values[i + 1]);
      params[key] = value;
    }
    AssociateVariationParams(trial, group, params);
  }
  return true;
}

void AssociateParamsFromFieldTrialConfig(const FieldTrialTestingConfig& config,
                                         base::FeatureList* feature_list) {
  for (size_t i = 0; i < config.studies_size; ++i) {
    const FieldTrialTestingStudy& study = config.studies[i];
    if (study.experiments_size > 0) {
      AssociateParamsFromExperiment(
          study.name,
          ChooseExperiment(study.experiments, study.experiments_size),
          feature_list);
    } else {
      DLOG(ERROR) << "Unexpected empty study: " << study.name;
    }
  }
}

void AssociateDefaultFieldTrialConfig(base::FeatureList* feature_list) {
  AssociateParamsFromFieldTrialConfig(kFieldTrialConfig, feature_list);
}

}  // namespace variations

bool SetupFieldTrialsCommon(
    std::unique_ptr<base::FieldTrialList>& field_trial_list,
    std::unique_ptr<base::FeatureList>& feature_list,
    std::vector<std::string>& variation_ids,
    variations::VariationsSeed& seed,
    std::unique_ptr<const base::FieldTrial::EntropyProvider>*
        low_entropy_provider,
    PrefService* local_state,
    variations::UIStringOverrider* ui_string_overrider,
    std::unique_ptr<variations::ClientFilterableState>& client_state,
    variations::VariationsService* variations_service) {
  const base::CommandLine* command_line =
      base::CommandLine::ForCurrentProcess();
  if (command_line->HasSwitch(variations::switches::kEnableBenchmarking) ||
      command_line->HasSwitch(cc::switches::kEnableGpuBenchmarking)) {
    base::FieldTrial::EnableBenchmarking();
  }

  if (command_line->HasSwitch(variations::switches::kForceFieldTrialParams)) {
    bool result =
        variations::AssociateParamsFromString(command_line->GetSwitchValueASCII(
            variations::switches::kForceFieldTrialParams));
    CHECK(result) << "Invalid --"
                  << variations::switches::kForceFieldTrialParams
                  << " list specified.";
  }

  // Ensure any field trials specified on the command line are initialized.
  if (command_line->HasSwitch(switches::kForceFieldTrials)) {
    std::set<std::string> unforceable_field_trials;
#if defined(OFFICIAL_BUILD)
    unforceable_field_trials.insert("SettingsEnforcement");
#endif  // defined(OFFICIAL_BUILD)

    // Create field trials without activating them, so that this behaves in
    // a consistent manner with field trials created from the server.
    bool result = base::FieldTrialList::CreateTrialsFromString(
        command_line->GetSwitchValueASCII(switches::kForceFieldTrials),
        unforceable_field_trials);
    CHECK(result) << "Invalid --" << switches::kForceFieldTrials
                  << " list specified.";
  }

  variations::VariationsHttpHeaderProvider* http_header_provider =
      variations::VariationsHttpHeaderProvider::GetInstance();
  bool result = http_header_provider->ForceVariationIds(
      command_line->GetSwitchValueASCII(
          variations::switches::kForceVariationIds),
      &variation_ids);
  CHECK(result) << "Invalid list of variation ids specified (either in --"
                << variations::switches::kForceVariationIds
                << " or in chrome://flags)";

  feature_list->InitializeFromCommandLine(
      command_line->GetSwitchValueASCII(switches::kEnableFeatures),
      command_line->GetSwitchValueASCII(switches::kDisableFeatures));

#if defined(FIELDTRIAL_TESTING_ENABLED)
  if (!command_line->HasSwitch(
          variations::switches::kDisableFieldTrialTestingConfig) &&
      !command_line->HasSwitch(switches::kForceFieldTrials) &&
      !command_line->HasSwitch(variations::switches::kVariationsServerURL)) {
    variations::AssociateDefaultFieldTrialConfig(feature_list.get());
  }
#endif  // defined(FIELDTRIAL_TESTING_ENABLED)

  bool has_seed =
      (variations_service &&
       variations_service->CreateTrialsFromSeed(feature_list.get())) ||
      (!variations_service /*&& variations::CreateTrialsFromSeedCommon(&seed, &feature_list, low_entropy_provider,
                                                                    local_state, ui_string_overrider, &client_state)*/);
  base::FeatureList::SetInstance(std::move(feature_list));

  return has_seed;
}

bool SetupFieldTrialsCommon(
    std::unique_ptr<base::FieldTrialList>& field_trial_list,
    std::unique_ptr<base::FeatureList>* feature_list,
    std::vector<std::string>& variation_ids,
    variations::VariationsService* variations_service) {
  variations::VariationsSeed seed;
  // return SetupFieldTrialsCommon(field_trial_list, feature_list,
  // variation_ids, seed, nullptr, nullptr, nullptr, nullptr,
  // variations_service);
  return false;
}
