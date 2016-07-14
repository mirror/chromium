// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_TRACING_CORE_TRACE_EVENT_H_
#define COMPONENTS_TRACING_CORE_TRACE_EVENT_H_

#include "base/macros.h"
#include "components/tracing/tracing_export.h"

namespace tracing {
namespace v2 {

class TraceEventHandle;

class BASE_EXPORT TraceEvent : public ::tracing::Event {
 public:
  TraceEvent();
  ~TraceEvent() override;

  void set_event_handle(TraceEventHandle* handle) { handle_ = handle; }

 private:
  TraceEventHandle* handle_;

  DISALLOW_COPY_AND_ASSIGN(TraceEvent);
};

// Rationale of this handle: we want to return a (pointer to) Event to clients,
// so they can add extra properties (fill the proto fields).
// At the same time, we want to be in charge of its lifetime. More specifically
// an Event is valid for a given thread only until the next event is added.
// The functional purposes of this handle are:
//   1. Finalize the event when it goes out of scope from the caller.
//   2. Prevent bugs in the client code. If we did just return an Event* ptr,
//      clients might be tempted to keep it cached. Which means that once the
//      Event, that we own, is destroyed the client will have an invalid ptr
//      causing all sort of hard-to-debug memory bugs.
class BASE_EXPORT TraceEventHandle {
 public:
  TraceEventHandle();
  explicit TraceEventHandle(TraceEvent* event);
  ~TraceEventHandle();

  // Move-only type.
  TraceEventHandle(TraceEventHandle&& other);
  TraceEventHandle& operator=(TraceEventHandle&& other);

  TraceEvent& operator*() const { return *event_; }
  TraceEvent* operator->() const { return event_; }

 private:
  void Swap(TraceEventHandle* other);
  friend class TraceEvent;
  TraceEvent* event_;

  DISALLOW_COPY_AND_ASSIGN(TraceEventHandle);
};

}  // namespace v2
}  // namespace tracing

#endif  // COMPONENTS_TRACING_CORE_TRACE_EVENT_H_
