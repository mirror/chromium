// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef WEBAGENTS_EVENT_H_
#define WEBAGENTS_EVENT_H_

#include "third_party/WebKit/webagents/webagents_export.h"

namespace blink {
class Event;
}

namespace webagents {

// https://dom.spec.whatwg.org/#interface-event
class WEBAGENTS_EXPORT Event {
 public:
  virtual ~Event();

 protected:
  explicit Event(blink::Event*);
  blink::Event* GetEvent() const { return event_; }

 private:
  blink::Event* event_;
};

}  // namespace webagents

#endif  // WEBAGENTS_EVENT_H_
