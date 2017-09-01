// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef VRFrameRequestCallback_h
#define VRFrameRequestCallback_h

#include "platform/heap/Handle.h"

namespace blink {

class VRPresentationFrame;

class VRFrameRequestCallback
    : public GarbageCollectedFinalized<VRFrameRequestCallback> {
 public:
  virtual ~VRFrameRequestCallback() {}
  DEFINE_INLINE_VIRTUAL_TRACE() {}
  virtual void handleEvent(VRPresentationFrame*) = 0;

  int id_;
  bool cancelled_;
};

}  // namespace blink

#endif  // VRFrameRequestCallback_h
