// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/logging.h"
#include "ui/events/null_event_targeter.h"

namespace ui {

NullEventTargeter::NullEventTargeter() = default;

NullEventTargeter::~NullEventTargeter() = default;

EventTarget* NullEventTargeter::FindTargetForEvent(EventTarget* root,
                                                   Event* event) {
  return nullptr;
}

EventTarget* NullEventTargeter::FindNextBestTarget(EventTarget* previous_target,
                                                   Event* event) {
  NOTREACHED();
  return nullptr;
}

}  // namespace ui
