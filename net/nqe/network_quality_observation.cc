// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "net/nqe/network_quality_observation.h"

namespace net {

namespace nqe {

namespace internal {

Observation::Observation(float value,
                         base::TimeTicks timestamp,
                         const base::Optional<int32_t>& signal_strength,
                         NetworkQualityObservationSource source)
    : value(value),
      timestamp(timestamp),
      signal_strength(signal_strength),
      source(source) {
  DCHECK(!timestamp.is_null());
}

Observation::Observation(base::TimeDelta value,
                         base::TimeTicks timestamp,
                         const base::Optional<int32_t>& signal_strength,
                         NetworkQualityObservationSource source)
    : value(value.InMillisecondsF()),
      timestamp(timestamp),
      signal_strength(signal_strength),
      source(source) {
  DCHECK(!timestamp.is_null());
}

Observation::Observation(const Observation& other)
    : value(other.value),
      timestamp(other.timestamp),
      signal_strength(other.signal_strength),
      source(other.source) {}

Observation::~Observation() {}

}  // namespace internal

}  // namespace nqe

}  // namespace net
