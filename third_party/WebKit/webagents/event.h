// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef WEBAGENTS_EVENT_H_
#define WEBAGENTS_EVENT_H_

#include "third_party/WebKit/webagents/webagents_export.h"

#include <string>

namespace blink {
class Event;
}

namespace webagents {

class EventTarget;

// https://dom.spec.whatwg.org/#interface-event
class WEBAGENTS_EXPORT Event {
 public:
  // readonly attribute DOMString type;
  std::string type() const;
  // readonly attribute EventTarget? target;
  EventTarget target() const;

 protected:
  friend class WebagentsEventListener;
  explicit Event(blink::Event&);
  blink::Event& GetEvent() const { return *event_; }

 private:
  // TODO(joelhockey): Does this need to be WebPrivatePtr?
  blink::Event* event_;
};

}  // namespace webagents

#endif  // WEBAGENTS_EVENT_H_
