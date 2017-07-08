// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "public/platform/WebCommon.h"
#include "public/platform/WebPrivatePtr.h"

namespace blink {

class WebLocalFrameBase;
class WebRemoteFrameBase;

// Move-only opaque handle class to ensure a WebFrame remains alive even when
// not attached to the frame tree. The frame handle can be consumed by swapping
// it into the frame tree via WebFrame::Swap(), or released by letting the
// handle go out of scope.
class BLINK_EXPORT WebFrameHandle {
 public:
  ~WebFrameHandle();

  WebFrameHandle(const WebFrameHandle&) = delete;
  WebFrameHandle& operator=(const WebFrameHandle&) = delete;

  WebFrameHandle(WebFrameHandle&&);
  WebFrameHandle& operator=(WebFrameHandle&&);

 private:
#if BLINK_IMPLEMENTATION
  explicit WebFrameHandle(WebLocalFrameBase&);
  explicit WebFrameHandle(WebRemoteFrameBase&);
#endif

  // Note: these fields are mutually exclusive.
  WebPrivatePtr<WebLocalFrameBase> local_frame_;
  WebPrivatePtr<WebRemoteFrameBase> remote_frame_;
};

}  // namespace blink
