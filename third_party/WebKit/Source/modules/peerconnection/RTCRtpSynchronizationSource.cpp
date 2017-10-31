// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "modules/peerconnection/RTCRtpSynchronizationSource.h"

#include "modules/peerconnection/RTCRtpReceiver.h"
#include "public/platform/WebRTCRtpSource.h"

namespace blink {

RTCRtpSynchronizationSource::RTCRtpSynchronizationSource(
    RTCRtpReceiver* receiver,
    const WebRTCRtpSource& webSource)
    : receiver_(receiver),
      timestamp_ms_(webSource.TimestampMs()),
      source_(webSource.Source()),
      // TODO(zstein): DCHECK that we have a value here once we match the spec.
      audio_level_(webSource.AudioLevel()),
      has_audio_level_(webSource.HasAudioLevel()) {
  DCHECK(receiver_);
  DCHECK_EQ(webSource.SourceType(), WebRTCRtpSourceType::SSRC);
}

void RTCRtpSynchronizationSource::UpdateMembers(
    const WebRTCRtpSource& webSource) {
  timestamp_ms_ = webSource.TimestampMs();
  has_audio_level_ = webSource.HasAudioLevel();
  audio_level_ = webSource.AudioLevel();
  DCHECK_EQ(webSource.Source(), source_);
  DCHECK_EQ(webSource.SourceType(), WebRTCRtpSourceType::SSRC);
}

double RTCRtpSynchronizationSource::timestamp() {
  receiver_->UpdateSourcesIfNeeded();
  return timestamp_ms_;
}

uint32_t RTCRtpSynchronizationSource::source() const {
  // Skip |receiver_->UpdateSourcesIfNeeded()|, |source_| is a constant.
  return source_;
}

uint8_t RTCRtpSynchronizationSource::audioLevel(bool& isNull) {
  receiver_->UpdateSourcesIfNeeded();
  if (has_audio_level_)
    return audio_level_;

  isNull = true;
  // 127 is silence.
  return 127;
}

void RTCRtpSynchronizationSource::Trace(blink::Visitor* visitor) {
  visitor->Trace(receiver_);
  ScriptWrappable::Trace(visitor);
}

}  // namespace blink
