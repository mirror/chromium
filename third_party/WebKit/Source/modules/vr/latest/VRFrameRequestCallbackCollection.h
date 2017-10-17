// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef VRFrameRequestCallbackCollection_h
#define VRFrameRequestCallbackCollection_h

#include "platform/heap/Handle.h"

namespace blink {

class ExecutionContext;
class VRFrameRequestCallback;
class VRPresentationFrame;

class VRFrameRequestCallbackCollection final {
  DISALLOW_NEW();

 public:
  explicit VRFrameRequestCallbackCollection(ExecutionContext*);

  using CallbackId = int;
  CallbackId RegisterCallback(VRFrameRequestCallback*);
  void CancelCallback(CallbackId);
  void ExecuteCallbacks(VRPresentationFrame*);

  bool IsEmpty() const { return !callbacks_.size(); }

  DECLARE_TRACE();

 private:
  using CallbackList = HeapVector<Member<VRFrameRequestCallback>>;
  CallbackList callbacks_;
  CallbackList
      callbacks_to_invoke_;  // only non-empty while inside executeCallbacks

  CallbackId next_callback_id_ = 0;

  Member<ExecutionContext> context_;
};

}  // namespace blink

#endif  // FrameRequestCallbackCollection_h
