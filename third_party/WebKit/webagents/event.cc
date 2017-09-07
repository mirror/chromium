// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/WebKit/webagents/event.h"

#include "core/dom/events/Event.h"
#include "core/dom/events/EventTarget.h"
#include "public/platform/WebString.h"
#include "third_party/WebKit/webagents/event_target.h"

namespace webagents {

Event::~Event() {
  private_.Reset();
}

Event::Event(blink::Event& event) : private_(&event) {}

Event::Event(const Event& event) {
  Assign(event);
}

void Event::Assign(const Event& event) {
  private_ = event.private_;
}

blink::WebString Event::type() const {
  return private_->type();
}
EventTarget Event::target() const {
  DCHECK(private_->target());
  return EventTarget(*private_->target());
}

}  // namespace webagents
