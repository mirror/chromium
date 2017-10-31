// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef RTCRtpSynchronizationSource_h
#define RTCRtpSynchronizationSource_h

#include "platform/bindings/ScriptWrappable.h"
#include "platform/heap/GarbageCollected.h"
#include "platform/heap/Member.h"
#include "platform/heap/Visitor.h"

namespace blink {

class RTCRtpReceiver;
class WebRTCRtpSource;

// https://w3c.github.io/webrtc-pc/#dom-rtcrtpsynchronizationsource
class RTCRtpSynchronizationSource final : public ScriptWrappable {
  DEFINE_WRAPPERTYPEINFO();

 public:
  RTCRtpSynchronizationSource(RTCRtpReceiver*, const WebRTCRtpSource&);
  // The source's source ID must match |source|.
  void UpdateMembers(const WebRTCRtpSource&);

  double timestamp();
  uint32_t source() const;
  uint8_t audioLevel(bool& isNull);

  virtual void Trace(blink::Visitor*);

 private:
  Member<RTCRtpReceiver> receiver_;
  double timestamp_ms_;
  const uint32_t source_;
  uint8_t audio_level_;
  bool has_audio_level_;
};

}  // namespace blink

#endif  // RTCRtpSynchronizationSource_h
