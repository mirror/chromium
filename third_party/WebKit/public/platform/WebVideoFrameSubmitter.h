// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef WebVideoFrameSubmitter_h
#define WebVideoFrameSubmitter_h

#include "WebCommon.h"
#include "cc/layers/video_frame_provider.h"

namespace viz {
class FrameSinkId;
}  // namespace viz

namespace blink {

class BLINK_PLATFORM_EXPORT WebVideoFrameSubmitter
    : public cc::VideoFrameProvider::Client {
 public:
  static WebVideoFrameSubmitter* Create(cc::VideoFrameProvider*,
                                        const viz::FrameSinkId&);
  virtual ~WebVideoFrameSubmitter() {}
};

}  // namespace blink

#endif  // WebVideoFrameSubmitter_h
