// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef WEB_AGENT_API_EVENT_TARGET_H_
#define WEB_AGENT_API_EVENT_TARGET_H_

#include "platform/heap/Handle.h"

namespace blink {
class EventTarget;
}

namespace web {
class EventTarget : public blink::GarbageCollected<EventTarget> {
 public:
  DECLARE_TRACE();
  virtual ~EventTarget() = default;
  static EventTarget* Create(blink::EventTarget*);

 protected:
  explicit EventTarget(blink::EventTarget*);
  blink::EventTarget* GetEventTarget() const { return event_target_; }

 private:
  blink::Member<blink::EventTarget> event_target_;
};

}  // namespace web
#endif  // WEB_AGENT_API_EVENT_TARGET_H_
