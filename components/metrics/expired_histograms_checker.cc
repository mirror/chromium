// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/metrics/expired_histograms_checker.h"

#include "base/feature_list.h"
#include "base/metrics/field_trial_params.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/string_tokenizer.h"

#include <algorithm>
#include <string>

namespace metrics {

namespace {

const base::Feature kConfigureExpiredHistogramsFeature{
    "ConfigureExpiredHistograms", base::FEATURE_DISABLED_BY_DEFAULT};

const std::string kParameterDelimeter = ",";

}  // namespace

ExpiredHistogramsChecker::ExpiredHistogramsChecker(const uint64_t* array,
                                                   size_t size)
    : array_(array),
      size_(size),
      enabled_histograms_(),
      disabled_histograms_(),
      variation_parameters_loaded_(false) {}

ExpiredHistogramsChecker::~ExpiredHistogramsChecker() {}

bool ExpiredHistogramsChecker::ShouldRecord(uint64_t histogram_hash) {
  LoadVariationParameters();

  bool is_expired = std::binary_search(array_, array_ + size_, histogram_hash);
  bool is_enabled = std::binary_search(
      enabled_histograms_.begin(), enabled_histograms_.end(), histogram_hash);
  bool is_disabled = std::binary_search(
      disabled_histograms_.begin(), disabled_histograms_.end(), histogram_hash);

  return (is_enabled || (!is_expired && !is_disabled));
}

void ExpiredHistogramsChecker::LoadVariationParameters() {
  if (variation_parameters_loaded_)
    return;
  if (base::FeatureList::GetInstance() == nullptr)
    return;

  std::map<std::string, std::string> params;
  if (base::GetFieldTrialParamsByFeature(kConfigureExpiredHistogramsFeature,
                                         &params)) {
    std::string enabled_histograms_parameter = params["enabled_histograms"];
    std::string disabled_histograms_parameter = params["disabled_histograms"];

    enabled_histograms_ = ParseVariationParameter(enabled_histograms_parameter);
    disabled_histograms_ =
        ParseVariationParameter(disabled_histograms_parameter);
  }

  variation_parameters_loaded_ = true;
}

std::vector<uint64_t> ExpiredHistogramsChecker::ParseVariationParameter(
    const std::string& parameter) const {
  std::vector<uint64_t> result;
  base::StringTokenizer tokenizer(parameter, kParameterDelimeter);
  while (tokenizer.GetNext()) {
    uint64_t histogram_hash;
    base::HexStringToUInt64(tokenizer.token(), &histogram_hash);
    result.push_back(histogram_hash);
  }
  return result;
}

}  // namespace metrics
