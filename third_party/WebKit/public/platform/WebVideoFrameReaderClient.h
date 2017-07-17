// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef WebVideoFrameReaderClient_h
#define WebVideoFrameReaderClient_h

#include "WebCommon.h"
#include "third_party/skia/include/core/SkRefCnt.h"

class SkImage;

namespace blink {

class WebString;

// Interface used by a WebVideoFrameReader to get errors and recorded data
// delivered.
class WebVideoFrameReaderClient {
 public:
  virtual void OnVideoFrame(sk_sp<SkImage>) = 0;
  virtual void OnError(const WebString& message) = 0;
};

}  // namespace blink

#endif  // WebVideoFrameReaderClient_h
