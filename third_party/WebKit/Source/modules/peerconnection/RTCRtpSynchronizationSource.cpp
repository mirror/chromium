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
      audio_level_(webSource.HasAudioLevel() ? webSource.AudioLevel() : 127) {
  DCHECK(receiver_);
}

void RTCRtpSynchronizationSource::UpdateMembers(
    const WebRTCRtpSource& webSource) {
  timestamp_ms_ = webSource.TimestampMs();
  // TODO(zstein): According to the spec, we should compute the audio level if
  // it is not provided here. We report silence (127 - see RFC6464) for now
  // instead.
  audio_level_ = webSource.HasAudioLevel() ? webSource.AudioLevel() : 127;
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

uint8_t RTCRtpSynchronizationSource::audioLevel() {
  receiver_->UpdateSourcesIfNeeded();
  return audio_level_;
}

DEFINE_TRACE(RTCRtpSynchronizationSource) {
  visitor->Trace(receiver_);
}

}  // namespace blink
