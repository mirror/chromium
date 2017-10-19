// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "modules/vr/latest/VRFrameRequestCallbackCollection.h"

#include "bindings/modules/v8/v8_vr_frame_request_callback.h"
#include "core/inspector/InspectorTraceEvents.h"
#include "core/probe/CoreProbes.h"

namespace blink {

class VRFrameRequestCallbackWrapper
    : public GarbageCollected<VRFrameRequestCallbackWrapper> {
 public:
  VRFrameRequestCallbackWrapper(V8VRFrameRequestCallback* callback, int id)
      : callback_(callback), id_(id) {}

  int id() { return id_; }
  bool cancelled() { return cancelled_; }

  void Invoke() { callback_->call(nullptr); };
  void Cancel() { cancelled_ = true; }

  DEFINE_INLINE_TRACE() { visitor->Trace(callback_); }

 private:
  Member<V8VRFrameRequestCallback> callback_;
  int id_;
  bool cancelled_ = false;
};

VRFrameRequestCallbackCollection::VRFrameRequestCallbackCollection(
    ExecutionContext* context)
    : context_(context) {}

VRFrameRequestCallbackCollection::CallbackId
VRFrameRequestCallbackCollection::RegisterCallback(
    V8VRFrameRequestCallback* callback) {
  VRFrameRequestCallbackWrapper* callback_wrapper =
      new VRFrameRequestCallbackWrapper(callback, ++next_callback_id_);
  callbacks_.push_back(callback_wrapper);

  probe::AsyncTaskScheduledBreakable(context_, "requestFrame", callback);
  return callback_wrapper->id();
}

void VRFrameRequestCallbackCollection::CancelCallback(CallbackId id) {
  for (size_t i = 0; i < callbacks_.size(); ++i) {
    if (callbacks_[i]->id() == id) {
      probe::AsyncTaskCanceledBreakable(context_, "cancelFrame", callbacks_[i]);
      callbacks_.EraseAt(i);
      return;
    }
  }
  for (const auto& callback : callbacks_to_invoke_) {
    if (callback->id() == id) {
      probe::AsyncTaskCanceledBreakable(context_, "cancelFrame", callback);
      callback->Cancel();
      // will be removed at the end of executeCallbacks()
      return;
    }
  }
}

void VRFrameRequestCallbackCollection::ExecuteCallbacks() {
  // First, generate a list of callbacks to consider.  Callbacks registered from
  // this point on are considered only for the "next" frame, not this one.
  DCHECK(callbacks_to_invoke_.IsEmpty());
  callbacks_to_invoke_.swap(callbacks_);

  for (const auto& callback : callbacks_to_invoke_) {
    if (!callback->cancelled()) {
      probe::AsyncTask async_task(context_, callback);
      probe::UserCallback probe(context_, "requestFrame", AtomicString(), true);
      callback->Invoke();
    }
  }

  callbacks_to_invoke_.clear();
}

DEFINE_TRACE(VRFrameRequestCallbackCollection) {
  visitor->Trace(callbacks_);
  visitor->Trace(callbacks_to_invoke_);
  visitor->Trace(context_);
}

}  // namespace blink
