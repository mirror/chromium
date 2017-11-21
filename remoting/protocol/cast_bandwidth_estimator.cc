// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "remoting/protocol/cast_bandwidth_estimator.h"

#include "media/cast/common/frame_id.h"
#include "media/cast/sender/congestion_control.h"
#include "remoting/base/constants.h"

namespace remoting {
namespace protocol {

CastBandwidthEstimator::CastBandwidthEstimator()
    : frame_id_(media::cast::FrameId::first()),
      control_(media::cast::NewAdaptiveCongestionControl(
          &tick_clock_,
          1000000000,
          1000000,
          kTargetFrameRate)) {
  LOG(WARNING) << "CastBandwidthEstimator is used.";
  control_->UpdateTargetPlayoutDelay(base::TimeDelta());
}

CastBandwidthEstimator::~CastBandwidthEstimator() = default;

void CastBandwidthEstimator::UpdateRtt(base::TimeDelta rtt) {
  control_->UpdateRtt(rtt);
}

void CastBandwidthEstimator::OnSendingFrame(
    const WebrtcVideoEncoder::EncodedFrame& frame) {
  control_->SendFrameToTransport(frame_id_,
                                 frame.data.size() * 8,
                                 base::TimeTicks::Now());
  frame_id_++;
}

void CastBandwidthEstimator::OnReceivedAck() {
  control_->AckFrame(frame_id_ - 1, base::TimeTicks::Now());
}

int CastBandwidthEstimator::GetBitrateKbps() {
  return control_->GetBitrate(base::TimeTicks::Now(), base::TimeDelta());
}

}  // namespace protocol
}  // namespace remoting
