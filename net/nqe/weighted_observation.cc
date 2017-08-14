// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "net/nqe/weighted_observation.h"

#include <vector>

namespace net {

namespace nqe {

namespace internal {

WeightedObservation::WeightedObservation(int32_t value, double weight)
    : value(value), weight(weight) {}

WeightedObservation::WeightedObservation(const WeightedObservation& other)
    : WeightedObservation(other.value, other.weight, other.subnet_id) {}

WeightedObservation& WeightedObservation::operator=(
    const WeightedObservation& other) {
  value = other.value;
  weight = other.weight;
  return *this;
}

// Required for sorting the samples in the ascending order of values.
bool WeightedObservation::operator<(const WeightedObservation& other) const {
  return (value < other.value);
}

}  // namespace internal

}  // namespace nqe

}  // namespace net
