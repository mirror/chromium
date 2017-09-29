// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "modules/vr/latest/VRFrameRequestCallbackCollection.h"

#include "core/inspector/InspectorTraceEvents.h"
#include "core/probe/CoreProbes.h"
#include "modules/vr/latest/VRFrameRequestCallback.h"
#include "modules/vr/latest/VRPresentationFrame.h"

namespace blink {

VRFrameRequestCallbackCollection::VRFrameRequestCallbackCollection(
    ExecutionContext* context)
    : context_(context) {}

VRFrameRequestCallbackCollection::CallbackId
VRFrameRequestCallbackCollection::RegisterCallback(
    VRFrameRequestCallback* callback) {
  VRFrameRequestCallbackCollection::CallbackId id = ++next_callback_id_;
  callback->cancelled_ = false;
  callback->id_ = id;
  callbacks_.push_back(callback);

  /*TRACE_EVENT_INSTANT1("devtools.timeline", "RequestVRFrame",
                       TRACE_EVENT_SCOPE_THREAD, "data",
                       InspectorAnimationFrameEvent::Data(context_, id));*/
  probe::AsyncTaskScheduledBreakable(context_, "requestFrame", callback);
  return id;
}

void VRFrameRequestCallbackCollection::CancelCallback(CallbackId id) {
  for (size_t i = 0; i < callbacks_.size(); ++i) {
    if (callbacks_[i]->id_ == id) {
      probe::AsyncTaskCanceledBreakable(context_, "cancelFrame", callbacks_[i]);
      callbacks_.EraseAt(i);
      /*TRACE_EVENT_INSTANT1("devtools.timeline", "CancelVRFrame",
                           TRACE_EVENT_SCOPE_THREAD, "data",
                           InspectorAnimationFrameEvent::Data(context_, id));*/
      return;
    }
  }
  for (const auto& callback : callbacks_to_invoke_) {
    if (callback->id_ == id) {
      probe::AsyncTaskCanceledBreakable(context_, "cancelFrame", callback);
      /*TRACE_EVENT_INSTANT1("devtools.timeline", "CancelVRFrame",
                           TRACE_EVENT_SCOPE_THREAD, "data",
                           InspectorAnimationFrameEvent::Data(context_, id));*/
      callback->cancelled_ = true;
      // will be removed at the end of executeCallbacks()
      return;
    }
  }
}

void VRFrameRequestCallbackCollection::ExecuteCallbacks(
    VRPresentationFrame* frame) {
  // First, generate a list of callbacks to consider.  Callbacks registered from
  // this point on are considered only for the "next" frame, not this one.
  DCHECK(callbacks_to_invoke_.IsEmpty());
  callbacks_to_invoke_.swap(callbacks_);

  for (const auto& callback : callbacks_to_invoke_) {
    if (!callback->cancelled_) {
      /*TRACE_EVENT1("devtools.timeline", "FireVRAnimationFrame", "data",
                   InspectorAnimationFrameEvent::Data(context_,
         callback->id_));*/
      probe::AsyncTask async_task(context_, callback);
      probe::UserCallback probe(context_, "requestFrame", AtomicString(), true);
      callback->handleEvent(frame);
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
