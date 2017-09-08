// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef WEBAGENTS_UI_EVENT_H_
#define WEBAGENTS_UI_EVENT_H_

#include "third_party/WebKit/webagents/event.h"
#include "third_party/WebKit/webagents/webagents_export.h"

namespace blink {
class UIEvent;
}

namespace webagents {

// https://w3c.github.io/uievents/#interface-uievent
class WEBAGENTS_EXPORT UIEvent : public Event {
 public:
#if INSIDE_BLINK
 protected:
  explicit UIEvent(blink::UIEvent&);
#endif
};

}  // namespace webagents

#endif  // WEBAGENTS_EVENT_TARGET_H_
