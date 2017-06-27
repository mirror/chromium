// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_VARIATIONS_FIELD_TRIAL_CONFIG_FIELD_TRIAL_UTIL_H_
#define COMPONENTS_VARIATIONS_FIELD_TRIAL_CONFIG_FIELD_TRIAL_UTIL_H_

#include <string>
#include "base/metrics/field_trial.h"
#include "components/variations/service/variations_service.h"

namespace base {
class FeatureList;
}

namespace variations {

struct FieldTrialTestingConfig;

// Provides a mechanism to associate multiple set of params to multiple groups
// with a formatted string specified from commandline. See
// kForceFieldTrialParams in components/variations/variations_switches.cc for
// more details on the formatting.
bool AssociateParamsFromString(const std::string& variations_string);

// Provides a mechanism to associate multiple set of params and features to
// multiple groups with the |config| struct. This will also force the selection
// of FieldTrial groups specified in the |config|. Registers features associated
// with default field trials with |feature_list|.
void AssociateParamsFromFieldTrialConfig(const FieldTrialTestingConfig& config,
                                         base::FeatureList* feature_list);

// Associates params and features to FieldTrial groups and forces the selection
// of groups specified in testing/variations/fieldtrial_testing_config.json.
// Registers features associated with default field trials with |feature_list|.
void AssociateDefaultFieldTrialConfig(base::FeatureList* feature_list);

}  // namespace variations

bool SetupFieldTrialsCommon(
    std::unique_ptr<base::FieldTrialList>& field_trial_list,
    std::unique_ptr<base::FeatureList>* feature_list,
    std::vector<std::string>& variation_ids,
    variations::VariationsSeed& seed,
    std::unique_ptr<const base::FieldTrial::EntropyProvider>*
        low_entropy_provider,
    PrefService* local_state,
    variations::UIStringOverrider* ui_string_overrider,
    std::unique_ptr<variations::ClientFilterableState>* client_state,
    variations::VariationsService* variations_service);

bool SetupFieldTrialsCommon(
    std::unique_ptr<base::FieldTrialList>& field_trial_list,
    std::unique_ptr<base::FeatureList>* feature_list,
    std::vector<std::string>& variation_ids,
    variations::VariationsService* variations_service);

#endif  // COMPONENTS_VARIATIONS_FIELD_TRIAL_CONFIG_FIELD_TRIAL_UTIL_H_
