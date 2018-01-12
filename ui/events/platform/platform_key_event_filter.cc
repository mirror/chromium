// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/events/platform/platform_key_event_filter.h"

#include "ui/events/event.h"
#include "ui/events/event_utils.h"
#include "ui/events/platform/key_event_filter.h"

namespace ui {

PlatformKeyEventFilter::PlatformKeyEventFilter(KeyEventFilter* key_event_filter)
    : key_event_filter_(key_event_filter) {
  DCHECK(key_event_filter_);
}
PlatformKeyEventFilter::~PlatformKeyEventFilter() = default;

bool PlatformKeyEventFilter::OnPlatformEvent(const ui::PlatformEvent& event) {
  const ui::EventType event_type = ui::EventTypeFromNative(event);
  if (event_type != ui::ET_KEY_PRESSED && event_type != ui::ET_KEY_RELEASED) {
    return false;
  }

  // TODO(joedow): Confirm whether repeated keys are supported correctly.
  const ui::KeyboardCode key_code = ui::KeyboardCodeFromNative(event);
  const ui::DomCode dom_code = ui::CodeFromNative(event);
  const int flags = ui::EventFlagsFromNative(event);
  ui::KeyEvent key_event(event_type, key_code, dom_code, flags);
  return key_event_filter_->OnKeyEvent(key_event);
}

}  // namespace ui
