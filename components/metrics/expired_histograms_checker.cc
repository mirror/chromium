// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/metrics/expired_histograms_checker.h"
#include "base/metrics/field_trial_params.h"

#include <algorithm>
#include <sstream>
#include <string>

namespace metrics {

ExpiredHistogramsChecker::ExpiredHistogramsChecker(const uint64_t* array,
                                                   size_t size)
    : array_(array),
      size_(size),
      enabled_histograms_(),
      disabled_histograms_(),
      finch_parameters_loaded_(false) {}

ExpiredHistogramsChecker::~ExpiredHistogramsChecker() {}

bool ExpiredHistogramsChecker::ShouldRecord(uint64_t histogram_hash) {
  LoadFinchParameters();

  bool is_expired = std::binary_search(array_, array_ + size_, histogram_hash);
  bool is_enabled = std::binary_search(
      enabled_histograms_.begin(), enabled_histograms_.end(), histogram_hash);
  bool is_disabled = std::binary_search(
      disabled_histograms_.begin(), disabled_histograms_.end(), histogram_hash);

  return (is_enabled || (!is_expired && !is_disabled));
}

void ExpiredHistogramsChecker::LoadFinchParameters() {
  if (finch_parameters_loaded_)
    return;
  std::map<std::string, std::string> params;
  if (base::GetFieldTrialParams(finch_study_, &params)) {
    std::string enabled_histograms_parameter = params["enabled_histograms"];
    std::string disabled_histograms_parameter = params["disabled_histograms"];

    enabled_histograms_ = ParseFinchParameter(enabled_histograms_parameter);
    disabled_histograms_ = ParseFinchParameter(disabled_histograms_parameter);

    finch_parameters_loaded_ = true;
  }
}

std::vector<uint64_t> ExpiredHistogramsChecker::ParseFinchParameter(
    const std::string& parameter) const {
  std::vector<uint64_t> result;
  std::istringstream iss(parameter);
  std::string histogram_hash;
  while (std::getline(iss, histogram_hash, delimeter)) {
    result.push_back(std::stoll(histogram_hash, nullptr, 16));
  }
  return result;
}

}  // namespace metrics
