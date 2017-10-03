// Copyright (c) 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/renderer/media/webrtc/rtc_rtp_source.h"

#include "base/logging.h"
#include "base/time/time.h"
#include "third_party/webrtc/rtc_base/scoped_ref_ptr.h"

namespace content {

RTCRtpSource::RTCRtpSource(const webrtc::RtpSource& source) : source_(source) {}

RTCRtpSource::~RTCRtpSource() {}

blink::WebRTCRtpSourceType RTCRtpSource::SourceType() const {
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

double RTCRtpSource::TimestampMs() const {
  return source_.timestamp_ms();
}

uint32_t RTCRtpSource::Source() const {
  return source_.source_id();
}

uint8_t RTCRtpSource::AudioLevel() const {
  // TODO(zstein): DCHECK that we have a value once we match the spec.
  return *source_.audio_level();
}

bool RTCRtpSource::HasAudioLevel() const {
  // TODO(zstein): DCHECK this is true for SSRCs once we match the spec.
  return static_cast<bool>(source_.audio_level());
}

}  // namespace content
