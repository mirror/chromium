// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/version_ui/version_handler_helper.h"

#include <utility>
#include <vector>

#include "base/base_switches.h"
#include "base/metrics/field_trial.h"
#include "base/strings/string_piece.h"
#include "base/strings/string_util.h"
#include "base/values.h"
#include "components/variations/active_field_trials.h"

namespace version_ui {

namespace {

std::string GenerateCmdParam(const std::string& param_key,
                             const std::string& param_value) {
  if (!param_value.empty()) {
    return " --" + param_key + "=\"" + param_value + "\"";
  }
  return "";
}

}  // namespace

std::unique_ptr<base::Value> GetVariationsList() {
  std::vector<std::string> variations;
#if !defined(NDEBUG)
  base::FieldTrial::ActiveGroups active_groups;
  base::FieldTrialList::GetActiveFieldTrialGroups(&active_groups);

  const unsigned char kNonBreakingHyphenUTF8[] = {0xE2, 0x80, 0x91, '\0'};
  const std::string kNonBreakingHyphenUTF8String(
      reinterpret_cast<const char*>(kNonBreakingHyphenUTF8));
  for (const auto& group : active_groups) {
    std::string line = group.trial_name + ":" + group.group_name;
    base::ReplaceChars(line, "-", kNonBreakingHyphenUTF8String, &line);
    variations.push_back(line);
  }
#else
  // In release mode, display the hashes only.
  variations::GetFieldTrialActiveGroupIdsAsStrings(base::StringPiece(),
                                                   &variations);
#endif

  std::unique_ptr<base::ListValue> variations_list(new base::ListValue);
  for (std::vector<std::string>::const_iterator it = variations.begin();
       it != variations.end(); ++it) {
    variations_list->AppendString(*it);
  }

  return std::move(variations_list);
}

base::Value GetVariationsCmd() {
  std::string field_trial_states;
  base::FieldTrialList::AllStatesToString(&field_trial_states, true);

  std::string enable_features;
  std::string disable_features;
  base::FeatureList::GetInstance()->GetFeatureOverrides(&enable_features,
                                                        &disable_features);

  std::string variations_cmd_data =
      GenerateCmdParam(switches::kForceFieldTrials, field_trial_states) +
      GenerateCmdParam(switches::kEnableFeatures, enable_features) +
      GenerateCmdParam(switches::kDisableFeatures, disable_features);

  base::Value variations_cmd(variations_cmd_data);
  return variations_cmd;
}

}  // namespace version_ui
