// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef NET_NQE_NETWORK_QUALITY_OBSERVATION_H_
#define NET_NQE_NETWORK_QUALITY_OBSERVATION_H_

#include <stdint.h>

#include "base/optional.h"
#include "base/sequence_checker.h"
#include "base/time/time.h"
#include "net/base/net_export.h"
#include "net/nqe/network_quality_estimator_util.h"
#include "net/nqe/network_quality_observation_source.h"

namespace net {

namespace nqe {

namespace internal {

// Records observations of network quality metrics (such as round trip time
// or throughput), along with the time the observation was made. Observations
// can be made at several places in the network stack, thus the observation
// source is provided as well.
class NET_EXPORT_PRIVATE Observation {
 public:
  Observation(int32_t value,
              base::TimeTicks timestamp,
              const base::Optional<int32_t>& signal_strength,
              NetworkQualityObservationSource source);

  Observation(int32_t value,
              base::TimeTicks timestamp,
              const base::Optional<int32_t>& signal_strength,
              NetworkQualityObservationSource source,
              const base::Optional<IPHash>& host);

  Observation(const Observation& other);
  Observation& operator=(const Observation& other);

  ~Observation();

  // Value of the observation.
  int32_t value() const {
    DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
    return value_;
  }

  // Time when the observation was taken.
  base::TimeTicks timestamp() const {
    DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
    return timestamp_;
  }

  // Signal strength when the observation was taken.
  base::Optional<int32_t> signal_strength() const {
    DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
    return signal_strength_;
  }

  // The source of the observation.
  NetworkQualityObservationSource source() const {
    DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
    return source_;
  }

  // A unique identifier for the remote host which was used for the measurement.
  base::Optional<IPHash> host() const {
    DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
    return host_;
  }

 private:
  int32_t value_;

  base::TimeTicks timestamp_;

  base::Optional<int32_t> signal_strength_;

  NetworkQualityObservationSource source_;

  base::Optional<IPHash> host_;

  SEQUENCE_CHECKER(sequence_checker_);
};

}  // namespace internal

}  // namespace nqe

}  // namespace net

#endif  // NET_NQE_NETWORK_QUALITY_OBSERVATION_H_
