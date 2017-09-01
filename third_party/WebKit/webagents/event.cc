// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/WebKit/webagents/event.h"

#include "core/dom/events/Event.h"
#include "core/dom/events/EventTarget.h"
#include "public/platform/WebString.h"
#include "third_party/WebKit/webagents/event_target.h"

namespace webagents {

Event::Event(blink::Event& event) : event_(&event) {}

std::string Event::type() const {
  return blink::WebString(event_->type()).Ascii();
}
EventTarget Event::target() const {
  DCHECK(event_->target());
  return EventTarget(*event_->target());
}

}  // namespace webagents
