// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "platform/loader/field_trial/LoaderTrial.h"

#include "base/metrics/field_trial_params.h"
#include "base/strings/string_number_conversions.h"

namespace blink {

LoaderTrial::LoaderTrial(const char* trial) {
  bool result = base::GetFieldTrialParams(trial, &trial_params_);
  DCHECK(result);
}

uint32_t LoaderTrial::GetUint32Param(const char* name, uint32_t default_param) {
  const auto& found = trial_params_.find(name);
  if (found == trial_params_.end())
    return default_param;

  uint32_t param;
  if (!base::StringToUint(found->second, &param))
    return default_param;

  return param;
}

}  // namespace blink
