// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/feature_engagement_tracker/internal/configuration.h"

#include <string>

namespace feature_engagement_tracker {

Comparator::Comparator() : type(ANY), value(0) {}

Comparator::~Comparator() = default;

PreconditionConfig::PreconditionConfig() : window(0), storage(0) {}

PreconditionConfig::~PreconditionConfig() = default;

FeatureConfig::FeatureConfig() : valid(false) {}

FeatureConfig::FeatureConfig(const FeatureConfig& other) = default;

FeatureConfig::~FeatureConfig() = default;

bool operator==(const Comparator& lhs, const Comparator& rhs) {
  return lhs.type == rhs.type && lhs.value == rhs.value;
}

bool operator!=(const Comparator& lhs, const Comparator& rhs) {
  return !(lhs == rhs);
}

bool operator<(const Comparator& lhs, const Comparator& rhs) {
  return lhs.type < rhs.type || lhs.value < rhs.value;
}

bool operator==(const PreconditionConfig& lhs, const PreconditionConfig& rhs) {
  return lhs.event_name == rhs.event_name && lhs.comparator == rhs.comparator &&
         lhs.window == rhs.window && lhs.storage == rhs.storage;
}

bool operator!=(const PreconditionConfig& lhs, const PreconditionConfig& rhs) {
  return !(lhs == rhs);
}

bool operator<(const PreconditionConfig& lhs, const PreconditionConfig& rhs) {
  return lhs.event_name < rhs.event_name || lhs.comparator < rhs.comparator ||
         lhs.window < rhs.window || lhs.storage < rhs.storage;
}

bool operator==(const FeatureConfig& lhs, const FeatureConfig& rhs) {
  return lhs.valid == rhs.valid && lhs.used == rhs.used &&
         lhs.trigger == rhs.trigger && lhs.preconditions == rhs.preconditions &&
         lhs.session_rate == rhs.session_rate &&
         lhs.availability == rhs.availability;
}

bool operator<(const FeatureConfig& lhs, const FeatureConfig& rhs) {
  return lhs.valid < rhs.valid || lhs.used < rhs.used ||
         lhs.trigger < rhs.trigger || lhs.preconditions < rhs.preconditions ||
         lhs.session_rate < rhs.session_rate ||
         lhs.availability < rhs.availability;
}

}  // namespace feature_engagement_tracker
