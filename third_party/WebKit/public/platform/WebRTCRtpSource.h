// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef WebRTCRtpSource_h
#define WebRTCRtpSource_h

#include "WebCommon.h"

namespace blink {

enum class WebRTCRtpSourceType {
  SSRC,
  CSRC,
};

// https://w3c.github.io/webrtc-pc/#dom-rtcrtpcontributingsource
class BLINK_PLATFORM_EXPORT WebRTCRtpSource {
 public:
  virtual ~WebRTCRtpSource();

  virtual WebRTCRtpSourceType SourceType() const = 0;
  virtual double TimestampMs() const = 0;
  virtual uint32_t Source() const = 0;
  virtual uint8_t AudioLevel() const = 0;
  virtual bool HasAudioLevel() const = 0;
};

}  // namespace blink

#endif  // WebRTCRtpSource_h
