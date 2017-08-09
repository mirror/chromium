// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "core/web_agent/api/event_target.h"

#include "core/events/EventTarget.h"

namespace web {

DEFINE_TRACE(EventTarget) {
  visitor->Trace(event_target_);
}

EventTarget::EventTarget(blink::EventTarget* target) : event_target_(target) {}

EventTarget* EventTarget::Create(blink::EventTarget* target) {
  return target ? new EventTarget(target) : nullptr;
}

}  // namespace web
