// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef REMOTING_PROTOCOL_CAST_BANDWIDTH_ESTIMATOR_H_
#define REMOTING_PROTOCOL_CAST_BANDWIDTH_ESTIMATOR_H_

#include <memory>

#include "base/time/default_tick_clock.h"
#include "base/time/time.h"
#include "media/cast/common/frame_id.h"
#include "remoting/protocol/bandwidth_estimator.h"

namespace media {
namespace cast {
class CongestionControl;
}  // namespace cast
}  // namespace media

namespace remoting {
namespace protocol {

// An implementation of BandwidthEstimator by using AdaptiveCongestionControl in
// //media/cast/sender/.
class CastBandwidthEstimator final : public BandwidthEstimator {
 public:
  CastBandwidthEstimator();
  ~CastBandwidthEstimator() override;

  void UpdateRtt(base::TimeDelta rtt) override;
  void OnSendingFrame(const WebrtcVideoEncoder::EncodedFrame& frame) override;
  void OnReceivedAck() override;
  int GetBitrateKbps() override;

 private:
  base::DefaultTickClock tick_clock_;
  media::cast::FrameId frame_id_;
  const std::unique_ptr<media::cast::CongestionControl> control_;
};

}  // namespace protocol
}  // namespace remoting

#endif  // REMOTING_PROTOCOL_CAST_BANDWIDTH_ESTIMATOR_H_
