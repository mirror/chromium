// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef WebVideoFrameReader_h
#define WebVideoFrameReader_h

#include <memory>

#include "WebCommon.h"

namespace blink {

class WebVideoFrameReaderClient;
class WebMediaStreamTrack;

// Platform interface of a MediaRecorder.
class BLINK_PLATFORM_EXPORT WebVideoFrameReader {
 public:
  virtual ~WebVideoFrameReader() = default;
  virtual bool Initialize(WebMediaStreamTrack* track,
                          WebVideoFrameReaderClient* client) {
    return false;
  }
};

}  // namespace blink

#endif  // WebVideoFrameReader_h
