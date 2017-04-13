// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/feature_engagement_tracker/internal/chrome_variations_configuration.h"

#include <map>
#include <memory>
#include <string>
#include <vector>

#include "base/logging.h"
#include "base/memory/ptr_util.h"
#include "base/metrics/field_trial_params.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/string_piece.h"
#include "base/strings/string_split.h"
#include "base/strings/string_util.h"
#include "components/feature_engagement_tracker/internal/configuration.h"
#include "components/feature_engagement_tracker/internal/feature_list.h"

namespace {

const char* kComparatorTypeAny = "any";
const char* kComparatorTypeLessThan = "<";
const char* kComparatorTypeGreaterThan = ">";
const char* kComparatorTypeLessThanOrEqual = "<=";
const char* kComparatorTypeGreaterThanOrEqual = ">=";
const char* kComparatorTypeEqual = "==";
const char* kComparatorTypeNotEqual = "!=";

const char* kPreconditionUsedKey = "precondition_used";
const char* kPreconditionTriggerKey = "precondition_trigger";
const char* kPreconditionKeyPrefix = "precondition_";
const char* kSessionRateKey = "session_rate";
const char* kAvailabilityKey = "availability";

const char* kPreconditionDataEventKey = "event";
const char* kPreconditionDataComparatorKey = "comparator";
const char* kPreconditionDataWindowKey = "window";
const char* kPreconditionDataStorageKey = "storage";

}  // namespace

namespace feature_engagement_tracker {

bool ParseComparatorSubstring(const std::string& definition,
                              Comparator* comparator,
                              ComparatorType type,
                              uint32_t type_len) {
  std::string number_string = definition.substr(type_len, std::string::npos);
  base::TrimString(number_string, " \t", &number_string);
  uint32_t value;
  if (!base::StringToUint(number_string, &value)) {
    return false;
  }

  comparator->type = type;
  comparator->value = value;
  return true;
}

bool ParseComparator(const std::string& definition, Comparator* comparator) {
  if (base::LowerCaseEqualsASCII(definition, kComparatorTypeAny)) {
    comparator->type = ANY;
    comparator->value = 0;
    return true;
  }

  if (base::StartsWith(definition, kComparatorTypeLessThanOrEqual,
                       base::CompareCase::INSENSITIVE_ASCII)) {
    return ParseComparatorSubstring(definition, comparator, LESS_THAN_OR_EQUAL,
                                    2);
  }

  if (base::StartsWith(definition, kComparatorTypeGreaterThanOrEqual,
                       base::CompareCase::INSENSITIVE_ASCII)) {
    return ParseComparatorSubstring(definition, comparator,
                                    GREATER_THAN_OR_EQUAL, 2);
  }

  if (base::StartsWith(definition, kComparatorTypeEqual,
                       base::CompareCase::INSENSITIVE_ASCII)) {
    return ParseComparatorSubstring(definition, comparator, EQUAL, 2);
  }

  if (base::StartsWith(definition, kComparatorTypeNotEqual,
                       base::CompareCase::INSENSITIVE_ASCII)) {
    return ParseComparatorSubstring(definition, comparator, NOT_EQUAL, 2);
  }

  if (base::StartsWith(definition, kComparatorTypeLessThan,
                       base::CompareCase::INSENSITIVE_ASCII)) {
    return ParseComparatorSubstring(definition, comparator, LESS_THAN, 1);
  }

  if (base::StartsWith(definition, kComparatorTypeGreaterThan,
                       base::CompareCase::INSENSITIVE_ASCII)) {
    return ParseComparatorSubstring(definition, comparator, GREATER_THAN, 1);
  }

  return false;
}

bool ParsePreconditionConfig(const std::string& definition,
                             PreconditionConfig* precondition_config) {
  // Support definitions with at least 4 tokens.
  std::vector<std::string> tokens = base::SplitString(
      definition, ";", base::TRIM_WHITESPACE, base::SPLIT_WANT_ALL);
  if (tokens.size() < 4) {
    return false;
  }

  // Parse tokens in any order.
  bool has_event_name = false;
  bool has_comparator = false;
  bool has_window = false;
  bool has_storage = false;
  for (const std::string& token : tokens) {
    std::vector<std::string> pair = base::SplitString(
        token, ":", base::TRIM_WHITESPACE, base::SPLIT_WANT_ALL);
    if (pair.size() != 2) {
      *precondition_config = PreconditionConfig();
      return false;
    }

    const std::string& key = pair[0];
    const std::string& value = pair[1];
    // TODO(nyquist): Ensure that key matches regex /^[a-zA-Z0-9-_]+$/.

    if (base::LowerCaseEqualsASCII(key, kPreconditionDataEventKey)) {
      if (has_event_name)
        return false;
      has_event_name = true;

      precondition_config->event_name = value;
    } else if (base::LowerCaseEqualsASCII(key,
                                          kPreconditionDataComparatorKey)) {
      if (has_comparator)
        return false;
      has_comparator = true;

      Comparator comparator;
      if (!ParseComparator(value, &comparator)) {
        *precondition_config = PreconditionConfig();
        return false;
      }

      precondition_config->comparator = comparator;
    } else if (base::LowerCaseEqualsASCII(key, kPreconditionDataWindowKey)) {
      if (has_window)
        return false;
      has_window = true;

      uint32_t parsed_value;
      if (!base::StringToUint(value, &parsed_value)) {
        *precondition_config = PreconditionConfig();
        return false;
      }

      precondition_config->window = parsed_value;
    } else if (base::LowerCaseEqualsASCII(key, kPreconditionDataStorageKey)) {
      if (has_storage)
        return false;
      has_storage = true;

      uint32_t parsed_value;
      if (!base::StringToUint(value, &parsed_value)) {
        *precondition_config = PreconditionConfig();
        return false;
      }

      precondition_config->storage = parsed_value;
    }
  }

  return has_event_name && has_comparator && has_window && has_storage;
}

ChromeVariationsConfiguration::ChromeVariationsConfiguration() = default;

ChromeVariationsConfiguration::~ChromeVariationsConfiguration() = default;

void ChromeVariationsConfiguration::ParseFeatureConfigs(
    FeatureVector features) {
  for (auto* feature : features) {
    ParseFeatureConfig(feature);
  }
}

void ChromeVariationsConfiguration::ParseFeatureConfig(
    const base::Feature* feature) {
  DCHECK(feature);
  DCHECK(configs_.find(feature) == configs_.end());

  // Initially all new configurations are considered invalid.
  FeatureConfig& config = configs_[feature];
  config.valid = false;
  uint32_t parse_errors = 0;

  std::map<std::string, std::string> params;
  bool result = base::GetFieldTrialParamsByFeature(*feature, &params);
  if (!result)
    return;

  for (auto it = params.begin(); it != params.end(); ++it) {
    const std::string& key = it->first;
    if (key == kPreconditionUsedKey) {
      PreconditionConfig precondition_config;
      if (!ParsePreconditionConfig(params[key], &precondition_config)) {
        ++parse_errors;
        continue;
      }
      config.used = precondition_config;
    } else if (key == kPreconditionTriggerKey) {
      PreconditionConfig precondition_config;
      if (!ParsePreconditionConfig(params[key], &precondition_config)) {
        ++parse_errors;
        continue;
      }
      config.trigger = precondition_config;
    } else if (key == kSessionRateKey) {
      Comparator comparator;
      if (!ParseComparator(params[key], &comparator)) {
        ++parse_errors;
        continue;
      }
      config.session_rate = comparator;
    } else if (key == kAvailabilityKey) {
      Comparator comparator;
      if (!ParseComparator(params[key], &comparator)) {
        ++parse_errors;
        continue;
      }
      config.availability = comparator;
    } else if (base::StartsWith(key, kPreconditionKeyPrefix,
                                base::CompareCase::INSENSITIVE_ASCII)) {
      PreconditionConfig precondition_config;
      if (!ParsePreconditionConfig(params[key], &precondition_config)) {
        ++parse_errors;
        continue;
      }
      config.preconditions.insert(precondition_config);
    } else {
      DVLOG(1) << "Ignoring unknown key when parsing config for feature "
               << feature->name << ": " << key;
    }
  }

  // The |used| and |triggger| members are required, so should not be the
  // default values.
  config.valid = config.used != PreconditionConfig() &&
                 config.trigger != PreconditionConfig() && parse_errors == 0;
}

const FeatureConfig& ChromeVariationsConfiguration::GetFeatureConfig(
    const base::Feature& feature) const {
  auto it = configs_.find(&feature);
  DCHECK(it != configs_.end());
  return it->second;
}

}  // namespace feature_engagement_tracker
