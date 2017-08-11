// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef NET_NQE_WEIGHTED_OBSERVATION_H_
#define NET_NQE_WEIGHTED_OBSERVATION_H_

#include <vector>

#include "base/optional.h"
#include "net/base/net_export.h"

namespace net {

namespace nqe {

namespace internal {

// Holds an observation and its weight.
struct NET_EXPORT_PRIVATE WeightedObservation {
  WeightedObservation(int32_t value,
                      double weight,
                      const base::Optional<uint64_t>& subnet_id);

  WeightedObservation(const WeightedObservation& other);

  WeightedObservation& operator=(const WeightedObservation& other);

  // Required for sorting the samples in the ascending order of values.
  bool operator<(const WeightedObservation& other) const;

  // Value of the sample.
  int32_t value;

  // Weight of the sample. This is computed based on how much time has passed
  // since the sample was taken.
  double weight;

  // The subnet identifier from which the measurement was taken.
  base::Optional<uint64_t> subnet_id;
};

}  // namespace internal

}  // namespace nqe

}  // namespace net

#endif  // NET_NQE_WEIGHTED_OBSERVATION_H_
