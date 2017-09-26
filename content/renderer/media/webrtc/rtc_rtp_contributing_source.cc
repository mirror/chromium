// Copyright (c) 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/renderer/media/webrtc/rtc_rtp_contributing_source.h"

#include "base/logging.h"
#include "base/time/time.h"
#include "third_party/webrtc/rtc_base/scoped_ref_ptr.h"

namespace content {

RTCRtpContributingSource::RTCRtpContributingSource(
    const webrtc::RtpSource& source)
    : source_(source) {}

RTCRtpContributingSource::~RTCRtpContributingSource() {}

blink::WebRTCRtpSourceType RTCRtpContributingSource::SourceType() const {
  switch (source_.source_type()) {
    case webrtc::RtpSourceType::SSRC:
      return blink::WebRTCRtpSourceType::SSRC;
    case webrtc::RtpSourceType::CSRC:
      return blink::WebRTCRtpSourceType::CSRC;
    default:
      NOTREACHED();
      return blink::WebRTCRtpSourceType::SSRC;
  }
}

double RTCRtpContributingSource::TimestampMs() const {
  return source_.timestamp_ms();
}

uint32_t RTCRtpContributingSource::Source() const {
  return source_.source_id();
}

uint8_t RTCRtpContributingSource::AudioLevel() const {
  // TODO(zstein): This should return null if we have no underlying value.
  // We return silence (127 - see the spec) for now instead.
  return source_.audio_level().value_or(127);
}

bool RTCRtpContributingSource::HasAudioLevel() const {
  return static_cast<bool>(source_.audio_level());
}

}  // namespace content
