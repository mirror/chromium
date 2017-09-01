// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef WEBAGENTS_KEYBOARD_EVENT_H_
#define WEBAGENTS_KEYBOARD_EVENT_H_

#include <string>
#include "third_party/WebKit/webagents/ui_event.h"
#include "third_party/WebKit/webagents/webagents_export.h"

namespace blink {
class KeyboardEvent;
}

namespace webagents {

// https://w3c.github.io/uievents/#interface-keyboardevent
class WEBAGENTS_EXPORT KeyboardEvent : public UIEvent {
 public:
  //  readonly attribute DOMString key;
  std::string key() const;

 protected:
  explicit KeyboardEvent(blink::KeyboardEvent&);
  blink::KeyboardEvent& GetKeyboardEvent() const;
};

}  // namespace webagents

#endif  // WEBAGENTS_KEYBOARD_EVENT_H_
