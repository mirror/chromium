// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef REMOTING_PROTOCOL_BANDWIDTH_ESTIMATOR_H_
#define REMOTING_PROTOCOL_BANDWIDTH_ESTIMATOR_H_

#include "base/time/time.h"

namespace remoting {
namespace protocol {

// An interface to collect information from various sources and estimate the
// network bandwidth.
class BandwidthEstimator {
 public:
  BandwidthEstimator() = default;
  virtual ~BandwidthEstimator() = default;

  // Called at any time to update the latest round-trip delay.
  virtual void UpdateRtt(base::TimeDelta rtt) {}

  // Called before sending a frame with |frame_bytes| size.
  virtual void OnSendingFrame(size_t frame_bytes) {}

  // Called after a frame has been ACKED by the other end of the peer.
  virtual void OnReceivingAck() {}

  // Called at any time when an external estimator reports an estimate of
  // bitrate.
  virtual void OnBitrateEstimation(int bitrate_kbps) {}

  // Returns the estimate of the bitrate. This function returns 0 if no enough
  // data have been received for the implementation to make decision.
  virtual int GetBitrateKbps() = 0;
};

}  // namespace protocol
}  // namespace remoting

#endif  // REMOTING_PROTOCOL_BANDWIDTH_ESTIMATOR_H_
