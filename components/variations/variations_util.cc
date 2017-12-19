// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/variations/variations_util.h"

#include <vector>

#include "base/strings/string_number_conversions.h"
#include "base/strings/string_piece.h"
#include "components/crash/core/common/crash_key.h"
#include "components/variations/active_field_trials.h"

namespace variations {

void SetVariationListCrashKeys() {
  std::vector<std::string> experiment_strings;
  GetFieldTrialActiveGroupIdsAsStrings(base::StringPiece(),
                                       &experiment_strings);
  GetSyntheticTrialGroupIdsAsString(&experiment_strings);

  static crash_reporter::CrashKeyString<8> num_variations_key(
      "num-experiments");
  num_variations_key.Set(base::NumberToString(experiment_strings.size()));

  static constexpr size_t kVariationsKeySize = 2048;
  static crash_reporter::CrashKeyString<kVariationsKeySize> crash_key(
      "variations");

  std::string variations_string;
  variations_string.reserve(kVariationsKeySize);

  for (const auto& variation : experiment_strings) {
    // Do not truncate an individual experiment.
    if (variations_string.size() + variation.size() >= kVariationsKeySize)
      break;
    variations_string += variation;
    variations_string += ",";
  }
  crash_key.Set(variations_string);
}

}  // namespace variations
