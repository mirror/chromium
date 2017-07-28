// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "services/resource_coordinator/public/cpp/resource_coordinator_features.h"

#include <string>

#include "base/metrics/field_trial_params.h"
#include "base/strings/string_number_conversions.h"

namespace {

int64_t GetIntegerFieldTrialParameter(base::Feature feature,
                                      const char* parameter_name,
                                      int64_t default_val) {
  std::string parameter_str =
      base::GetFieldTrialParamValueByFeature(feature, parameter_name);

  int64_t parameter_value;
  if (parameter_str.empty() ||
      !base::StringToInt64(parameter_str, &parameter_value)) {
    return default_val;
  }

  return parameter_value;
}

}  // namespace

namespace features {

// Globally enable the GRC.
const base::Feature kGlobalResourceCoordinator{
    "GlobalResourceCoordinator", base::FEATURE_ENABLED_BY_DEFAULT};

// Enable render process CPU profiling for GRC.
const base::Feature kGRCRenderProcessCPUProfiling{
    "GRCRenderProcessCPUProfiling", base::FEATURE_DISABLED_BY_DEFAULT};

const char* kGRCRenderProcessCPUProfilingIntervalInMs = "intervalInMs";
const char* kGRCRenderProcessCPUProfilingDurationInMs = "durationInMs";

}  // namespace features

namespace resource_coordinator {

bool IsResourceCoordinatorEnabled() {
  return base::FeatureList::IsEnabled(features::kGlobalResourceCoordinator);
}

bool IsGRCRenderProcessCPUProfilingEnabled() {
  return base::FeatureList::IsEnabled(features::kGRCRenderProcessCPUProfiling);
}

int64_t GetGRCRenderProcessCPUProfilingDurationInMs() {
  return GetIntegerFieldTrialParameter(
      features::kGRCRenderProcessCPUProfiling,
      features::kGRCRenderProcessCPUProfilingDurationInMs, -1);
}

int64_t GetGRCRenderProcessCPUProfilingIntervalInMs() {
  return GetIntegerFieldTrialParameter(
      features::kGRCRenderProcessCPUProfiling,
      features::kGRCRenderProcessCPUProfilingIntervalInMs, -1);
}

}  // namespace resource_coordinator
