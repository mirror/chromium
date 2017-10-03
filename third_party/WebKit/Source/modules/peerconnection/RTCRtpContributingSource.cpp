// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "modules/peerconnection/RTCRtpContributingSource.h"

#include "modules/peerconnection/RTCRtpReceiver.h"
#include "public/platform/WebRTCRtpSource.h"

namespace blink {

RTCRtpContributingSource::RTCRtpContributingSource(
    RTCRtpReceiver* receiver,
    const WebRTCRtpSource& webSource)
    : receiver_(receiver),
      timestamp_ms_(webSource.TimestampMs()),
      source_(webSource.Source()) {
  DCHECK(receiver_);
}

void RTCRtpContributingSource::UpdateMembers(const WebRTCRtpSource& webSource) {
  timestamp_ms_ = webSource.TimestampMs();
  DCHECK_EQ(webSource.Source(), source_);
  DCHECK_EQ(webSource.SourceType(), WebRTCRtpSourceType::CSRC);
}

double RTCRtpContributingSource::timestamp() {
  receiver_->UpdateSourcesIfNeeded();
  return timestamp_ms_;
}

uint32_t RTCRtpContributingSource::source() const {
  // Skip |receiver_->UpdateSourcesIfNeeded()|, |source_| is a constant.
  return source_;
}

DEFINE_TRACE(RTCRtpContributingSource) {
  visitor->Trace(receiver_);
}

}  // namespace blink
