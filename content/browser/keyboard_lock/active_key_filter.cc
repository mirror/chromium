// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/keyboard_lock/active_key_filter.h"

#include "ui/events/event.h"
#include "ui/events/platform/key_event_filter.h"

namespace content {

ActiveKeyFilter::ActiveKeyFilter() = default;
ActiveKeyFilter::~ActiveKeyFilter() = default;

bool ActiveKeyFilter::OnKeyEvent(const ui::KeyEvent& event) {
  return false;
}

}  // namespace content
