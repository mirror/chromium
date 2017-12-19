// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "modules/peerconnection/RTCRtpSender.h"

#include "modules/mediastream/MediaStreamTrack.h"
#include "modules/peerconnection/RTCPeerConnection.h"

namespace blink {

RTCRtpSender::RTCRtpSender(RTCPeerConnection* pc,
                           std::unique_ptr<WebRTCRtpSender> sender,
                           MediaStreamTrack* track)
    : pc_(pc), sender_(std::move(sender)), track_(track) {
  DCHECK(pc_);
  DCHECK(sender_);
  DCHECK(track_);
}

MediaStreamTrack* RTCRtpSender::track() {
  return track_;
}

ScriptPromise RTCRtpSender::replaceTrack(ScriptState* script_state,
                                         MediaStreamTrack* with_track) {
  return pc_->ReplaceTrack(script_state, this, with_track);
}

WebRTCRtpSender* RTCRtpSender::web_sender() {
  return sender_.get();
}

void RTCRtpSender::SetTrack(MediaStreamTrack* track) {
  track_ = track;
}

void RTCRtpSender::Trace(blink::Visitor* visitor) {
  visitor->Trace(pc_);
  visitor->Trace(track_);
  ScriptWrappable::Trace(visitor);
}

}  // namespace blink
