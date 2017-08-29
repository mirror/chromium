// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/WebKit/webagents/event_target.h"

#include "core/dom/events/EventTarget.h"

namespace webagents {

EventTarget::EventTarget(blink::EventTarget* target) : event_target_(target) {}

EventTarget::~EventTarget() {}

}  // namespace webagents
