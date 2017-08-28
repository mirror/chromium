// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef WEBAGENTS_EVENT_TARGET_H_
#define WEBAGENTS_EVENT_TARGET_H_

namespace blink {
class EventTarget;
}

namespace webagents {

class EventTarget {
 public:
  virtual ~EventTarget() = default;

 protected:
  explicit EventTarget(blink::EventTarget*);
  blink::EventTarget* GetEventTarget() const { return event_target_; }

 private:
  blink::EventTarget* event_target_;
};

}  // namespace webagents

#endif  // WEBAGENTS_EVENT_TARGET_H_
