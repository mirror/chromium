// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "core/dom/events/EventTargetImpl.h"

namespace blink {

EventTargetImpl::EventTargetImpl(ScriptState* script_state) {
  ContextLifecycleObserver(ExecutionContext::From(script_state));
}

EventTargetImpl::~EventTargetImpl(){};

EventTargetImpl::InterfaceName() {
  return EventTargetNames::EventTargetImpl;
}

ExecutionContext* EventTargetImpl::GetExecutionContext() const {
  return ContextLifecycleObserver::GetExecutionContext();
}

}  // namespace blink
