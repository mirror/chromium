// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/WebKit/webagents/ui_event.h"

#include "core/events/UIEvent.h"

namespace webagents {

UIEvent::UIEvent(blink::UIEvent& ui_event) : Event(ui_event) {}

}  // namespace webagents
