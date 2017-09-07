// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/WebKit/webagents/keyboard_event.h"

#include "core/events/KeyboardEvent.h"
#include "public/platform/WebString.h"

namespace webagents {

KeyboardEvent::KeyboardEvent(blink::KeyboardEvent& keyboard_event)
    : UIEvent(keyboard_event) {}

blink::WebString KeyboardEvent::key() const {
  return ConstUnwrap<blink::KeyboardEvent>().key();
}

}  // namespace webagents
